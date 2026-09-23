#include "joy_translate/joy_translate.hpp"
#include "joy_translate/controller.hpp"

JoyTranslate::JoyTranslate()
: Node("joy_translate_node")
{
    joy_twist_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("manual_cmd_vel", 10);
    manual_mode_pub_ = this->create_publisher<std_msgs::msg::Empty>("manual_mode", 10);
    auto_mode_pub_ = this->create_publisher<std_msgs::msg::Empty>("auto_mode", 10);
    joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
        "joy", 10, std::bind(&JoyTranslate::joy_output_cb, this, std::placeholders::_1));
}

void JoyTranslate::joy_output_cb(const sensor_msgs::msg::Joy &msg)
{
    controller.update_key_value(msg);

    auto twist_msg = geometry_msgs::msg::Twist();
    twist_msg.linear.x = controller.joy_left_y > 0 ? 
                     controller.joy_left_y * max_linear_velocity_ : 
                     std::abs(controller.joy_left_y) * min_linear_velocity_;

    twist_msg.angular.z = controller.joy_right_x > 0 ? 
                     controller.joy_right_x * max_angular_velocity_ : 
                     std::abs(controller.joy_right_x) * min_angular_velocity_;
    joy_twist_pub_->publish(twist_msg);

    if (controller.LB_btn) {
        manual_mode_pub_->publish(std_msgs::msg::Empty());
    } else if (controller.RB_btn) {
        auto_mode_pub_->publish(std_msgs::msg::Empty());
    } 
}


int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<JoyTranslate>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
