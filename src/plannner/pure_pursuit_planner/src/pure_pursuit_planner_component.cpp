#include "pure_pursuit_planner/pure_pursuit_planner_component.hpp"

#include <angles/angles.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <limits>

using std::placeholders::_1;

namespace pure_pursuit_planner
{
namespace
{
// |v| がこれ未満は 0 扱いにしてから最小絶対値のクランプを行う
constexpr double kCmdVelDeadzone = 1e-4;

void clampCmdAxisToMinimum(double & v, double v_min_abs)
{
  const double av = std::abs(v);
  if (av < kCmdVelDeadzone) {
    v = 0.0;
    return;
  }
  if (v_min_abs <= kCmdVelDeadzone) {
    return;
  }
  if (av < v_min_abs) {
    v = std::copysign(v_min_abs, v);
    return;
  }
  // 最小値付近 (0.0500 と 0.05 など) は最小値にスナップする
  if (av <= v_min_abs + kCmdVelDeadzone) {
    v = std::copysign(v_min_abs, v);
  }
}

bool isZeroTime(const rclcpp::Time & t) {return t.nanoseconds() == 0;}
}  // namespace

PurePursuitNode::PurePursuitNode(const rclcpp::NodeOptions & node_options)
: Node("pure_pursuit_planner", node_options),
  tf_buffer_(this->get_clock()),
  tf_listener_(tf_buffer_)
{
  robot_frame_id_ = declare_parameter<std::string>("robot_frame_id", "base_link");
  map_frame_id_ = declare_parameter<std::string>("map_frame_id", "map");
  control_rate_ = declare_parameter<double>("control_rate", 20.0);

  // --- 状態遷移 ---
  transition_wait_time_ = declare_parameter<double>("transition_wait_time", 0.2);
  goal_standstill_linear_thresh_ = declare_parameter<double>("goal_standstill_linear_thresh", 0.01);
  goal_standstill_angular_thresh_ = declare_parameter<double>("goal_standstill_angular_thresh", 0.05);
  goal_standstill_confirm_time_ = declare_parameter<double>("goal_standstill_confirm_time", 0.1);
  goal_standstill_timeout_ = declare_parameter<double>("goal_standstill_timeout", 2.0);
  odom_timeout_ = declare_parameter<double>("odom_timeout", 0.5);

  // --- 許容値 ---
  start_tolerance_ang_ = declare_parameter<double>("start_tolerance_ang", 0.03);
  goal_tolerance_dist_ = declare_parameter<double>("goal_tolerance_dist", 0.03);
  goal_rotation_only_distance_threshold_ =
    declare_parameter<double>("goal_rotation_only_distance_threshold", 0.2);
  goal_tolerance_ang_ = declare_parameter<double>("goal_tolerance_ang", 0.03);
  turn_in_place_threshold_ = declare_parameter<double>("turn_in_place_threshold", 0.2);

  // --- 旋回整合 ---
  align_angular_vel_ = declare_parameter<double>("align_angular_vel", 0.5);
  align_angular_acceleration_ = declare_parameter<double>("align_angular_acceleration", 1.0);
  align_slowdown_angle_ = declare_parameter<double>("align_slowdown_angle", 0.2);
  align_slowdown_angular_vel_ = declare_parameter<double>("align_slowdown_angular_vel", 0.08);
  align_angular_deceleration_ = declare_parameter<double>("align_angular_deceleration", 0.4);

  // --- 速度・加減速 ---
  cruise_speed_ = declare_parameter<double>("cruise_speed", 0.8);
  max_angular_velocity_ = declare_parameter<double>("max_angular_velocity", 1.0);
  min_cmd_linear_abs_ = declare_parameter<double>("min_cmd_linear_abs", 0.02);
  min_cmd_angular_abs_ = declare_parameter<double>("min_cmd_angular_abs", 0.01);
  acceleration_ = declare_parameter<double>("acceleration", 0.2);
  deceleration_ = declare_parameter<double>("deceleration", 0.5);
  max_angular_acceleration_ = declare_parameter<double>("max_angular_acceleration", 0.5);
  max_angular_deceleration_ = declare_parameter<double>("max_angular_deceleration", 0.5);
  slowdown_deceleration_ = declare_parameter<double>("slowdown_deceleration", 0.15);
  brake_deceleration_ = declare_parameter<double>("brake_deceleration", 0.5);
  brake_timeout_ = declare_parameter<double>("brake_timeout", 0.2);

  // --- スタート/ゴールアプローチ ---
  start_approach_distance_ = declare_parameter<double>("start_approach_distance", 0.0);
  start_approach_speed_ = declare_parameter<double>("start_approach_speed", 0.3);
  goal_approach_distance_ = declare_parameter<double>("goal_approach_distance", 0.0);
  goal_approach_speed_ = declare_parameter<double>("goal_approach_speed", 0.05);

  // --- 停止後リカバリ ---
  post_stop_recovery_distance_ = declare_parameter<double>("post_stop_recovery_distance", 0.3);
  post_stop_recovery_speed_ = declare_parameter<double>("post_stop_recovery_speed", 0.3);

  // --- Pure Pursuit ---
  lookahead_distance_ = declare_parameter<double>("lookahead_distance", 0.8);
  min_lookahead_distance_ = declare_parameter<double>("min_lookahead_distance", 0.3);
  curvature_safety_factor_ = declare_parameter<double>("curvature_safety_factor", 0.6);
  slow_speed_lookahead_threshold_ =
    declare_parameter<double>("slow_speed_lookahead_threshold", 0.1);
  slow_speed_lookahead_distance_ =
    declare_parameter<double>("slow_speed_lookahead_distance", 0.35);
  sensor_delay_ = declare_parameter<double>("sensor_delay", 0.01);

  // --- 経路逸脱 ---
  enable_path_deviation_check_ = declare_parameter<bool>("enable_path_deviation_check", true);
  path_deviation_threshold_ = declare_parameter<double>("path_deviation_threshold", 2.0);
  path_deviation_recovery_threshold_ =
    declare_parameter<double>("path_deviation_recovery_threshold", 0.8);

  publish_approach_zone_marker_ = declare_parameter<bool>("publish_approach_zone_marker", false);

  pp_lookahead_distance_ = lookahead_distance_;
  effective_max_speed_ = cruise_speed_;
  last_cmd_time_ = now();

  path_sub_ = create_subscription<nav_msgs::msg::Path>(
    "path", rclcpp::QoS(10).transient_local().reliable().keep_last(1),
    std::bind(&PurePursuitNode::pathCallback, this, _1));
  odom_sub_ = create_subscription<nav_msgs::msg::Odometry>(
    "odom", 1, std::bind(&PurePursuitNode::odomCallback, this, _1));
  speed_sub_ = create_subscription<std_msgs::msg::Float32>(
    "/local_planner/speed", 1, std::bind(&PurePursuitNode::speedCallback, this, _1));
  brake_sub_ = create_subscription<std_msgs::msg::Bool>(
    "/speed_handler/brake", 1, std::bind(&PurePursuitNode::brakeCallback, this, _1));
  slowdown_sub_ = create_subscription<std_msgs::msg::Bool>(
    "/speed_handler/slowdown", 1, std::bind(&PurePursuitNode::slowdownCallback, this, _1));
  pause_sub_ = create_subscription<std_msgs::msg::Empty>(
    "/pause", 1, std::bind(&PurePursuitNode::pauseCallback, this, _1));
  resume_sub_ = create_subscription<std_msgs::msg::Empty>(
    "/resume", 1, std::bind(&PurePursuitNode::resumeCallback, this, _1));

  cmd_pub_ = create_publisher<geometry_msgs::msg::Twist>("/auto_cmd_vel", 10);
  mode_pub_ = create_publisher<std_msgs::msg::String>("~/mode", 10);
  following_pub_ = create_publisher<std_msgs::msg::Bool>("~/following", 10);
  reached_pub_ = create_publisher<std_msgs::msg::Bool>("~/goal_reached", 10);
  marker_pub_ = create_publisher<visualization_msgs::msg::MarkerArray>("~/marker", 10);
  approach_zone_pub_ = create_publisher<visualization_msgs::msg::MarkerArray>(
    "~/approach_zone_marker", rclcpp::QoS(1).transient_local().reliable());

  control_timer_ = create_wall_timer(
    std::chrono::duration<double>(1.0 / std::max(control_rate_, 1.0)),
    std::bind(&PurePursuitNode::controlLoop, this));

  RCLCPP_INFO(
    get_logger(),
    "PurePursuit initialized (accel %.2f m/s^2, decel %.2f m/s^2, ang_accel %.2f rad/s^2, "
    "ang_decel %.2f rad/s^2, min_cmd_linear %.4f, min_cmd_angular %.4f)",
    acceleration_, deceleration_, max_angular_acceleration_, max_angular_deceleration_,
    min_cmd_linear_abs_, min_cmd_angular_abs_);
}

PurePursuitNode::~PurePursuitNode()
{
  if (cmd_pub_) {
    cmd_pub_->publish(geometry_msgs::msg::Twist());
  }
}

double PurePursuitNode::elapsedSince(const rclcpp::Time & t) const
{
  return (now() - t).seconds();
}

// ---------------------------------------------------------------------------
// コールバック
// ---------------------------------------------------------------------------
void PurePursuitNode::pathCallback(const nav_msgs::msg::Path::SharedPtr msg)
{
  if (!isValidPath(*msg)) {
    RCLCPP_WARN(get_logger(), "Received path with NaN/Inf positions, ignoring");
    return;
  }
  current_path_ = *msg;
  rotate_at_start_complete_ = false;
  rotate_at_goal_complete_ = false;
  rotate_at_start_ready_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
  rotate_at_goal_ready_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
  rotate_at_goal_standstill_confirmed_ = false;
  goal_standstill_since_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
  align_direction_sign_ = 0;
  align_force_goal_flag_ = false;
  post_stop_recovery_start_dist_ = -1.0;
  updateStateOnPath();
  computeArcLength();
  setPurePursuitPath();
  publishApproachZoneMarkers();
  RCLCPP_INFO(
    get_logger(), "Path received (%zu poses), state: %s", current_path_.poses.size(),
    stateToString(state_).c_str());
}

void PurePursuitNode::odomCallback(const nav_msgs::msg::Odometry::SharedPtr msg)
{
  measured_linear_speed_ = std::hypot(msg->twist.twist.linear.x, msg->twist.twist.linear.y);
  measured_angular_speed_ = msg->twist.twist.angular.z;
  last_odom_time_ = now();
}

void PurePursuitNode::speedCallback(const std_msgs::msg::Float32::SharedPtr msg)
{
  external_speed_limit_ = msg->data;
  priority_speed_ = msg->data;
}

void PurePursuitNode::brakeCallback(const std_msgs::msg::Bool::SharedPtr msg)
{
  last_brake_msg_time_ = now();
  if (is_brake_active_ && !msg->data) {
    // 走行中の安全停止からの再開時のみリカバリを開始する
    // (ゴール後のブレーキ解除などでは開始しない)
    if (state_ == State::FOLLOW_PATH || state_ == State::PATH_DEVIATION_WARNING) {
      startPostStopRecovery();
      RCLCPP_INFO(
        get_logger(), "Brake released, starting post-stop recovery (%.2fm, max speed: %.2f m/s)",
        post_stop_recovery_distance_, post_stop_recovery_speed_);
    }
  }
  is_brake_active_ = msg->data;
}

void PurePursuitNode::slowdownCallback(const std_msgs::msg::Bool::SharedPtr msg)
{
  is_slowdown_ = msg->data;
}

void PurePursuitNode::pauseCallback(const std_msgs::msg::Empty::SharedPtr)
{
  if (!is_paused_) {
    RCLCPP_INFO(get_logger(), "Pause state activated");
    is_paused_ = true;
  }
}

void PurePursuitNode::resumeCallback(const std_msgs::msg::Empty::SharedPtr)
{
  if (is_paused_) {
    RCLCPP_INFO(get_logger(), "Resume state activated");
    is_paused_ = false;
  }
}

bool PurePursuitNode::isValidPath(const nav_msgs::msg::Path & path) const
{
  for (const auto & pose_stamped : path.poses) {
    const auto & p = pose_stamped.pose.position;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
      !std::isfinite(tf2::getYaw(pose_stamped.pose.orientation)))
    {
      return false;
    }
  }
  return true;
}

// ---------------------------------------------------------------------------
// 状態管理
// ---------------------------------------------------------------------------
bool PurePursuitNode::getCurrentPoseFromTF()
{
  geometry_msgs::msg::TransformStamped transform;
  try {
    transform = tf_buffer_.lookupTransform(map_frame_id_, robot_frame_id_, tf2::TimePointZero);
  } catch (const tf2::TransformException & ex) {
    RCLCPP_ERROR_THROTTLE(
      get_logger(), *get_clock(), 2000, "Could not transform %s to %s: %s",
      map_frame_id_.c_str(), robot_frame_id_.c_str(), ex.what());
    pose_received_ = false;
    return false;
  }
  geometry_msgs::msg::Pose pose;
  pose.position.x = transform.transform.translation.x;
  pose.position.y = transform.transform.translation.y;
  pose.position.z = transform.transform.translation.z;
  pose.orientation = transform.transform.rotation;
  if (!std::isfinite(pose.position.x) || !std::isfinite(pose.position.y) ||
    !std::isfinite(tf2::getYaw(pose.orientation)))
  {
    RCLCPP_ERROR_THROTTLE(
      get_logger(), *get_clock(), 2000,
      "TF pose contains NaN/Inf (%s -> %s). Reset pose via 2D Pose Estimate.",
      map_frame_id_.c_str(), robot_frame_id_.c_str());
    pose_received_ = false;
    return false;
  }
  current_pose_ = pose;
  pose_received_ = true;
  return true;
}

void PurePursuitNode::updateStateOnPath()
{
  if (current_path_.poses.empty()) {
    state_ = State::ARRIVED_GOAL;
    return;
  }

  const auto & goal = current_path_.poses.back().pose.position;
  const double dist_to_goal =
    std::hypot(goal.x - current_pose_.position.x, goal.y - current_pose_.position.y);
  // ゴールに近い経路は、最初の経路方向への位置合わせを省く
  if (dist_to_goal < goal_rotation_only_distance_threshold_) {
    state_ = isNearGoalYaw() ? State::ARRIVED_GOAL : State::ROTATE_AT_GOAL;
    return;
  }

  if (current_path_.poses.size() == 2) {
    const auto & p0 = current_path_.poses[0].pose.position;
    const auto & p1 = current_path_.poses[1].pose.position;
    if (std::hypot(p1.x - p0.x, p1.y - p0.y) < turn_in_place_threshold_) {
      state_ = State::ROTATE_AT_GOAL;
      return;
    }
  }
  if (dist_to_goal < goal_tolerance_dist_) {
    state_ = isNearGoalYaw() ? State::ARRIVED_GOAL : State::ROTATE_AT_GOAL;
  } else {
    state_ = State::ROTATE_AT_START;
  }
}

void PurePursuitNode::updateStateOnPose()
{
  switch (state_) {
    case State::ROTATE_AT_START: {
      if (!rotate_at_start_complete_ && isNearStartYaw()) {
        rotate_at_start_complete_ = true;
      }
      if (rotate_at_start_complete_) {
        if (isZeroTime(rotate_at_start_ready_time_)) {
          rotate_at_start_ready_time_ = now();
        }
        if (elapsedSince(rotate_at_start_ready_time_) >= transition_wait_time_) {
          state_ = State::FOLLOW_PATH;
          rotate_at_start_ready_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
        }
      } else {
        rotate_at_start_ready_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
      }
      break;
    }
    case State::FOLLOW_PATH: {
      if (isNearGoalXY()) {
        align_direction_sign_ = 0;
        align_force_goal_flag_ = false;
        state_ = State::ROTATE_AT_GOAL;
        // 遷移と同じ周期で待機ホールドを効かせるため、ここで ready_time を起動する。
        // 怠ると ROTATE_AT_GOAL の待機判定が素通りし、全力旋回が 1 周期漏れる。
        rotate_at_goal_ready_time_ = now();
        // 回頭開始前の静止確認をやり直す
        rotate_at_goal_standstill_confirmed_ = false;
        goal_standstill_since_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
      }
      if (enable_path_deviation_check_ &&
        calculatePathDeviationDistance() > path_deviation_threshold_)
      {
        state_ = State::PATH_DEVIATION_WARNING;
      }
      break;
    }
    case State::PATH_DEVIATION_WARNING: {
      const double deviation = calculatePathDeviationDistance();
      if (!enable_path_deviation_check_ || deviation <= path_deviation_recovery_threshold_) {
        RCLCPP_INFO(
          get_logger(),
          "Path deviation recovered (%.3fm), starting post-stop recovery (%.2fm, max speed: %.2f m/s)",
          deviation, post_stop_recovery_distance_, post_stop_recovery_speed_);
        startPostStopRecovery();
        state_ = State::FOLLOW_PATH;
      }
      break;
    }
    case State::ROTATE_AT_GOAL: {
      if (isZeroTime(rotate_at_goal_ready_time_)) {
        rotate_at_goal_ready_time_ = now();
      }
      if (elapsedSince(rotate_at_goal_ready_time_) < transition_wait_time_) {
        break;
      }
      if (!rotate_at_goal_complete_ && isNearGoalYaw()) {
        rotate_at_goal_complete_ = true;
      }
      if (rotate_at_goal_complete_) {
        state_ = State::ARRIVED_GOAL;
        rotate_at_goal_ready_time_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
      }
      break;
    }
    case State::ARRIVED_GOAL:
    case State::WAIT_PATH:
    default:
      break;
  }
}

bool PurePursuitNode::isAtStandstill() const
{
  // odom が古いときは静止を断定できない
  if (isZeroTime(last_odom_time_)) {return false;}
  if (elapsedSince(last_odom_time_) > odom_timeout_) {return false;}
  return std::abs(measured_linear_speed_) < goal_standstill_linear_thresh_ &&
         std::abs(measured_angular_speed_) < goal_standstill_angular_thresh_;
}

bool PurePursuitNode::isNearStartYaw()
{
  if (current_path_.poses.empty()) {return false;}
  const double diff =
    angles::shortest_angular_distance(getCurrentYaw(), getInitialTargetYaw());
  return std::abs(diff) < start_tolerance_ang_;
}

bool PurePursuitNode::isNearGoalXY()
{
  if (current_path_.poses.empty()) {return false;}
  const auto & current = current_pose_.position;
  const auto & goal = current_path_.poses.back().pose.position;
  const double remaining = getRemainingPathDistance();
  const double goal_dist = std::hypot(goal.x - current.x, goal.y - current.y);
  // 直線距離だけだと横偏差で到達判定できないため、経路残距離も併用する
  return remaining <= goal_tolerance_dist_ || goal_dist <= goal_tolerance_dist_;
}

bool PurePursuitNode::isNearGoalYaw()
{
  if (current_path_.poses.empty()) {return true;}
  const double diff = angles::shortest_angular_distance(getCurrentYaw(), getGoalYaw());
  if (std::abs(diff) < goal_tolerance_ang_) {return true;}
  return align_force_goal_flag_;
}

std::string PurePursuitNode::stateToString(State s)
{
  switch (s) {
    case State::WAIT_PATH: return "WAIT_PATH";
    case State::ROTATE_AT_START: return "ROTATE_AT_START";
    case State::FOLLOW_PATH: return "FOLLOW_PATH";
    case State::PATH_DEVIATION_WARNING: return "PATH_DEVIATION_WARNING";
    case State::ROTATE_AT_GOAL: return "ROTATE_AT_GOAL";
    case State::ARRIVED_GOAL: return "ARRIVED_GOAL";
  }
  return "UNKNOWN";
}

void PurePursuitNode::publishStatusTopics()
{
  std_msgs::msg::String mode_msg;
  mode_msg.data = stateToString(state_);
  mode_pub_->publish(mode_msg);

  std_msgs::msg::Bool following_msg, reached_msg;
  following_msg.data =
    (state_ == State::ROTATE_AT_START || state_ == State::FOLLOW_PATH ||
    state_ == State::ROTATE_AT_GOAL || state_ == State::PATH_DEVIATION_WARNING);
  reached_msg.data = (state_ == State::ARRIVED_GOAL);
  following_pub_->publish(following_msg);
  reached_pub_->publish(reached_msg);
}

// ---------------------------------------------------------------------------
// 制御ループ
// ---------------------------------------------------------------------------
void PurePursuitNode::publishCmd(const geometry_msgs::msg::Twist & cmd)
{
  if (!std::isfinite(cmd.linear.x) || !std::isfinite(cmd.angular.z)) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 2000,
      "Invalid cmd_vel (linear=%.3f, angular=%.3f), publishing stop instead",
      cmd.linear.x, cmd.angular.z);
    last_cmd_linear_x_ = 0.0;
    last_cmd_angular_z_ = 0.0;
    cmd_pub_->publish(geometry_msgs::msg::Twist());
    return;
  }
  cmd_pub_->publish(cmd);
}

void PurePursuitNode::controlLoop()
{
  getCurrentPoseFromTF();
  if (!pose_received_) {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 2000, "map->%s TF is unavailable; waiting for pose estimate",
      robot_frame_id_.c_str());
    publishStatusTopics();
    cmd_pub_->publish(geometry_msgs::msg::Twist());
    return;
  }

  updateStateOnPose();
  publishStatusTopics();

  geometry_msgs::msg::Twist cmd;
  if (shouldEmergencyBrake()) {
    // brake トピックのハートビート途絶: 即停止
    last_cmd_linear_x_ = 0.0;
    last_cmd_angular_z_ = 0.0;
    last_cmd_time_ = now();
    cmd_pub_->publish(cmd);
    return;
  }

  if (is_paused_) {
    cmd = applyAccelerationLimits(cmd);
  } else {
    switch (state_) {
      case State::ROTATE_AT_START:
        cmd = calcInitAlignCmd();
        break;
      case State::FOLLOW_PATH: {
        const bool use_slow_speed_lookahead =
          slow_speed_lookahead_threshold_ > 0.0 &&
          external_speed_limit_ <= slow_speed_lookahead_threshold_;
        pp_lookahead_distance_ =
          use_slow_speed_lookahead ? slow_speed_lookahead_distance_ : lookahead_distance_;
        cmd = calcPurePursuitCmd();
        if (getRemainingPathDistance() <= goal_tolerance_dist_) {
          cmd.linear.x = 0.0;
        }
        break;
      }
      case State::ROTATE_AT_GOAL: {
        // 回頭開始前に実際に停止したことを odom で確認する。直進終端の応答遅れで
        // 前進惰性が残ったまま回頭すると身をよじって見えるため。
        // 古い/無い odom で永久に止まらないようタイムアウトを設ける。
        if (!rotate_at_goal_standstill_confirmed_) {
          if (isAtStandstill()) {
            if (isZeroTime(goal_standstill_since_)) {
              goal_standstill_since_ = now();
            }
          } else {
            goal_standstill_since_ = rclcpp::Time(0, 0, RCL_ROS_TIME);
          }
          const bool confirmed =
            !isZeroTime(goal_standstill_since_) &&
            elapsedSince(goal_standstill_since_) >= goal_standstill_confirm_time_;
          const bool timed_out =
            !isZeroTime(rotate_at_goal_ready_time_) &&
            elapsedSince(rotate_at_goal_ready_time_) >= goal_standstill_timeout_;
          if (timed_out && !confirmed) {
            RCLCPP_WARN(
              get_logger(),
              "Goal-rotation standstill not confirmed within %.1fs (|v|=%.3f m/s, |w|=%.3f rad/s); "
              "starting rotation on timeout.",
              goal_standstill_timeout_, std::abs(measured_linear_speed_),
              std::abs(measured_angular_speed_));
          }
          if (confirmed || timed_out) {
            rotate_at_goal_standstill_confirmed_ = true;
          } else {
            cmd = geometry_msgs::msg::Twist();
            break;
          }
        }
        cmd = calcGoalAlignCmd();
        break;
      }
      case State::WAIT_PATH:
      case State::ARRIVED_GOAL:
      case State::PATH_DEVIATION_WARNING:
      default:
        break;
    }

    if (!(state_ == State::ROTATE_AT_START || state_ == State::ROTATE_AT_GOAL)) {
      const bool in_goal_approach =
        state_ == State::FOLLOW_PATH && getRemainingPathDistance() <= goal_approach_distance_;
      if (!in_goal_approach) {
        cmd = applyAccelerationLimits(cmd);
      } else {
        // ゴールアプローチ区間は計画した直線減速をそのまま使う。ただし注視点が
        // 下限に張り付いて曲率が跳ねるため、角速度のみレート制限する。
        cmd.angular.z = applyAngularAccelLimit(cmd.angular.z);
        last_cmd_linear_x_ = cmd.linear.x;
      }
    } else if (is_brake_active_) {
      // ブレーキ中の旋回は brake_deceleration_ で両軸を徐々に減速する
      cmd = applyAccelerationLimits(cmd);
    } else {
      // その場旋回は calcAlignCmd が最初から align_angular_vel_ を出すため、
      // 立ち上がりを旋回専用の角加速度で制限する
      cmd.angular.z = applyAngularAccelLimit(cmd.angular.z, true);
    }
  }
  if (!is_paused_) {
    applyCmdVelMinimumMagnitude(cmd, !is_brake_active_);
  }
  last_cmd_linear_x_ = cmd.linear.x;
  last_cmd_angular_z_ = cmd.angular.z;
  publishCmd(cmd);
}

// ---------------------------------------------------------------------------
// 角度
// ---------------------------------------------------------------------------
double PurePursuitNode::getCurrentYaw() const
{
  return tf2::getYaw(current_pose_.orientation);
}

double PurePursuitNode::getGoalYaw() const
{
  return tf2::getYaw(current_path_.poses.back().pose.orientation);
}

double PurePursuitNode::getInitialTargetYaw()
{
  const auto lookahead = getLookaheadPoint();
  const double dx = lookahead.x - current_pose_.position.x;
  const double dy = lookahead.y - current_pose_.position.y;
  // 注視点が近すぎる場合は現在の向きを使う
  if (std::hypot(dx, dy) < 0.01) {
    return getCurrentYaw();
  }
  return std::atan2(dy, dx);
}

// ---------------------------------------------------------------------------
// 旋回整合
// ---------------------------------------------------------------------------
geometry_msgs::msg::Twist PurePursuitNode::calcAlignCmd(
  double target_yaw, double tolerance, double angular_vel)
{
  geometry_msgs::msg::Twist cmd;
  if (is_brake_active_) {
    return cmd;
  }

  const double diff = angles::shortest_angular_distance(getCurrentYaw(), target_yaw);
  const double abs_diff = std::abs(diff);

  const int sign = (diff > 0) ? 1 : (diff < 0 ? -1 : 0);
  if (align_direction_sign_ == 0) {
    align_direction_sign_ = sign;
  }
  // 旋回方向が反転した (行き過ぎた) 場合はゴール扱いにして振動を防ぐ
  if (align_direction_sign_ != 0 && sign != 0 && sign != align_direction_sign_) {
    align_force_goal_flag_ = true;
  }
  if (abs_diff <= tolerance) {
    align_force_goal_flag_ = true;
  }

  if (!align_force_goal_flag_ && abs_diff > tolerance) {
    const double a = std::abs(align_angular_deceleration_);
    const double d_stop =
      (align_slowdown_angular_vel_ * align_slowdown_angular_vel_) / (2.0 * a);
    const double d_brake_to_slow =
      (angular_vel * angular_vel - align_slowdown_angular_vel_ * align_slowdown_angular_vel_) /
      (2.0 * a);

    double vel;
    if (abs_diff <= d_stop) {
      vel = std::sqrt(2.0 * a * abs_diff);
    } else if (abs_diff <= align_slowdown_angle_) {
      vel = align_slowdown_angular_vel_;
    } else if (abs_diff <= align_slowdown_angle_ + d_brake_to_slow) {
      vel = std::sqrt(
        align_slowdown_angular_vel_ * align_slowdown_angular_vel_ +
        2.0 * a * (abs_diff - align_slowdown_angle_));
      vel = std::min(vel, angular_vel);
    } else {
      vel = angular_vel;
    }
    cmd.angular.z = std::copysign(vel, align_direction_sign_ != 0 ? align_direction_sign_ : diff);
  }
  return cmd;
}

geometry_msgs::msg::Twist PurePursuitNode::calcInitAlignCmd()
{
  return calcAlignCmd(getInitialTargetYaw(), start_tolerance_ang_, align_angular_vel_);
}

geometry_msgs::msg::Twist PurePursuitNode::calcGoalAlignCmd()
{
  return calcAlignCmd(getGoalYaw(), goal_tolerance_ang_, align_angular_vel_);
}

// ---------------------------------------------------------------------------
// 経路追従 (PurePursuit)
// ---------------------------------------------------------------------------
void PurePursuitNode::computeArcLength()
{
  cumulative_arc_length_.clear();
  if (current_path_.poses.empty()) {return;}
  cumulative_arc_length_.push_back(0.0);
  for (size_t i = 1; i < current_path_.poses.size(); ++i) {
    const auto & p0 = current_path_.poses[i - 1].pose.position;
    const auto & p1 = current_path_.poses[i].pose.position;
    cumulative_arc_length_.push_back(
      cumulative_arc_length_.back() + std::hypot(p1.x - p0.x, p1.y - p0.y));
  }
}

bool PurePursuitNode::isGoalApproachEnabled() const
{
  return goal_approach_distance_ > 1e-6 && goal_approach_speed_ > 1e-6;
}

// 停止点までの残距離 decel_distance での計画速度。ゴールアプローチ有効時は
// 区間内を goal_approach_speed_ で保ち、外側では連続的に減速曲線へつなぐ:
//   v(d) = sqrt(v_creep^2 + 2*a*(d - d_creep))
double PurePursuitNode::computeDecelProfileSpeed(double decel_distance) const
{
  if (decel_distance <= 0.0) {return 0.0;}
  if (deceleration_ <= 1e-6) {return cruise_speed_;}
  if (isGoalApproachEnabled()) {
    if (decel_distance <= goal_approach_distance_) {
      return goal_approach_speed_;
    }
    const double v_creep_sq = goal_approach_speed_ * goal_approach_speed_;
    return std::sqrt(
      v_creep_sq + 2.0 * deceleration_ * (decel_distance - goal_approach_distance_));
  }
  return std::sqrt(2.0 * deceleration_ * decel_distance);
}

void PurePursuitNode::setPurePursuitPath()
{
  last_closest_index_ = 0;
  total_path_length_ = cumulative_arc_length_.empty() ? 0.0 : cumulative_arc_length_.back();
  const double path_length = total_path_length_;

  double v_max_from_path = 0.0;
  if (deceleration_ > 1e-6 && path_length > goal_tolerance_dist_) {
    v_max_from_path = computeDecelProfileSpeed(path_length - goal_tolerance_dist_);
  }

  if (v_max_from_path < cruise_speed_ && v_max_from_path > 1e-6) {
    // 短い経路: 巡航速度に届かないので最高速を落とす
    effective_max_speed_ = v_max_from_path;
    decel_start_distance_ = path_length;
  } else {
    effective_max_speed_ = cruise_speed_;
    double required_stopping_distance = 0.0;
    if (deceleration_ > 1e-6) {
      if (isGoalApproachEnabled()) {
        const double v_creep_sq = goal_approach_speed_ * goal_approach_speed_;
        const double v_cruise_sq = cruise_speed_ * cruise_speed_;
        required_stopping_distance = goal_approach_distance_ +
          std::max(0.0, v_cruise_sq - v_creep_sq) / (2.0 * deceleration_);
      } else {
        required_stopping_distance = (cruise_speed_ * cruise_speed_) / (2.0 * deceleration_);
      }
    }
    decel_start_distance_ = required_stopping_distance + goal_tolerance_dist_;
  }
}

geometry_msgs::msg::Pose PurePursuitNode::computePredictedPose() const
{
  if (sensor_delay_ <= 1e-6) {return current_pose_;}
  const double yaw = tf2::getYaw(current_pose_.orientation);
  geometry_msgs::msg::Pose predicted = current_pose_;
  predicted.position.x += pre_speed_ * std::cos(yaw) * sensor_delay_;
  predicted.position.y += pre_speed_ * std::sin(yaw) * sensor_delay_;
  tf2::Quaternion q;
  q.setRPY(0.0, 0.0, yaw + last_omega_ * sensor_delay_);
  predicted.orientation = tf2::toMsg(q);
  return predicted;
}

geometry_msgs::msg::Twist PurePursuitNode::calcPurePursuitCmd()
{
  geometry_msgs::msg::Twist cmd;
  if (current_path_.poses.empty()) {return cmd;}

  // センサ遅延補償した姿勢で制御する
  const geometry_msgs::msg::Pose control_pose = computePredictedPose();
  const geometry_msgs::msg::Point lookahead_point = getLookaheadPointImpl(control_pose);
  const double remaining_path_distance = getRemainingPathDistanceImpl(control_pose);

  double target_speed = 0.0;
  if (remaining_path_distance <= goal_tolerance_dist_) {
    target_speed = 0.0;
  } else if (remaining_path_distance <= decel_start_distance_ && deceleration_ > 1e-6) {
    const double decel_distance = remaining_path_distance - goal_tolerance_dist_;
    if (decel_distance > 0.0) {
      target_speed = std::min(effective_max_speed_, computeDecelProfileSpeed(decel_distance));
    }
  } else {
    target_speed = effective_max_speed_;
  }

  target_speed = std::min(target_speed, priority_speed_);

  if (is_brake_active_) {
    target_speed = 0.0;
  } else if (is_slowdown_) {
    const double slowdown_speed_cap = std::sqrt(
      2.0 * slowdown_deceleration_ * std::max(0.0, remaining_path_distance - goal_tolerance_dist_));
    target_speed = std::min(target_speed, slowdown_speed_cap);
  }

  // スタート直後の低速区間
  const double traveled_distance = total_path_length_ - remaining_path_distance;
  if (start_approach_distance_ > 0.0 && traveled_distance < start_approach_distance_) {
    target_speed = std::min(target_speed, start_approach_speed_);
  }

  cmd.linear.x = target_speed;

  if (remaining_path_distance > goal_tolerance_dist_) {
    const double dx = lookahead_point.x - control_pose.position.x;
    const double dy = lookahead_point.y - control_pose.position.y;
    const double actual_lookahead_dist = std::max(std::hypot(dx, dy), min_lookahead_distance_);

    if (actual_lookahead_dist > goal_tolerance_dist_) {
      const double yaw = tf2::getYaw(control_pose.orientation);
      const double local_y = -std::sin(yaw) * dx + std::cos(yaw) * dy;
      const double curvature = 2.0 * local_y / (actual_lookahead_dist * actual_lookahead_dist);

      if (acceleration_ > 1e-6) {
        constexpr double kMinCurvature = 1e-6;
        const double max_curvature = std::max(std::abs(curvature), kMinCurvature);
        const double v_max_curvature =
          std::sqrt(acceleration_ / max_curvature) * curvature_safety_factor_;
        cmd.linear.x = std::min(cmd.linear.x, v_max_curvature);
      }
      cmd.angular.z = cmd.linear.x * curvature;
    }
  }
  pre_speed_ = cmd.linear.x;
  last_omega_ = cmd.angular.z;
  publishLookaheadMarker(lookahead_point);
  return cmd;
}

geometry_msgs::msg::Point PurePursuitNode::getLookaheadPoint()
{
  return getLookaheadPointImpl(current_pose_);
}

geometry_msgs::msg::Point PurePursuitNode::getLookaheadPointImpl(
  const geometry_msgs::msg::Pose & pose)
{
  if (current_path_.poses.empty()) {return geometry_msgs::msg::Point();}

  size_t proj_seg_idx = last_closest_index_;  // 前回より後ろに下がらない
  double min_proj_dist = std::numeric_limits<double>::max();

  for (size_t i = last_closest_index_; i + 1 < current_path_.poses.size(); ++i) {
    const auto & p1 = current_path_.poses[i].pose.position;
    const auto & p2 = current_path_.poses[i + 1].pose.position;
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double seg_len2 = dx * dx + dy * dy;
    if (seg_len2 < 1e-6) {continue;}

    // 線分 p1-p2 への射影係数 t (0: p1, 1: p2)
    double t = ((pose.position.x - p1.x) * dx + (pose.position.y - p1.y) * dy) / seg_len2;
    t = std::clamp(t, 0.0, 1.0);
    const double dist =
      std::hypot(pose.position.x - (p1.x + t * dx), pose.position.y - (p1.y + t * dy));
    if (dist < min_proj_dist) {
      min_proj_dist = dist;
      proj_seg_idx = i;
    }
  }
  last_closest_index_ = proj_seg_idx;  // 後退禁止

  geometry_msgs::msg::Point lookahead_point = current_path_.poses.back().pose.position;

  for (size_t i = proj_seg_idx; i + 1 < current_path_.poses.size(); ++i) {
    const auto & p1 = current_path_.poses[i].pose.position;
    const auto & p2 = current_path_.poses[i + 1].pose.position;

    const double d1 = std::hypot(pose.position.x - p1.x, pose.position.y - p1.y);
    const double d2 = std::hypot(pose.position.x - p2.x, pose.position.y - p2.y);

    if (d2 >= pp_lookahead_distance_) {
      const double span = d2 - d1;
      double t = (std::abs(span) > 1e-6) ? (pp_lookahead_distance_ - d1) / span : 1.0;
      t = std::clamp(t, 0.0, 1.0);
      lookahead_point.x = p1.x + t * (p2.x - p1.x);
      lookahead_point.y = p1.y + t * (p2.y - p1.y);
      break;
    }
  }
  return lookahead_point;
}

double PurePursuitNode::getRemainingPathDistance() const
{
  return getRemainingPathDistanceImpl(current_pose_);
}

double PurePursuitNode::getRemainingPathDistanceImpl(const geometry_msgs::msg::Pose & pose) const
{
  if (current_path_.poses.empty()) {
    return std::numeric_limits<double>::infinity();
  }

  const auto & current = pose.position;
  const auto & goal = current_path_.poses.back().pose.position;
  if (current_path_.poses.size() == 1 || cumulative_arc_length_.empty()) {
    return std::hypot(goal.x - current.x, goal.y - current.y);
  }

  double min_dist = std::numeric_limits<double>::max();
  size_t closest_seg = 0;
  double closest_t = 0.0;
  bool found = false;

  for (size_t i = 0; i + 1 < current_path_.poses.size(); ++i) {
    const auto & p1 = current_path_.poses[i].pose.position;
    const auto & p2 = current_path_.poses[i + 1].pose.position;
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double seg_len2 = dx * dx + dy * dy;
    if (seg_len2 < 1e-6) {continue;}

    double t = ((current.x - p1.x) * dx + (current.y - p1.y) * dy) / seg_len2;
    t = std::clamp(t, 0.0, 1.0);
    const double dist = std::hypot(current.x - (p1.x + t * dx), current.y - (p1.y + t * dy));
    if (dist < min_dist) {
      min_dist = dist;
      closest_seg = i;
      closest_t = t;
      found = true;
    }
  }

  if (!found) {
    return std::hypot(goal.x - current.x, goal.y - current.y);
  }

  const double seg_length =
    cumulative_arc_length_[closest_seg + 1] - cumulative_arc_length_[closest_seg];
  const double s_current = cumulative_arc_length_[closest_seg] + closest_t * seg_length;
  return cumulative_arc_length_.back() - s_current;
}

double PurePursuitNode::calculatePathDeviationDistance() const
{
  if (current_path_.poses.empty()) {return 0.0;}

  const auto & current = current_pose_.position;
  if (current_path_.poses.size() == 1) {
    const auto & goal = current_path_.poses.back().pose.position;
    return std::hypot(goal.x - current.x, goal.y - current.y);
  }

  double min_dist = std::numeric_limits<double>::max();
  bool found = false;
  for (size_t i = 0; i + 1 < current_path_.poses.size(); ++i) {
    const auto & p1 = current_path_.poses[i].pose.position;
    const auto & p2 = current_path_.poses[i + 1].pose.position;
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;
    const double seg_len2 = dx * dx + dy * dy;
    if (seg_len2 < 1e-6) {continue;}

    double t = ((current.x - p1.x) * dx + (current.y - p1.y) * dy) / seg_len2;
    t = std::clamp(t, 0.0, 1.0);
    const double dist = std::hypot(current.x - (p1.x + t * dx), current.y - (p1.y + t * dy));
    if (dist < min_dist) {
      min_dist = dist;
      found = true;
    }
  }
  return found ? min_dist : 0.0;
}

// ---------------------------------------------------------------------------
// 出力整形
// ---------------------------------------------------------------------------
void PurePursuitNode::startPostStopRecovery()
{
  post_stop_recovery_start_dist_ = getRemainingPathDistance();
}

bool PurePursuitNode::isPostStopRecoveryActive() const
{
  if (post_stop_recovery_start_dist_ < 0.0) {return false;}
  const double traveled = post_stop_recovery_start_dist_ - getRemainingPathDistance();
  return traveled < post_stop_recovery_distance_;
}

bool PurePursuitNode::shouldEmergencyBrake() const
{
  // brake トピックのハートビートが途絶えたときのみ即停止する。
  // brake 要求自体は brake_deceleration_ で緩やかに減速する。
  if (isZeroTime(last_brake_msg_time_)) {return false;}
  return elapsedSince(last_brake_msg_time_) > brake_timeout_;
}

geometry_msgs::msg::Twist PurePursuitNode::applyAccelerationLimits(
  const geometry_msgs::msg::Twist & cmd)
{
  geometry_msgs::msg::Twist limited_cmd = cmd;

  const rclcpp::Time current_time = now();
  double dt = (current_time - last_cmd_time_).seconds();
  if (dt <= 0.0 || dt > 0.5) {dt = 0.05;}

  // 減速度は brake > slowdown > 通常 の順に選ぶ
  double effective_deceleration = deceleration_;
  if (is_brake_active_) {
    effective_deceleration = brake_deceleration_;
  } else if (is_slowdown_) {
    effective_deceleration = slowdown_deceleration_;
  }

  const double linear_vel_change = limited_cmd.linear.x - last_cmd_linear_x_;
  if (std::abs(linear_vel_change) > 1e-6) {
    const bool is_accelerating = linear_vel_change > 0.0;
    const double max_vel_change = (is_accelerating ? acceleration_ : effective_deceleration) * dt;
    if (std::abs(linear_vel_change) > max_vel_change) {
      limited_cmd.linear.x = last_cmd_linear_x_ + std::copysign(max_vel_change, linear_vel_change);
    }
  }

  const double desired_angular_vel = limited_cmd.angular.z;
  const double angular_vel_change = desired_angular_vel - last_cmd_angular_z_;
  if (std::abs(angular_vel_change) > 1e-6) {
    const bool is_accelerating = std::abs(desired_angular_vel) > std::abs(last_cmd_angular_z_);
    const double max_vel_change =
      (is_accelerating ? max_angular_acceleration_ : max_angular_deceleration_) * dt;
    if (std::abs(angular_vel_change) > max_vel_change) {
      limited_cmd.angular.z =
        last_cmd_angular_z_ + std::copysign(max_vel_change, angular_vel_change);
    }
  }
  if (std::abs(limited_cmd.angular.z) > max_angular_velocity_) {
    limited_cmd.angular.z = std::copysign(max_angular_velocity_, limited_cmd.angular.z);
  }
  if (is_slowdown_) {
    limited_cmd.linear.x = std::min(limited_cmd.linear.x, external_speed_limit_);
  }
  if (isPostStopRecoveryActive()) {
    const double distance_from_start = total_path_length_ - getRemainingPathDistance();
    // スタートアプローチ区間は除外
    if (distance_from_start >= start_approach_distance_) {
      limited_cmd.linear.x = std::min(limited_cmd.linear.x, post_stop_recovery_speed_);
    }
  }

  last_cmd_time_ = current_time;
  last_cmd_linear_x_ = limited_cmd.linear.x;
  last_cmd_angular_z_ = limited_cmd.angular.z;
  return limited_cmd;
}

double PurePursuitNode::applyAngularAccelLimit(double desired_angular_vel, bool in_place_rotation)
{
  const rclcpp::Time current_time = now();
  double dt = (current_time - last_cmd_time_).seconds();
  if (dt <= 0.0 || dt > 0.5) {dt = 0.05;}
  last_cmd_time_ = current_time;

  const double angular_vel_change = desired_angular_vel - last_cmd_angular_z_;
  if (std::abs(angular_vel_change) > 1e-6) {
    const bool is_accelerating = std::abs(desired_angular_vel) > std::abs(last_cmd_angular_z_);
    const double angular_acceleration =
      in_place_rotation ? align_angular_acceleration_ : max_angular_acceleration_;
    const double max_vel_change =
      (is_accelerating ? angular_acceleration : max_angular_deceleration_) * dt;
    if (std::abs(angular_vel_change) > max_vel_change) {
      desired_angular_vel = last_cmd_angular_z_ + std::copysign(max_vel_change, angular_vel_change);
    }
  }
  const double angular_velocity_limit =
    in_place_rotation ? align_angular_vel_ : max_angular_velocity_;
  if (std::abs(desired_angular_vel) > angular_velocity_limit) {
    desired_angular_vel = std::copysign(angular_velocity_limit, desired_angular_vel);
  }
  return desired_angular_vel;
}

void PurePursuitNode::applyCmdVelMinimumMagnitude(
  geometry_msgs::msg::Twist & cmd, bool enforce_minimum)
{
  clampCmdAxisToMinimum(cmd.linear.x, enforce_minimum ? min_cmd_linear_abs_ : 0.0);
  // 角速度の下限はその場旋回 (線速度 ~0) のときだけ。並進中に下限をかけると蛇行する。
  if (enforce_minimum && std::abs(cmd.linear.x) < kCmdVelDeadzone) {
    clampCmdAxisToMinimum(cmd.angular.z, min_cmd_angular_abs_);
  }
}

// ---------------------------------------------------------------------------
// 可視化
// ---------------------------------------------------------------------------
void PurePursuitNode::publishLookaheadMarker(const geometry_msgs::msg::Point & lookahead)
{
  visualization_msgs::msg::Marker marker;
  marker.header.stamp = now();
  marker.header.frame_id = map_frame_id_;
  marker.ns = "pure_pursuit";
  marker.id = 0;
  marker.type = visualization_msgs::msg::Marker::SPHERE;
  marker.action = visualization_msgs::msg::Marker::ADD;
  marker.pose.position = lookahead;
  marker.pose.orientation.w = 1.0;
  marker.scale.x = 0.2;
  marker.scale.y = 0.2;
  marker.scale.z = 0.2;
  marker.color.r = 1.0;
  marker.color.g = 0.0;
  marker.color.b = 0.0;
  marker.color.a = 1.0;
  marker.lifetime = rclcpp::Duration::from_seconds(0.2);

  visualization_msgs::msg::MarkerArray array;
  array.markers.push_back(marker);
  marker_pub_->publish(array);
}

void PurePursuitNode::publishApproachZoneMarkers()
{
  if (!publish_approach_zone_marker_) {return;}
  if (current_path_.poses.empty() || cumulative_arc_length_.empty()) {return;}

  const rclcpp::Time stamp = now();
  std::string frame_id = current_path_.header.frame_id;
  if (frame_id.empty()) {frame_id = map_frame_id_;}
  const double total_length = cumulative_arc_length_.back();

  constexpr double kZOffset = 0.05;  // パスとの重なり回避
  auto sample_point_at_s = [&](double s) {
      geometry_msgs::msg::Point p;
      s = std::clamp(s, 0.0, total_length);
      const auto it = std::lower_bound(cumulative_arc_length_.begin(), cumulative_arc_length_.end(), s);
      const size_t idx = std::distance(cumulative_arc_length_.begin(), it);
      if (idx == 0) {
        p = current_path_.poses.front().pose.position;
        p.z += kZOffset;
        return p;
      }
      if (idx >= current_path_.poses.size()) {
        p = current_path_.poses.back().pose.position;
        p.z += kZOffset;
        return p;
      }
      const double prev_s = cumulative_arc_length_[idx - 1];
      const double seg_len = cumulative_arc_length_[idx] - prev_s;
      const auto & p1 = current_path_.poses[idx - 1].pose.position;
      const auto & p2 = current_path_.poses[idx].pose.position;
      const double t = (seg_len > 1e-6) ? (s - prev_s) / seg_len : 0.0;
      p.x = p1.x + t * (p2.x - p1.x);
      p.y = p1.y + t * (p2.y - p1.y);
      p.z = p1.z + t * (p2.z - p1.z) + kZOffset;
      return p;
    };

  auto build_line_strip = [&](const std::string & ns, double s_start, double s_end,
      float r, float g, float b) {
      visualization_msgs::msg::Marker marker;
      marker.header.stamp = stamp;
      marker.header.frame_id = frame_id;
      marker.ns = ns;
      marker.id = 0;
      marker.type = visualization_msgs::msg::Marker::LINE_STRIP;
      marker.action = visualization_msgs::msg::Marker::ADD;
      marker.pose.orientation.w = 1.0;
      marker.scale.x = 0.04;
      marker.color.r = r;
      marker.color.g = g;
      marker.color.b = b;
      marker.color.a = 1.0;

      s_start = std::clamp(s_start, 0.0, total_length);
      s_end = std::clamp(s_end, 0.0, total_length);
      if (s_end <= s_start) {return marker;}

      marker.points.push_back(sample_point_at_s(s_start));
      for (size_t i = 0; i < cumulative_arc_length_.size(); ++i) {
        const double s = cumulative_arc_length_[i];
        if (s > s_start && s < s_end) {
          geometry_msgs::msg::Point p = current_path_.poses[i].pose.position;
          p.z += kZOffset;
          marker.points.push_back(p);
        }
      }
      marker.points.push_back(sample_point_at_s(s_end));
      return marker;
    };

  visualization_msgs::msg::MarkerArray array;
  array.markers.push_back(
    build_line_strip("start_approach_zone", 0.0, start_approach_distance_, 0.0f, 0.0f, 1.0f));
  if (goal_approach_distance_ > 1e-6) {
    const double s_creep_start = total_length - goal_tolerance_dist_ - goal_approach_distance_;
    array.markers.push_back(
      build_line_strip("goal_approach_zone", s_creep_start, total_length, 0.0f, 1.0f, 0.0f));
  }
  approach_zone_pub_->publish(array);
}
}  // namespace pure_pursuit_planner

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(pure_pursuit_planner::PurePursuitNode)
