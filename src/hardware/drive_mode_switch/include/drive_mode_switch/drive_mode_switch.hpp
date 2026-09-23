#ifndef DRIVE_MODE_SWITCH_HPP_
#define DRIVE_MODE_SWITCH_HPP_

#include "rclcpp/rclcpp.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "sensor_msgs/msg/joy.hpp"
#include "std_msgs/msg/string.hpp"

// /joy を盗み見て manual / auto を切り替え、選ばれた側の指令だけを /cmd_vel に流す。
// ボタンを押すと、もう一方のボタンが押されるまでそのモードを保持する。
class DriveModeSwitch : public rclcpp::Node
{
public:
    DriveModeSwitch();

private:
    enum class Mode { MANUAL, AUTO };

    void joy_cb(const sensor_msgs::msg::Joy & msg);
    void manual_cb(const geometry_msgs::msg::Twist & msg);
    void auto_cb(const geometry_msgs::msg::Twist & msg);
    void set_mode(Mode mode);
    static bool pressed(const sensor_msgs::msg::Joy & msg, int index);

    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr manual_sub_;
    rclcpp::Subscription<geometry_msgs::msg::Twist>::SharedPtr auto_sub_;
    rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr cmd_vel_pub_;
    rclcpp::Publisher<std_msgs::msg::String>::SharedPtr mode_pub_;

    int manual_button_;
    int auto_button_;
    Mode mode_ = Mode::MANUAL;
};

#endif  // DRIVE_MODE_SWITCH_HPP_
