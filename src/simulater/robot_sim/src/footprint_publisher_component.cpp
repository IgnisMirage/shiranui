#include "robot_sim/footprint_publisher_component.hpp"

#include <sstream>
#include <stdexcept>

namespace robot_sim
{
namespace
{
bool parseVertex(const std::string & text, float & x, float & y)
{
  std::string normalized = text;
  for (char & c : normalized) {
    if (c == ',') {
      c = ' ';
    }
  }

  std::istringstream iss(normalized);
  double xd = 0.0;
  double yd = 0.0;
  if (!(iss >> xd >> yd)) {
    return false;
  }

  x = static_cast<float>(xd);
  y = static_cast<float>(yd);
  return true;
}
}  // namespace

FootprintPublisherNode::FootprintPublisherNode(const rclcpp::NodeOptions & options)
: Node("footprint_publisher", options)
{
  frame_id_ = declare_parameter<std::string>("frame_id", "base_link");
  publish_rate_ = declare_parameter<double>("publish_rate", 1.0);
  vertices_ = declare_parameter<std::vector<std::string>>("vertices", std::vector<std::string>{});

  if (!loadFootprintFromParams()) {
    throw std::runtime_error("Failed to load footprint parameters");
  }

  footprint_publisher_ = create_publisher<geometry_msgs::msg::PolygonStamped>(
    "footprint", rclcpp::QoS(1).transient_local().reliable());

  const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
  publish_timer_ = create_wall_timer(
    std::chrono::duration_cast<std::chrono::nanoseconds>(period),
    std::bind(&FootprintPublisherNode::publishFootprint, this));

  publishFootprint();
  RCLCPP_INFO(get_logger(),
    "Footprint publisher started (frame=%s, vertices=%zu, rate=%.1f Hz)",
    frame_id_.c_str(), footprint_msg_.polygon.points.size(), publish_rate_);
}

bool FootprintPublisherNode::loadFootprintFromParams()
{
  footprint_msg_.polygon.points.clear();
  footprint_msg_.header.frame_id = frame_id_;

  if (vertices_.size() < 3) {
    RCLCPP_ERROR(get_logger(),
      "vertices must contain at least 3 points, got %zu", vertices_.size());
    return false;
  }

  for (size_t i = 0; i < vertices_.size(); ++i) {
    float x = 0.0f;
    float y = 0.0f;
    if (!parseVertex(vertices_[i], x, y)) {
      RCLCPP_ERROR(get_logger(),
        "vertex[%zu] must be \"x, y\" format, got \"%s\"", i, vertices_[i].c_str());
      return false;
    }

    geometry_msgs::msg::Point32 point;
    point.x = x;
    point.y = y;
    point.z = 0.0f;
    footprint_msg_.polygon.points.push_back(point);
  }

  return true;
}

void FootprintPublisherNode::publishFootprint()
{
  footprint_msg_.header.stamp = now();
  footprint_publisher_->publish(footprint_msg_);
}

}  // namespace robot_sim

#include <rclcpp_components/register_node_macro.hpp>
RCLCPP_COMPONENTS_REGISTER_NODE(robot_sim::FootprintPublisherNode)
