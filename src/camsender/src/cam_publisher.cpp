#include <sstream>
#include <iostream>

#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

static const std::string TOPIC_NAME = "mcamera/rgb/image";

enum ERRORS {err_vcap_argument = 1, err_vcap_number, err_vcap_opening};

std::string gstreamer_pipeline (int camera_id, int capture_width, int capture_height, int display_width, int display_height, int framerate, int flip_method) {
    return 
    "nvarguscamerasrc sensor-id=" + std::to_string(camera_id) + 
    // "nvarguscamerasrc ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + 
    " ! video/x-raw(memory:NVMM), width=(int)" + std::to_string(capture_width) + 
    ", height=(int)" + std::to_string(capture_height) + 
    ", framerate=(fraction)" + std::to_string(framerate) +
    "/1 ! nvvidconv flip-method=" + std::to_string(flip_method) + 
    " ! video/x-raw, width=(int)" + std::to_string(display_width) + 
    ", height=(int)" + std::to_string(display_height) + 
    ", format=(string)BGRx ! videoconvert ! video/x-raw, format=(string)BGR ! appsink";
}

int main(int argc, char ** argv)
{
    // int capture_width = 1280;
    // int capture_height = 720;
    // int display_width = 1280;
    // int display_height = 720;
    int framerate = 21;
    int capture_width = 3264;
    int capture_height = 2464;
    int display_width = 3264;
    int display_height = 2464;
    // int framerate = 21;
    // int capture_width = 1920;
    // int capture_height = 1080;
    // int display_width = 1920;
    // int display_height = 1080;
    int flip_method = 0;
    try
    {
        std::cout << "Started" << std::endl;
        if (argv[1] == NULL) { throw err_vcap_argument; }
        ros::init(argc, argv, "camera_image_publisher");
        ros::NodeHandle nh;

        std::istringstream video_sourceCmd(argv[1]);
        int video_source;

        if (!(video_sourceCmd >> video_source)) { throw err_vcap_number; }
        std::string pipeline = gstreamer_pipeline(
            video_source,
            capture_width,
            capture_height,
            display_width,
            display_height,
            framerate,
            flip_method);

        std::cout << "Using pipeline: \n\t" << pipeline << "\n";

        cv::VideoCapture cap(pipeline, cv::CAP_GSTREAMER);
        // cv::VideoCapture cap(video_source);
        if (!cap.isOpened()) { throw err_vcap_opening; }
        
        cv::Mat frame;
        std_msgs::Header hdr;
        sensor_msgs::ImagePtr msg;   
        
        image_transport::ImageTransport it(nh);
        image_transport::Publisher pub = it.advertise(TOPIC_NAME, 1);   
        ros::Rate loop_rate(framerate);
        std::cout << "Main loop... " << std::endl;
        while (nh.ok())
        {
            cap >> frame;
            // Check if grabbed frame is actually full with some content
            if (!frame.empty()) 
            {
                // std::cout << "Sent" << std::endl;
                // ROS_INFO ("Sent");
                msg = cv_bridge::CvImage(hdr, "bgr8", frame).toImageMsg();
                pub.publish(msg);
                cv::waitKey(1);
            }
            loop_rate.sleep();
        }
        std::cout << "... is finished " << std::endl;
    }
    catch (ERRORS e)
    {
        switch (e)
        {
            case err_vcap_argument:
                std::cout << "Check if video source has been passed as a parameter" << std::endl;
            break;
            case err_vcap_number:
                std::cout << "Check if videosource is indeed a number" << std::endl;
            break;
            case err_vcap_opening:
                std::cout << "Check if video device can be opened with the given index" << std::endl;
            break;
            default: std::cout << "What the fuck happened" << std::endl;
        }
        return e;
    }
}