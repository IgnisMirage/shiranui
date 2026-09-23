#ifndef JOY_TRANSLATE_HPP_
#define JOY_TRANSLATE_HPP_

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "std_msgs/msg/int32.hpp"
#include "controller.hpp"
#include "std_msgs/msg/empty.hpp"

class JoyTranslate : public rclcpp::Node
{
public:
    JoyTranslate(); 

private:
    void joy_output_cb(const sensor_msgs::msg::Joy & msg);
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr joy_twist_pub_;
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
    rclcpp::Publisher<std_msgs::msg::Empty>::SharedPtr manual_mode_pub_;
    rclcpp::Publisher<std_msgs::msg::Empty>::SharedPtr auto_mode_pub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    Controller controller;

    // Y: joy の指令を /cmd_vel に直接流す, LB/RB: 直結を解除
    bool direct_cmd_vel_mode_ = false;
    
    
    float max_linear_velocity_ = 0.8;
    float min_linear_velocity_ = -0.8;
    float max_angular_velocity_= 0.5;
    float min_angular_velocity_ = -0.5;
};

#endif  // JOY_TRANSLATE_HPP_
