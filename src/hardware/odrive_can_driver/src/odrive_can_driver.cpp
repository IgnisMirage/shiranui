#include "odrive_can_driver/odrive_can_driver.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>

using std::placeholders::_1;
using namespace std::chrono_literals;

namespace odrive_can_driver
{

MotorStatus::MotorStatus()
{
    rclcpp::Clock clock;
    last_heartbeat_ = clock.now();
    last_encoder_update_ = clock.now();
    last_status_update_ = clock.now();
}

ODriveCANDriver::ODriveCANDriver(const rclcpp::NodeOptions & options)
: Node("odrive_can_driver", options)
{
    this->declare_parameter("can_interface", "can0");
    this->declare_parameter("left_wheel_node_id", 1);
    this->declare_parameter("right_wheel_node_id", 2);
    this->declare_parameter("wheel_base", 0.3);      
    this->declare_parameter("wheel_radius", 0.05);   
    this->declare_parameter("max_velocity", 1.0);
    this->declare_parameter("velocity_timeout", 1.0);
    this->declare_parameter("traj_vel_limit", 10.0);
    this->declare_parameter("left_wheel_sign", 1.0);
    this->declare_parameter("right_wheel_sign", 1.0);
    
    can_interface_ = this->get_parameter("can_interface").as_string();
    left_wheel_node_id_ = this->get_parameter("left_wheel_node_id").as_int();
    right_wheel_node_id_ = this->get_parameter("right_wheel_node_id").as_int();
    wheel_base_ = this->get_parameter("wheel_base").as_double();
    wheel_radius_ = this->get_parameter("wheel_radius").as_double();
    max_velocity_ = this->get_parameter("max_velocity").as_double();
    velocity_timeout_ = this->get_parameter("velocity_timeout").as_double();
    traj_vel_limit_ = this->get_parameter("traj_vel_limit").as_double();
    left_wheel_sign_ = this->get_parameter("left_wheel_sign").as_double();
    right_wheel_sign_ = this->get_parameter("right_wheel_sign").as_double();
    
    if (!init_can_interface()) {
        RCLCPP_ERROR(this->get_logger(), "Failed to initialize CAN interface");
        return;
    }
    
    cmd_vel_sub_ = this->create_subscription<geometry_msgs::msg::Twist>(
        "cmd_vel", 10, std::bind(&ODriveCANDriver::cmd_vel_callback, this, _1));
    
    odom_pub_ = this->create_publisher<nav_msgs::msg::Odometry>("odom", 10);
    tf_broadcaster_ = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    auto odom_timer = this->create_wall_timer(50ms, std::bind(&ODriveCANDriver::update_odometry, this));
    
    // ODrive Info Initialization
    initialize_odrive(left_wheel_node_id_);
    initialize_odrive(right_wheel_node_id_);
    can_receive_thread_ = std::thread(&ODriveCANDriver::can_receive_loop, this);
    
    last_cmd_time_ = this->now();
    last_odom_time_ = this->now();
}

ODriveCANDriver::~ODriveCANDriver()
{
    running_ = false;
    stop_motors();
    if (can_receive_thread_.joinable()) {
        can_receive_thread_.join();
    }    
    if (can_socket_ >= 0) {
        close(can_socket_);
    }
    RCLCPP_INFO(this->get_logger(), "ODrive CAN Driver shutdown");
}

bool ODriveCANDriver::init_can_interface()
{
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket_ < 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to create CAN socket");
        return false;
    }
    struct ifreq ifr;
    strcpy(ifr.ifr_name, can_interface_.c_str());
    if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to get CAN interface index for %s", can_interface_.c_str());
        close(can_socket_);
        can_socket_ = -1;
        return false;
    }
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(can_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        RCLCPP_ERROR(this->get_logger(), "Failed to bind CAN socket");
        close(can_socket_);
        can_socket_ = -1;
        return false;
    }
    running_ = true;
    RCLCPP_INFO(this->get_logger(), "CAN interface %s initialized successfully", can_interface_.c_str());
    return true;
}

void ODriveCANDriver::initialize_odrive(uint8_t node_id)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_CLEAR_ERRORS;
    frame.can_dlc = 0;
    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to send clear errors command to node %d", node_id);
        return;
    }
    std::this_thread::sleep_for(100ms);
    send_controller_mode_command(node_id, CONTROL_MODE_VELOCITY_CONTROL, INPUT_MODE_VEL_RAMP);
    std::this_thread::sleep_for(100ms);
    send_traj_vel_limit_command(node_id, static_cast<float>(traj_vel_limit_)); // 台形軌道の速度制限 [回転/秒]
    std::this_thread::sleep_for(100ms);
    send_axis_state_command(node_id, AXIS_STATE_CLOSED_LOOP_CONTROL);
}

void ODriveCANDriver::cmd_vel_callback(const geometry_msgs::msg::Twist::SharedPtr msg)
{
    last_cmd_time_ = this->now();
    cmd_vel_timed_out_ = false;

    double linear_vel = msg->linear.x;
    double angular_vel = msg->angular.z;

    double left_wheel_vel, right_wheel_vel;
    differential_drive_kinematics(linear_vel, angular_vel, left_wheel_vel, right_wheel_vel);
    clamp_wheel_velocities(left_wheel_vel, right_wheel_vel);

    double left_turns_per_sec = (left_wheel_vel / (2.0 * M_PI * wheel_radius_)) * left_wheel_sign_;
    double right_turns_per_sec = (right_wheel_vel / (2.0 * M_PI * wheel_radius_)) * right_wheel_sign_;

    send_velocity_command(left_wheel_node_id_, left_turns_per_sec);
    send_velocity_command(right_wheel_node_id_, right_turns_per_sec);
}

void ODriveCANDriver::check_cmd_vel_timeout()
{
    if ((this->now() - last_cmd_time_).seconds() <= velocity_timeout_) {
        return;
    }

    if (!cmd_vel_timed_out_) {
        RCLCPP_WARN(this->get_logger(), "cmd_vel timeout (%.1f s), stopping motors", velocity_timeout_);
        stop_motors();
        cmd_vel_timed_out_ = true;
    }
}

void ODriveCANDriver::clamp_wheel_velocities(double& left_wheel_vel, double& right_wheel_vel)
{
    const double max_wheel_vel = std::max(std::abs(left_wheel_vel), std::abs(right_wheel_vel));
    if (max_wheel_vel <= max_velocity_) {
        return;
    }

    const double scale = max_velocity_ / max_wheel_vel;
    left_wheel_vel *= scale;
    right_wheel_vel *= scale;
}

void ODriveCANDriver::differential_drive_kinematics(double linear_vel, double angular_vel, 
                                                   double& left_wheel_vel, double& right_wheel_vel)
{
    // 差動駆動の運動学
    // v_left = v - (w * L) / 2
    // v_right = v + (w * L) / 2
    // v = 線形速度、w = 角速度、L = 車輪間距離
    
    left_wheel_vel = linear_vel - (angular_vel * wheel_base_) / 2.0;
    right_wheel_vel = linear_vel + (angular_vel * wheel_base_) / 2.0;
}

void ODriveCANDriver::send_velocity_command(uint8_t node_id, double velocity)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_SET_INPUT_VEL;
    frame.can_dlc = 8;
    
    // 速度をfloatとしてバイト配列に変換
    float vel_float = static_cast<float>(velocity);
    float ff_torque = 0.0f; // フィードフォワードトルク
    float_to_bytes(vel_float, &frame.data[0]);
    float_to_bytes(ff_torque, &frame.data[4]);
    
    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to send velocity command to node %d", node_id);
    }
}

void ODriveCANDriver::stop_motors()
{
    send_velocity_command(left_wheel_node_id_, 0.0);
    send_velocity_command(right_wheel_node_id_, 0.0);
}

void ODriveCANDriver::send_controller_mode_command(uint8_t node_id, ODriveControlMode control_mode, uint32_t input_mode)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_SET_CONTROLLER_MODES;
    frame.can_dlc = 8;
    
    uint32_to_bytes(static_cast<uint32_t>(control_mode), &frame.data[0]);
    uint32_to_bytes(input_mode, &frame.data[4]);
    
    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to send controller mode command to node %d", node_id);
    }
}

void ODriveCANDriver::send_clear_errors_command(uint8_t node_id, uint8_t identify)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_CLEAR_ERRORS;
    frame.can_dlc = 1;

    // identify: uint8 [0:通常クリア, 1:identify有効]
    frame.data[0] = identify;

    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to send clear errors command to node %d", node_id);
    }
}

void ODriveCANDriver::send_traj_vel_limit_command(uint8_t node_id, float vel_limit)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_SET_TRAJ_VEL_LIMIT;
    frame.can_dlc = 4;

    // Traj_Vel_Limit: float32 [回転/秒]
    float_to_bytes(vel_limit, &frame.data[0]);

    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to send traj vel limit command to node %d", node_id);
    }
}

void ODriveCANDriver::send_axis_state_command(uint8_t node_id, ODriveAxisState state)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_SET_AXIS_REQUESTED_STATE;
    frame.can_dlc = 4;
    
    uint32_to_bytes(static_cast<uint32_t>(state), frame.data);
    
    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_ERROR(this->get_logger(), "Failed to send axis state command to node %d", node_id);
    }
}

void ODriveCANDriver::can_receive_loop()
{
    struct can_frame frame;
    while (running_) {
        ssize_t nbytes = read(can_socket_, &frame, sizeof(frame));
        if (nbytes < 0) {
            if (running_) {
                RCLCPP_ERROR(this->get_logger(), "CAN read error");
            }
            break;
        }
        if (nbytes == sizeof(frame)) {
            process_can_message(frame);
        }
    }
}

void ODriveCANDriver::process_can_message(const struct can_frame& frame)
{
    uint8_t node_id = (frame.can_id >> 5) & 0x3F;  // 6ビット (ビット10-5)
    uint32_t cmd_id = frame.can_id & 0x1F;          // 5ビット (ビット4-0)
    
    switch (cmd_id) {
        case MSG_ODRIVE_HEARTBEAT:
            if (frame.can_dlc >= 7) {
                motor_status_[node_id].setAxisError(bytes_to_uint32(&frame.data[0]));
                motor_status_[node_id].setAxisState(frame.data[4]);
                motor_status_[node_id].setProcedureResult(frame.data[5]);
                motor_status_[node_id].setTrajectoryDone(frame.data[6] != 0);
                motor_status_[node_id].updateHeartbeat();

                if (motor_status_[node_id].hasError()) {
                    RCLCPP_WARN_THROTTLE(
                        this->get_logger(), *this->get_clock(), 1000,
                        "ODrive node %d error: axis_error=0x%08X, state=%u",
                        node_id,
                        motor_status_[node_id].getAxisError(),
                        motor_status_[node_id].getAxisState());
                }
            }
            break;
            
        case MSG_GET_ENCODER_ESTIMATES:
            if (frame.can_dlc >= 8) {
                float pos = bytes_to_float(&frame.data[0]);
                float vel = bytes_to_float(&frame.data[4]);
                
                // モーター状態を更新
                motor_status_[node_id].setPositionEstimate(pos);
                motor_status_[node_id].setVelocityEstimate(vel);
                
                // RCLCPP_DEBUG(this->get_logger(), "Node %d - Position: %.3f, Velocity: %.3f", node_id, pos, vel);
            }
            break;
            
        default:
            // その他のメッセージ
            break;
    }
}


void ODriveCANDriver::request_encoder_estimates(uint8_t node_id)
{
    struct can_frame frame;
    frame.can_id = (node_id << 5) | MSG_GET_ENCODER_ESTIMATES;
    frame.can_dlc = 0;  // リクエストなのでデータ長は0
    
    if (write(can_socket_, &frame, sizeof(frame)) != sizeof(frame)) {
        RCLCPP_WARN(this->get_logger(), "Failed to send encoder estimates request to node %d", node_id);
    }
}

void ODriveCANDriver::update_odometry()
{
    check_cmd_vel_timeout();

    request_encoder_estimates(left_wheel_node_id_);
    request_encoder_estimates(right_wheel_node_id_);
    
    // モーター状態を確認
    auto left_status = motor_status_.find(left_wheel_node_id_);
    auto right_status = motor_status_.find(right_wheel_node_id_);
    
    if (left_status == motor_status_.end() || right_status == motor_status_.end()) {
        return;
    }
    
    // 現在の時刻を取得
    auto current_time = this->now();
    double dt = (current_time - last_odom_time_).seconds();
    
    if (dt <= 0.0) {
        return; 
    }
    
    double prev_left_pos = left_wheel_position_;
    double prev_right_pos = right_wheel_position_;    

    left_wheel_position_ = left_status->second.getPositionEstimate();
    right_wheel_position_ = right_status->second.getPositionEstimate();
    
    const double wheel_circumference = 2.0 * M_PI * wheel_radius_;
    double delta_left = (left_wheel_position_ - prev_left_pos) * left_wheel_sign_;
    double delta_right = (right_wheel_position_ - prev_right_pos) * right_wheel_sign_;
    double left_distance = delta_left * wheel_circumference;
    double right_distance = delta_right * wheel_circumference;
    
    double delta_distance = (left_distance + right_distance) / 2.0;
    double delta_theta = (right_distance - left_distance) / wheel_base_;
    
    double delta_x = delta_distance * cos(theta_ + delta_theta / 2.0);
    double delta_y = delta_distance * sin(theta_ + delta_theta / 2.0);
    
    x_position_ += delta_x;
    y_position_ += delta_y;
    theta_ += delta_theta;
    
    // 角度を正規化 (-π to π)
    while (theta_ > M_PI) theta_ -= 2.0 * M_PI;
    while (theta_ < -M_PI) theta_ += 2.0 * M_PI;
    
    measured_linear_vel_ = delta_distance / dt;
    measured_angular_vel_ = delta_theta / dt;

    publish_odometry();    
    last_odom_time_ = current_time;
}

void ODriveCANDriver::publish_odometry()
{
    auto current_time = this->now();
    auto odom_msg = nav_msgs::msg::Odometry();
    odom_msg.header.stamp = current_time;
    odom_msg.header.frame_id = "odom";
    odom_msg.child_frame_id = "base_link";

    odom_msg.pose.pose.position.x = x_position_;
    odom_msg.pose.pose.position.y = y_position_;
    odom_msg.pose.pose.position.z = 0.0;
    
    tf2::Quaternion q;
    q.setRPY(0, 0, theta_);
    odom_msg.pose.pose.orientation.x = q.x();
    odom_msg.pose.pose.orientation.y = q.y();
    odom_msg.pose.pose.orientation.z = q.z();
    odom_msg.pose.pose.orientation.w = q.w();
    
    odom_msg.twist.twist.linear.x = measured_linear_vel_;
    odom_msg.twist.twist.linear.y = 0.0;
    odom_msg.twist.twist.angular.z = measured_angular_vel_;
    
    odom_msg.pose.covariance[0] = 0.1;   // x
    odom_msg.pose.covariance[7] = 0.1;   // y
    odom_msg.pose.covariance[35] = 0.1;  // yaw
    odom_msg.twist.covariance[0] = 0.1;  // linear.x
    odom_msg.twist.covariance[35] = 0.1; // angular.z
    
    odom_pub_->publish(odom_msg);
    
    geometry_msgs::msg::TransformStamped transform;
    transform.header.stamp = current_time;
    transform.header.frame_id = "odom";
    transform.child_frame_id = "base_footprint";
    
    transform.transform.translation.x = x_position_;
    transform.transform.translation.y = y_position_;
    transform.transform.translation.z = 0.0;
    
    transform.transform.rotation.x = q.x();
    transform.transform.rotation.y = q.y();
    transform.transform.rotation.z = q.z();
    transform.transform.rotation.w = q.w();
    
    tf_broadcaster_->sendTransform(transform);
}

// ユーティリティ関数
//#############################################################################
void ODriveCANDriver::float_to_bytes(float value, uint8_t* bytes)
{
    memcpy(bytes, &value, sizeof(float));
}

float ODriveCANDriver::bytes_to_float(const uint8_t* bytes)
{
    float value;
    memcpy(&value, bytes, sizeof(float));
    return value;
}

void ODriveCANDriver::uint32_to_bytes(uint32_t value, uint8_t* bytes)
{
    memcpy(bytes, &value, sizeof(uint32_t));
}

uint32_t ODriveCANDriver::bytes_to_uint32(const uint8_t* bytes)
{
    uint32_t value;
    memcpy(&value, bytes, sizeof(uint32_t));
    return value;
}
//#############################################################################
}  // namespace odrive_can_driver

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(odrive_can_driver::ODriveCANDriver)
