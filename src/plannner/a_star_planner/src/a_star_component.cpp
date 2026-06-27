#include "a_star_planner/a_star_component.hpp"
#include "a_star_planner/smoother.hpp"

#include <tf2/LinearMath/Quaternion.h>
#include <tf2_geometry_msgs/tf2_geometry_msgs.hpp>

#include <cmath>

using a_star_planner::buildPlanningCostmap;
using a_star_planner::footprintFromPolygon;

namespace
{
void setPathOrientations(
  nav_msgs::msg::Path & path,
  const geometry_msgs::msg::Quaternion & goal_orientation)
{
  if (path.poses.empty()) {
    return;
  }

  for (size_t i = 0; i + 1 < path.poses.size(); ++i) {
    const auto & p1 = path.poses[i].pose.position;
    const auto & p2 = path.poses[i + 1].pose.position;
    const double dx = p2.x - p1.x;
    const double dy = p2.y - p1.y;

    tf2::Quaternion q;
    if (std::hypot(dx, dy) > 1e-6) {
      q.setRPY(0.0, 0.0, std::atan2(dy, dx));
    } else if (i > 0) {
      path.poses[i].pose.orientation = path.poses[i - 1].pose.orientation;
      continue;
    } else {
      q.setRPY(0.0, 0.0, 0.0);
    }
    path.poses[i].pose.orientation = tf2::toMsg(q);
  }

  path.poses.back().pose.orientation = goal_orientation;
  tf2::Quaternion goal_q;
  tf2::fromMsg(path.poses.back().pose.orientation, goal_q);
  if (goal_q.length2() < 1e-12) {
    goal_q.setRPY(0.0, 0.0, 0.0);
  } else {
    goal_q.normalize();
  }
  path.poses.back().pose.orientation = tf2::toMsg(goal_q);
}
}  // namespace

AStarGlobalPlanner::AStarGlobalPlanner(const rclcpp::NodeOptions & node_options)
: Node("navyu_global_planner", node_options)
{
  declare_parameter<std::string>("map_frame", "map");
  declare_parameter<std::string>("base_frame", "base_link");
  declare_parameter<std::string>("map_topic", "map_with_obstacles");
  declare_parameter<std::string>("footprint_topic", "footprint");
  declare_parameter<double>("lethal_cost_threshold", 30.0);
  declare_parameter<double>("displacement_threshold", 5.0);
  declare_parameter<double>("clearance_distance", 0.3);
  declare_parameter<double>("cost_weight", 2.0);
  declare_parameter<int>("lethal_cost_value", 100);

  get_parameter("map_frame", map_frame_);
  get_parameter("base_frame", base_frame_);
  get_parameter("map_topic", map_topic_);
  get_parameter("footprint_topic", footprint_topic_);
  lethal_cost_threshold_ = static_cast<int8_t>(get_parameter("lethal_cost_threshold").as_double());
  lethal_cost_value_ = static_cast<int8_t>(get_parameter("lethal_cost_value").as_int());
  clearance_distance_ = get_parameter("clearance_distance").as_double();
  cost_weight_ = get_parameter("cost_weight").as_double();

  const double displacement_threshold = get_parameter("displacement_threshold").as_double();
  smoother_ = std::make_shared<Smoother>(displacement_threshold);

  broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(this);

  goal_pose_subscriber_ = create_subscription<geometry_msgs::msg::PoseStamped>(
    "goal_pose", 5,
    std::bind(&AStarGlobalPlanner::callback_goal_pose, this, std::placeholders::_1));
  costmap_subscriber_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
    map_topic_, rclcpp::QoS(10).transient_local().reliable().keep_last(1),
    std::bind(&AStarGlobalPlanner::callback_costmap, this, std::placeholders::_1));
  footprint_subscriber_ = create_subscription<geometry_msgs::msg::PolygonStamped>(
    footprint_topic_, rclcpp::QoS(1).transient_local().reliable(),
    std::bind(&AStarGlobalPlanner::callback_footprint, this, std::placeholders::_1));

  raw_path_publisher_ = create_publisher<nav_msgs::msg::Path>(
    "raw_path", rclcpp::QoS(10).transient_local().reliable().keep_last(1));
  path_publisher_ = create_publisher<nav_msgs::msg::Path>(
    "path", rclcpp::QoS(10).transient_local().reliable().keep_last(1));

  RCLCPP_INFO(
    get_logger(),
    "A* planner started: map_topic=%s, footprint_topic=%s, clearance=%.3f m, cost_weight=%.2f",
    map_topic_.c_str(), footprint_topic_.c_str(), clearance_distance_, cost_weight_);
}

AStarGlobalPlanner::~AStarGlobalPlanner()
{
}

void AStarGlobalPlanner::callback_goal_pose(const geometry_msgs::msg::PoseStamped & msg)
{
  if (planner_ == nullptr) {
    RCLCPP_ERROR_STREAM(get_logger(), "Planner is not Initialized.");
    return;
  }

  geometry_msgs::msg::Pose start;
  if (!get_robot_pose(map_frame_, base_frame_, tf_buffer_, start)) {
    RCLCPP_ERROR_STREAM(get_logger(), "Can not get Robot Pose.");
    return;
  }

  geometry_msgs::msg::Pose goal = msg.pose;

  std::vector<Node2D *> path;
  if (plan(start, goal, path)) {
    publish_path(path, msg.pose.orientation);
    RCLCPP_INFO(get_logger(), "Path published (%zu nodes)", path.size());
  } else {
    RCLCPP_WARN(get_logger(),
      "Failed to plan path from (%.2f, %.2f) to (%.2f, %.2f). "
      "Check goal is reachable and not inside obstacles.",
      start.position.x, start.position.y, goal.position.x, goal.position.y);
  }
}

void AStarGlobalPlanner::callback_costmap(const nav_msgs::msg::OccupancyGrid & msg)
{
  applyCostmap(msg);
}

void AStarGlobalPlanner::callback_footprint(
  const geometry_msgs::msg::PolygonStamped::SharedPtr msg)
{
  if (!msg || msg->polygon.points.size() < 3) {
    RCLCPP_WARN(get_logger(), "Received invalid footprint, ignoring");
    return;
  }

  footprint_ = footprintFromPolygon(msg->polygon);
  footprint_ready_ = true;
  RCLCPP_INFO(
    get_logger(), "Updated footprint from %s (%zu vertices)",
    footprint_topic_.c_str(), footprint_.size());

  if (has_latest_map_) {
    applyCostmap(latest_map_);
  }
}

void AStarGlobalPlanner::applyCostmap(const nav_msgs::msg::OccupancyGrid & msg)
{
  latest_map_ = msg;
  has_latest_map_ = true;

  nav_msgs::msg::OccupancyGrid planning_map = msg;
  if (footprint_ready_) {
    planning_map = buildPlanningCostmap(
      msg, footprint_, lethal_cost_threshold_, lethal_cost_value_, clearance_distance_);
  } else {
    RCLCPP_WARN_THROTTLE(
      get_logger(), *get_clock(), 5000,
      "Waiting for %s before applying footprint clearance costs",
      footprint_topic_.c_str());
    planning_map = buildPlanningCostmap(
      msg, {}, lethal_cost_threshold_, lethal_cost_value_, clearance_distance_);
  }

  if (planner_ == nullptr) {
    const int size_x = static_cast<int>(planning_map.info.width);
    const int size_y = static_cast<int>(planning_map.info.height);
    const double resolution = planning_map.info.resolution;
    const double origin_x = planning_map.info.origin.position.x;
    const double origin_y = planning_map.info.origin.position.y;

    planner_ = std::make_shared<AstarPlanner>(size_x, size_y, resolution, origin_x, origin_y);
    planner_->set_costmap(planning_map.data);
    planner_->set_lethal_cost_threshold(lethal_cost_threshold_);
    planner_->set_lethal_planning_cost(lethal_cost_value_);
    planner_->set_cost_weight(cost_weight_);
    return;
  }

  planner_->set_origin(
    planning_map.info.origin.position.x, planning_map.info.origin.position.y);
  planner_->set_size(planning_map.info.width, planning_map.info.height);
  planner_->set_resolution(planning_map.info.resolution);
  planner_->set_costmap(planning_map.data);
  planner_->set_lethal_planning_cost(lethal_cost_value_);
  planner_->set_cost_weight(cost_weight_);
}


void AStarGlobalPlanner::publish_path(
  std::vector<Node2D *> path,
  const geometry_msgs::msg::Quaternion & goal_orientation)
{
  nav_msgs::msg::Path path_msgs;
  path_msgs.header.frame_id = map_frame_;
  path_msgs.header.stamp = now();

  for (auto & node : path) {
    geometry_msgs::msg::PoseStamped path_pose;
    path_pose.header = path_msgs.header;
    planner_->convert_grid_to_map(
      node->x_, node->y_, path_pose.pose.position.x, path_pose.pose.position.y);
    path_msgs.poses.emplace_back(path_pose);
  }

  setPathOrientations(path_msgs, goal_orientation);
  raw_path_publisher_->publish(path_msgs);

  nav_msgs::msg::Path optimized_path = smoother_->smooth(path_msgs);
  optimized_path.header.frame_id = map_frame_;
  optimized_path.header.stamp = now();
  setPathOrientations(optimized_path, goal_orientation);
  path_publisher_->publish(optimized_path);
}

bool AStarGlobalPlanner::plan(
  const geometry_msgs::msg::Pose start, const geometry_msgs::msg::Pose goal,
  std::vector<Node2D *> & path)
{
  int start_x, start_y;
  int goal_x, goal_y;
  planner_->convert_map_to_grid(start.position.x, start.position.y, start_x, start_y);
  planner_->convert_map_to_grid(goal.position.x, goal.position.y, goal_x, goal_y);

  Node2D * start_node = new Node2D(start_x, start_y, 0.0, NULL);
  start_node->set_grid_index(planner_->get_grid_index(start_x, start_y));

  Node2D * goal_node = new Node2D(goal_x, goal_y, 0.0, NULL);
  goal_node->set_grid_index(planner_->get_grid_index(goal_x, goal_y));

  return planner_->plan(start_node, goal_node, path);
}


#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(AStarGlobalPlanner)
