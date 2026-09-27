#ifndef JOY_TRANSLATE_HPP_
#define JOY_TRANSLATE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "controller.hpp"

class JoyTranslate : public rclcpp::Node
{
public:
    JoyTranslate(); 

private:
    void joy_output_cb(const sensor_msgs::msg::Joy & msg);
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr joy_twist_pub_;
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
    Controller controller;
    
    
    float max_linear_velocity_{0.5F};
    float min_linear_velocity_{-0.5F};
    float max_angular_velocity_{0.7F};
    float min_angular_velocity_{-0.7F};
};

#endif  // JOY_TRANSLATE_HPP_
