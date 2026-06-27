#include "robot_sim/map_obstacle_layer_component.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace robot_sim
{
namespace
{
std::vector<std::string> tokenizeObstacleSpec(const std::string & text)
{
  std::string normalized = text;
  for (char & c : normalized) {
    if (c == ',') {
      c = ' ';
    }
  }

  std::istringstream iss(normalized);
  std::vector<std::string> tokens;
  std::string token;
  while (iss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

bool parseDouble(const std::string & text, double & value)
{
  try {
    size_t consumed = 0;
    value = std::stod(text, &consumed);
    return consumed == text.size();
  } catch (const std::exception &) {
    return false;
  }
}

bool parseStaticObstacle(const std::string & text, StaticObstacle & obstacle)
{
  const auto tokens = tokenizeObstacleSpec(text);
  if (tokens.size() != 4) {
    return false;
  }

  if (!parseDouble(tokens[0], obstacle.center_x) ||
    !parseDouble(tokens[1], obstacle.center_y) ||
    !parseDouble(tokens[2], obstacle.width) ||
    !parseDouble(tokens[3], obstacle.height))
  {
    return false;
  }

  return obstacle.width > 0.0 && obstacle.height > 0.0;
}

bool parsePatternToken(const std::string & token, MotionPattern & pattern)
{
  if (token == "linear" || token == "ping_pong" || token == "back_and_forth") {
    pattern = MotionPattern::Linear;
    return true;
  }
  if (token == "circular" || token == "circle") {
    pattern = MotionPattern::Circular;
    return true;
  }
  if (token == "loop" || token == "rect_loop" || token == "orbit") {
    pattern = MotionPattern::Loop;
    return true;
  }
  return false;
}

bool parseDynamicObstacle(const std::string & text, DynamicObstacle & obstacle)
{
  const auto tokens = tokenizeObstacleSpec(text);
  if (tokens.empty()) {
    return false;
  }

  size_t index = 0;
  if (parsePatternToken(tokens[0], obstacle.pattern)) {
    ++index;
  } else if (tokens.size() == 7) {
    obstacle.pattern = MotionPattern::Linear;
  } else {
    return false;
  }

  switch (obstacle.pattern) {
    case MotionPattern::Linear:
      if (tokens.size() - index != 7) {
        return false;
      }
      if (!parseDouble(tokens[index + 0], obstacle.start_x) ||
        !parseDouble(tokens[index + 1], obstacle.start_y) ||
        !parseDouble(tokens[index + 2], obstacle.end_x) ||
        !parseDouble(tokens[index + 3], obstacle.end_y) ||
        !parseDouble(tokens[index + 4], obstacle.width) ||
        !parseDouble(tokens[index + 5], obstacle.height) ||
        !parseDouble(tokens[index + 6], obstacle.speed))
      {
        return false;
      }
      obstacle.path_length = std::hypot(
        obstacle.end_x - obstacle.start_x, obstacle.end_y - obstacle.start_y);
      return obstacle.width > 0.0 && obstacle.height > 0.0 && obstacle.speed > 0.0 &&
             obstacle.path_length > 1e-6;

    case MotionPattern::Circular:
      if (tokens.size() - index != 6) {
        return false;
      }
      if (!parseDouble(tokens[index + 0], obstacle.center_x) ||
        !parseDouble(tokens[index + 1], obstacle.center_y) ||
        !parseDouble(tokens[index + 2], obstacle.radius) ||
        !parseDouble(tokens[index + 3], obstacle.width) ||
        !parseDouble(tokens[index + 4], obstacle.height) ||
        !parseDouble(tokens[index + 5], obstacle.speed))
      {
        return false;
      }
      return obstacle.radius > 0.0 && obstacle.width > 0.0 && obstacle.height > 0.0 &&
             obstacle.speed > 0.0;

    case MotionPattern::Loop:
      if (tokens.size() - index != 7) {
        return false;
      }
      if (!parseDouble(tokens[index + 0], obstacle.bounds_min_x) ||
        !parseDouble(tokens[index + 1], obstacle.bounds_min_y) ||
        !parseDouble(tokens[index + 2], obstacle.bounds_max_x) ||
        !parseDouble(tokens[index + 3], obstacle.bounds_max_y) ||
        !parseDouble(tokens[index + 4], obstacle.width) ||
        !parseDouble(tokens[index + 5], obstacle.height) ||
        !parseDouble(tokens[index + 6], obstacle.speed))
      {
        return false;
      }
      if (obstacle.bounds_max_x <= obstacle.bounds_min_x ||
        obstacle.bounds_max_y <= obstacle.bounds_min_y)
      {
        return false;
      }
      obstacle.loop_perimeter = 2.0 * (
        (obstacle.bounds_max_x - obstacle.bounds_min_x) +
        (obstacle.bounds_max_y - obstacle.bounds_min_y));
      return obstacle.width > 0.0 && obstacle.height > 0.0 && obstacle.speed > 0.0;
  }

  return false;
}

const char * patternName(MotionPattern pattern)
{
  switch (pattern) {
    case MotionPattern::Linear:
      return "linear";
    case MotionPattern::Circular:
      return "circular";
    case MotionPattern::Loop:
      return "loop";
  }
  return "unknown";
}

void getLoopCenter(
  const DynamicObstacle & obstacle, double traveled, double & center_x, double & center_y)
{
  const double half_w = obstacle.width * 0.5;
  const double half_h = obstacle.height * 0.5;
  const double min_x = obstacle.bounds_min_x + half_w;
  const double min_y = obstacle.bounds_min_y + half_h;
  const double max_x = obstacle.bounds_max_x - half_w;
  const double max_y = obstacle.bounds_max_y - half_h;
  const double span_x = max_x - min_x;
  const double span_y = max_y - min_y;
  const double perimeter = 2.0 * (span_x + span_y);

  if (perimeter <= 1e-6) {
    center_x = 0.5 * (min_x + max_x);
    center_y = 0.5 * (min_y + max_y);
    return;
  }

  const double s = std::fmod(traveled, perimeter);
  if (s <= span_x) {
    center_x = min_x + s;
    center_y = min_y;
  } else if (s <= span_x + span_y) {
    center_x = max_x;
    center_y = min_y + (s - span_x);
  } else if (s <= 2.0 * span_x + span_y) {
    center_x = max_x - (s - span_x - span_y);
    center_y = max_y;
  } else {
    center_x = min_x;
    center_y = max_y - (s - 2.0 * span_x - span_y);
  }
}
}  // namespace

MapObstacleLayerNode::MapObstacleLayerNode(const rclcpp::NodeOptions & options)
: Node("map_obstacle_layer", options)
{
  input_map_topic_ = declare_parameter<std::string>("input_map_topic", "map");
  output_map_topic_ = declare_parameter<std::string>("output_map_topic", "map_with_obstacles");
  update_rate_ = declare_parameter<double>("update_rate", 10.0);
  obstacle_cost_value_ = static_cast<int8_t>(
    declare_parameter<int>("obstacle_cost_value", 100));
  static_obstacle_params_ = declare_parameter<std::vector<std::string>>(
    "static_obstacles", std::vector<std::string>{});
  dynamic_obstacle_params_ = declare_parameter<std::vector<std::string>>(
    "dynamic_obstacles", std::vector<std::string>{});

  if (!loadStaticObstaclesFromParams() || !loadDynamicObstaclesFromParams()) {
    throw std::runtime_error("Failed to load obstacle parameters");
  }

  start_time_ = now();

  map_subscriber_ = create_subscription<nav_msgs::msg::OccupancyGrid>(
    input_map_topic_, rclcpp::QoS(10).transient_local().reliable().keep_last(1),
    std::bind(&MapObstacleLayerNode::onMapReceived, this, std::placeholders::_1));

  map_publisher_ = create_publisher<nav_msgs::msg::OccupancyGrid>(
    output_map_topic_, rclcpp::QoS(10).transient_local().reliable().keep_last(1));

  if (update_rate_ > 0.0) {
    const auto period = std::chrono::duration<double>(1.0 / update_rate_);
    update_timer_ = create_wall_timer(
      std::chrono::duration_cast<std::chrono::nanoseconds>(period),
      std::bind(&MapObstacleLayerNode::onUpdateTimer, this));
  }

  RCLCPP_INFO(
    get_logger(),
    "map_obstacle_layer started: %s -> %s, static=%zu, dynamic=%zu, cost=%d, rate=%.1f Hz",
    input_map_topic_.c_str(), output_map_topic_.c_str(),
    static_obstacles_.size(), dynamic_obstacles_.size(),
    static_cast<int>(obstacle_cost_value_), update_rate_);
}

bool MapObstacleLayerNode::loadStaticObstaclesFromParams()
{
  static_obstacles_.clear();
  static_obstacles_.reserve(static_obstacle_params_.size());

  for (size_t i = 0; i < static_obstacle_params_.size(); ++i) {
    StaticObstacle obstacle{};
    if (!parseStaticObstacle(static_obstacle_params_[i], obstacle)) {
      RCLCPP_ERROR(
        get_logger(),
        "static_obstacles[%zu] must be \"center_x, center_y, width, height\", got \"%s\"",
        i, static_obstacle_params_[i].c_str());
      return false;
    }
    static_obstacles_.push_back(obstacle);
  }

  return true;
}

bool MapObstacleLayerNode::loadDynamicObstaclesFromParams()
{
  dynamic_obstacles_.clear();
  dynamic_obstacles_.reserve(dynamic_obstacle_params_.size());

  for (size_t i = 0; i < dynamic_obstacle_params_.size(); ++i) {
    DynamicObstacle obstacle{};
    if (!parseDynamicObstacle(dynamic_obstacle_params_[i], obstacle)) {
      RCLCPP_ERROR(
        get_logger(),
        "dynamic_obstacles[%zu] format invalid: \"%s\". "
        "Use linear/circular/loop pattern (see config comments).",
        i, dynamic_obstacle_params_[i].c_str());
      return false;
    }
    dynamic_obstacles_.push_back(obstacle);
    RCLCPP_INFO(
      get_logger(), "Loaded dynamic obstacle[%zu]: pattern=%s",
      i, patternName(obstacle.pattern));
  }

  return true;
}

void MapObstacleLayerNode::updateMapBounds(const nav_msgs::msg::OccupancyGrid & map)
{
  map_bounds_.min_x = map.info.origin.position.x;
  map_bounds_.min_y = map.info.origin.position.y;
  map_bounds_.max_x = map.info.origin.position.x + map.info.width * map.info.resolution;
  map_bounds_.max_y = map.info.origin.position.y + map.info.height * map.info.resolution;
  map_bounds_.valid = map.info.width > 0 && map.info.height > 0;
}

void MapObstacleLayerNode::updateFreeSpaceBounds(const nav_msgs::msg::OccupancyGrid & map)
{
  free_space_bounds_.valid = false;
  if (map.data.empty() || map.info.width == 0 || map.info.height == 0) {
    return;
  }

  const int width = static_cast<int>(map.info.width);
  const int height = static_cast<int>(map.info.height);
  const double resolution = map.info.resolution;
  const double origin_x = map.info.origin.position.x;
  const double origin_y = map.info.origin.position.y;

  bool found = false;
  double min_x = 0.0;
  double max_x = 0.0;
  double min_y = 0.0;
  double max_y = 0.0;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t index = static_cast<size_t>(y * width + x);
      if (map.data[index] != 0) {
        continue;
      }

      const double world_x = origin_x + (static_cast<double>(x) + 0.5) * resolution;
      const double world_y = origin_y + (static_cast<double>(y) + 0.5) * resolution;
      if (!found) {
        min_x = max_x = world_x;
        min_y = max_y = world_y;
        found = true;
      } else {
        min_x = std::min(min_x, world_x);
        max_x = std::max(max_x, world_x);
        min_y = std::min(min_y, world_y);
        max_y = std::max(max_y, world_y);
      }
    }
  }

  free_space_bounds_.min_x = min_x;
  free_space_bounds_.max_x = max_x;
  free_space_bounds_.min_y = min_y;
  free_space_bounds_.max_y = max_y;
  free_space_bounds_.valid = found;
  has_free_space_bounds_ = found;
}

bool MapObstacleLayerNode::snapCenterToFreeSpace(double & center_x, double & center_y) const
{
  if (!has_base_map_ || base_map_.data.empty()) {
    return false;
  }

  const int width = static_cast<int>(base_map_.info.width);
  const int height = static_cast<int>(base_map_.info.height);
  const double resolution = base_map_.info.resolution;
  const double origin_x = base_map_.info.origin.position.x;
  const double origin_y = base_map_.info.origin.position.y;

  const int start_x = static_cast<int>(std::floor((center_x - origin_x) / resolution));
  const int start_y = static_cast<int>(std::floor((center_y - origin_y) / resolution));

  auto isFreeCell = [&](int gx, int gy) -> bool {
    if (gx < 0 || gy < 0 || gx >= width || gy >= height) {
      return false;
    }
    return base_map_.data[static_cast<size_t>(gy * width + gx)] == 0;
  };

  if (isFreeCell(start_x, start_y)) {
    center_x = origin_x + (static_cast<double>(start_x) + 0.5) * resolution;
    center_y = origin_y + (static_cast<double>(start_y) + 0.5) * resolution;
    return true;
  }

  const int max_radius = std::max(width, height);
  for (int radius = 1; radius <= max_radius; ++radius) {
    for (int dy = -radius; dy <= radius; ++dy) {
      for (int dx = -radius; dx <= radius; ++dx) {
        if (std::max(std::abs(dx), std::abs(dy)) != radius) {
          continue;
        }
        const int gx = start_x + dx;
        const int gy = start_y + dy;
        if (!isFreeCell(gx, gy)) {
          continue;
        }
        center_x = origin_x + (static_cast<double>(gx) + 0.5) * resolution;
        center_y = origin_y + (static_cast<double>(gy) + 0.5) * resolution;
        return true;
      }
    }
  }

  return false;
}

void MapObstacleLayerNode::clampCenterToMap(
  double & center_x, double & center_y, double width, double height) const
{
  const MapBounds * bounds = map_bounds_.valid ? &map_bounds_ : nullptr;
  if (has_free_space_bounds_) {
    bounds = &free_space_bounds_;
  }
  if (bounds == nullptr || !bounds->valid) {
    return;
  }

  const double half_w = width * 0.5;
  const double half_h = height * 0.5;
  const double min_center_x = bounds->min_x + half_w;
  const double max_center_x = bounds->max_x - half_w;
  const double min_center_y = bounds->min_y + half_h;
  const double max_center_y = bounds->max_y - half_h;

  if (min_center_x > max_center_x || min_center_y > max_center_y) {
    center_x = 0.5 * (bounds->min_x + bounds->max_x);
    center_y = 0.5 * (bounds->min_y + bounds->max_y);
    return;
  }

  center_x = std::clamp(center_x, min_center_x, max_center_x);
  center_y = std::clamp(center_y, min_center_y, max_center_y);
}

void MapObstacleLayerNode::clampObstaclesToMap()
{
  if (!map_bounds_.valid) {
    return;
  }

  for (auto & obstacle : static_obstacles_) {
    const double before_x = obstacle.center_x;
    const double before_y = obstacle.center_y;
    clampCenterToMap(obstacle.center_x, obstacle.center_y, obstacle.width, obstacle.height);
    if (!snapCenterToFreeSpace(obstacle.center_x, obstacle.center_y)) {
      RCLCPP_WARN(
        get_logger(),
        "Static obstacle near (%.2f, %.2f) could not be snapped to free space",
        before_x, before_y);
    } else if (
      std::abs(before_x - obstacle.center_x) > 1e-3 ||
      std::abs(before_y - obstacle.center_y) > 1e-3)
    {
      RCLCPP_INFO(
        get_logger(),
        "Snapped static obstacle center from (%.2f, %.2f) to (%.2f, %.2f)",
        before_x, before_y, obstacle.center_x, obstacle.center_y);
    }
  }

  for (auto & obstacle : dynamic_obstacles_) {
    switch (obstacle.pattern) {
      case MotionPattern::Linear: {
        clampCenterToMap(obstacle.start_x, obstacle.start_y, obstacle.width, obstacle.height);
        clampCenterToMap(obstacle.end_x, obstacle.end_y, obstacle.width, obstacle.height);
        snapCenterToFreeSpace(obstacle.start_x, obstacle.start_y);
        snapCenterToFreeSpace(obstacle.end_x, obstacle.end_y);
        obstacle.path_length = std::hypot(
          obstacle.end_x - obstacle.start_x, obstacle.end_y - obstacle.start_y);
        if (obstacle.path_length < 1e-6) {
          RCLCPP_WARN(get_logger(), "Linear dynamic obstacle collapsed after clamping");
        }
        break;
      }
      case MotionPattern::Circular: {
        clampCenterToMap(obstacle.center_x, obstacle.center_y, obstacle.width, obstacle.height);
        snapCenterToFreeSpace(obstacle.center_x, obstacle.center_y);
        if (has_free_space_bounds_) {
          const double half_diag = 0.5 * std::hypot(obstacle.width, obstacle.height);
          const double max_radius_x = std::min(
            obstacle.center_x - free_space_bounds_.min_x,
            free_space_bounds_.max_x - obstacle.center_x) - half_diag;
          const double max_radius_y = std::min(
            obstacle.center_y - free_space_bounds_.min_y,
            free_space_bounds_.max_y - obstacle.center_y) - half_diag;
          const double max_radius = std::max(0.0, std::min(max_radius_x, max_radius_y));
          if (obstacle.radius > max_radius) {
            RCLCPP_WARN(
              get_logger(),
              "Reduced circular obstacle radius from %.2f to %.2f for free space",
              obstacle.radius, max_radius);
            obstacle.radius = max_radius;
          }
        }
        break;
      }
      case MotionPattern::Loop: {
        const double half_w = obstacle.width * 0.5;
        const double half_h = obstacle.height * 0.5;
        const MapBounds & bounds = has_free_space_bounds_ ? free_space_bounds_ : map_bounds_;
        if (!bounds.valid) {
          break;
        }

        const double min_allowed_x = bounds.min_x + half_w;
        const double min_allowed_y = bounds.min_y + half_h;
        const double max_allowed_x = bounds.max_x - half_w;
        const double max_allowed_y = bounds.max_y - half_h;

        obstacle.bounds_min_x = std::clamp(obstacle.bounds_min_x, min_allowed_x, max_allowed_x);
        obstacle.bounds_min_y = std::clamp(obstacle.bounds_min_y, min_allowed_y, max_allowed_y);
        obstacle.bounds_max_x = std::clamp(obstacle.bounds_max_x, min_allowed_x, max_allowed_x);
        obstacle.bounds_max_y = std::clamp(obstacle.bounds_max_y, min_allowed_y, max_allowed_y);

        if (obstacle.bounds_max_x - obstacle.bounds_min_x < obstacle.width ||
          obstacle.bounds_max_y - obstacle.bounds_min_y < obstacle.height)
        {
          const double center_x = 0.5 * (obstacle.bounds_min_x + obstacle.bounds_max_x);
          const double center_y = 0.5 * (obstacle.bounds_min_y + obstacle.bounds_max_y);
          obstacle.bounds_min_x = center_x - obstacle.width * 0.5;
          obstacle.bounds_max_x = center_x + obstacle.width * 0.5;
          obstacle.bounds_min_y = center_y - obstacle.height * 0.5;
          obstacle.bounds_max_y = center_y + obstacle.height * 0.5;
        }

        obstacle.loop_perimeter = 2.0 * (
          (obstacle.bounds_max_x - obstacle.bounds_min_x) +
          (obstacle.bounds_max_y - obstacle.bounds_min_y));
        break;
      }
    }
  }

  if (map_bounds_.valid) {
    RCLCPP_INFO(
      get_logger(),
      "Map bounds: x=[%.2f, %.2f], y=[%.2f, %.2f]",
      map_bounds_.min_x, map_bounds_.max_x, map_bounds_.min_y, map_bounds_.max_y);
  }
  if (has_free_space_bounds_) {
    RCLCPP_INFO(
      get_logger(),
      "Free space bounds: x=[%.2f, %.2f], y=[%.2f, %.2f]",
      free_space_bounds_.min_x, free_space_bounds_.max_x,
      free_space_bounds_.min_y, free_space_bounds_.max_y);
  }
}

void MapObstacleLayerNode::paintRect(
  nav_msgs::msg::OccupancyGrid & map,
  double center_x, double center_y, double width, double height) const
{
  if (!map_bounds_.valid) {
    return;
  }

  clampCenterToMap(center_x, center_y, width, height);

  const double resolution = map.info.resolution;
  const int grid_width = static_cast<int>(map.info.width);
  const int grid_height = static_cast<int>(map.info.height);
  const double origin_x = map.info.origin.position.x;
  const double origin_y = map.info.origin.position.y;

  const double half_w = width * 0.5;
  const double half_h = height * 0.5;
  const double x_min = center_x - half_w;
  const double x_max = center_x + half_w;
  const double y_min = center_y - half_h;
  const double y_max = center_y + half_h;

  const int mx_min = static_cast<int>(std::floor((x_min - origin_x) / resolution));
  const int mx_max = static_cast<int>(std::floor((x_max - origin_x) / resolution));
  const int my_min = static_cast<int>(std::floor((y_min - origin_y) / resolution));
  const int my_max = static_cast<int>(std::floor((y_max - origin_y) / resolution));

  for (int my = my_min; my <= my_max; ++my) {
    if (my < 0 || my >= grid_height) {
      continue;
    }
    for (int mx = mx_min; mx <= mx_max; ++mx) {
      if (mx < 0 || mx >= grid_width) {
        continue;
      }
      map.data[static_cast<size_t>(my * grid_width + mx)] = obstacle_cost_value_;
    }
  }
}

void MapObstacleLayerNode::getDynamicObstacleCenter(
  const DynamicObstacle & obstacle, double & center_x, double & center_y) const
{
  const double elapsed = (now() - start_time_).seconds();

  switch (obstacle.pattern) {
    case MotionPattern::Linear: {
      const double cycle_length = obstacle.path_length * 2.0;
      const double traveled = std::fmod(elapsed * obstacle.speed, cycle_length);
      const double dist_along = traveled <= obstacle.path_length ?
        traveled : (cycle_length - traveled);
      const double ratio = dist_along / obstacle.path_length;
      center_x = obstacle.start_x + ratio * (obstacle.end_x - obstacle.start_x);
      center_y = obstacle.start_y + ratio * (obstacle.end_y - obstacle.start_y);
      break;
    }
    case MotionPattern::Circular: {
      if (obstacle.radius <= 1e-6) {
        center_x = obstacle.center_x;
        center_y = obstacle.center_y;
        break;
      }
      const double angle = (elapsed * obstacle.speed) / obstacle.radius;
      center_x = obstacle.center_x + obstacle.radius * std::cos(angle);
      center_y = obstacle.center_y + obstacle.radius * std::sin(angle);
      break;
    }
    case MotionPattern::Loop: {
      getLoopCenter(obstacle, elapsed * obstacle.speed, center_x, center_y);
      break;
    }
  }

  clampCenterToMap(center_x, center_y, obstacle.width, obstacle.height);
}

void MapObstacleLayerNode::publishMergedMap()
{
  if (!has_base_map_) {
    return;
  }

  nav_msgs::msg::OccupancyGrid merged = base_map_;
  for (const auto & obstacle : static_obstacles_) {
    paintRect(merged, obstacle.center_x, obstacle.center_y, obstacle.width, obstacle.height);
  }

  for (const auto & obstacle : dynamic_obstacles_) {
    double center_x = 0.0;
    double center_y = 0.0;
    getDynamicObstacleCenter(obstacle, center_x, center_y);
    paintRect(merged, center_x, center_y, obstacle.width, obstacle.height);
  }

  merged.header.stamp = now();
  map_publisher_->publish(merged);
}

void MapObstacleLayerNode::onMapReceived(const nav_msgs::msg::OccupancyGrid::SharedPtr msg)
{
  if (!msg) {
    return;
  }

  base_map_ = *msg;
  has_base_map_ = true;
  updateMapBounds(base_map_);
  updateFreeSpaceBounds(base_map_);
  clampObstaclesToMap();
  publishMergedMap();

  RCLCPP_INFO(
    get_logger(),
    "Base map received (%ux%u, resolution=%.3f). Publishing merged map on %s",
    base_map_.info.width, base_map_.info.height, base_map_.info.resolution,
    output_map_topic_.c_str());
}

void MapObstacleLayerNode::onUpdateTimer()
{
  publishMergedMap();
}

}  // namespace robot_sim

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(robot_sim::MapObstacleLayerNode)
