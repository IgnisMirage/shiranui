#include "drive_mode_switch/drive_mode_switch.hpp"

DriveModeSwitch::DriveModeSwitch()
: Node("drive_mode_switch")
{
    // ボタン番号は joy_translate/controller.hpp の割り当てに合わせる (RB=5, start=9)
    manual_button_ = this->declare_parameter<int>("manual_button", 5);
    auto_button_ = this->declare_parameter<int>("auto_button", 9);
    const bool start_auto = this->declare_parameter<bool>("start_in_auto", false);

    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::Twist>("cmd_vel", 10);
    mode_pub_ = this->create_publisher<std_msgs::msg::String>(
        "drive_mode", rclcpp::QoS(1).transient_local());

    joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
        "joy", 10, std::bind(&DriveModeSwitch::joy_cb, this, std::placeholders::_1));
    manual_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "manual_cmd_vel", 10, std::bind(&DriveModeSwitch::manual_cb, this, std::placeholders::_1));
    auto_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "auto_cmd_vel", 10, std::bind(&DriveModeSwitch::auto_cb, this, std::placeholders::_1));

    mode_ = start_auto ? Mode::AUTO : Mode::MANUAL;
    set_mode(mode_);
}

bool DriveModeSwitch::pressed(const sensor_msgs::msg::Joy & msg, int index)
{
    return index >= 0 && static_cast<size_t>(index) < msg.buttons.size() && msg.buttons[index];
}

void DriveModeSwitch::joy_cb(const sensor_msgs::msg::Joy & msg)
{
    const bool manual = pressed(msg, manual_button_);
    const bool autom = pressed(msg, auto_button_);
    // 同時押しは曖昧なので無視する
    if (manual && !autom && mode_ != Mode::MANUAL) {
        set_mode(Mode::MANUAL);
    } else if (autom && !manual && mode_ != Mode::AUTO) {
        set_mode(Mode::AUTO);
    }
}

void DriveModeSwitch::set_mode(Mode mode)
{
    mode_ = mode;
    // 切り替え直後に前のモードの指令が残らないよう一度止める
    cmd_vel_pub_->publish(geometry_msgs::msg::Twist());

    std_msgs::msg::String mode_msg;
    mode_msg.data = mode_ == Mode::MANUAL ? "manual" : "auto";
    mode_pub_->publish(mode_msg);
    RCLCPP_INFO(this->get_logger(), "Drive mode: %s", mode_msg.data.c_str());
}

void DriveModeSwitch::manual_cb(const geometry_msgs::msg::Twist & msg)
{
    if (mode_ == Mode::MANUAL) {
        cmd_vel_pub_->publish(msg);
    }
}

void DriveModeSwitch::auto_cb(const geometry_msgs::msg::Twist & msg)
{
    if (mode_ == Mode::AUTO) {
        cmd_vel_pub_->publish(msg);
    }
}

int main(int argc, char *argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<DriveModeSwitch>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
