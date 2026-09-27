#include "joy_translate/joy_translate.hpp"
#include "joy_translate/controller.hpp"

#include <cmath>

JoyTranslate::JoyTranslate()
: Node("joy_translate_node")
{
    max_linear_velocity_ = static_cast<float>(declare_parameter("max_linear_velocity", 0.5));
    max_angular_velocity_ = static_cast<float>(declare_parameter("max_angular_velocity", 0.7));
    min_linear_velocity_ = -std::abs(max_linear_velocity_);
    min_angular_velocity_ = -std::abs(max_angular_velocity_);

    joy_twist_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("manual_cmd_vel", 10);
    joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
        "joy", 10, std::bind(&JoyTranslate::joy_output_cb, this, std::placeholders::_1));
}

void JoyTranslate::joy_output_cb(const sensor_msgs::msg::Joy &msg)
{
    controller.update_key_value(msg);

    auto twist_msg = geometry_msgs::msg::Twist();
    twist_msg.linear.x = controller.joy_left_y >= 0.0
                             ? controller.joy_left_y * max_linear_velocity_
                             : controller.joy_left_y * std::abs(min_linear_velocity_);

    twist_msg.angular.z = controller.joy_right_x >= 0.0
                              ? controller.joy_right_x * max_angular_velocity_
                              : controller.joy_right_x * std::abs(min_angular_velocity_);
    joy_twist_pub_->publish(twist_msg);
}


int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<JoyTranslate>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
