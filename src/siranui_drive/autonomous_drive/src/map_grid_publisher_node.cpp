// map.pcd の高さスライスを 2D 占有格子に投影して配信する（A* のコストマップ用）。
// 点が 1 つでも落ちたセルを占有 (100)、それ以外を自由 (0) とする。
#include <nav_msgs/msg/occupancy_grid.hpp>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <rclcpp/rclcpp.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>

namespace autonomous_drive
{

class MapGridPublisherNode : public rclcpp::Node
{
public:
  MapGridPublisherNode()
  : Node("map_grid_publisher")
  {
    const std::string map_pcd_path = declare_parameter<std::string>("map_pcd_path", "");
    frame_id_ = declare_parameter<std::string>("frame_id", "map");
    topic_name_ = declare_parameter<std::string>("topic_name", "map_grid");
    resolution_ = declare_parameter<double>("resolution", 0.05);
    // map 座標系で地面 z=0 とみなし、この帯に入る点を障害物にする
    min_height_ = declare_parameter<double>("min_height", 0.1);
    max_height_ = declare_parameter<double>("max_height", 1.0);
    // 地図の外側に自由領域として足す幅 [m]
    margin_ = declare_parameter<double>("margin", 2.0);
    publish_rate_ = declare_parameter<double>("publish_rate", 1.0);

    if (map_pcd_path.empty()) {
      throw std::runtime_error("map_pcd_path parameter is required");
    }
    if (resolution_ <= 0.0) {
      throw std::runtime_error("resolution must be positive");
    }

    pcl::PointCloud<pcl::PointXYZ> cloud;
    if (pcl::io::loadPCDFile(map_pcd_path, cloud) < 0 || cloud.empty()) {
      throw std::runtime_error("Failed to load PCD: " + map_pcd_path);
    }

    grid_ = buildGrid(cloud);

    publisher_ = create_publisher<nav_msgs::msg::OccupancyGrid>(
      topic_name_, rclcpp::QoS(1).transient_local().reliable());

    publishGrid();

    if (publish_rate_ > 0.0) {
      const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
      timer_ = create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(period),
        std::bind(&MapGridPublisherNode::publishGrid, this));
    }
  }

private:
  nav_msgs::msg::OccupancyGrid buildGrid(const pcl::PointCloud<pcl::PointXYZ> & cloud)
  {
    double min_x = std::numeric_limits<double>::max();
    double min_y = std::numeric_limits<double>::max();
    double max_x = std::numeric_limits<double>::lowest();
    double max_y = std::numeric_limits<double>::lowest();
    for (const auto & p : cloud.points) {
      min_x = std::min(min_x, static_cast<double>(p.x));
      min_y = std::min(min_y, static_cast<double>(p.y));
      max_x = std::max(max_x, static_cast<double>(p.x));
      max_y = std::max(max_y, static_cast<double>(p.y));
    }
    min_x -= margin_;
    min_y -= margin_;
    max_x += margin_;
    max_y += margin_;

    nav_msgs::msg::OccupancyGrid grid;
    grid.header.frame_id = frame_id_;
    grid.info.resolution = static_cast<float>(resolution_);
    grid.info.width = static_cast<uint32_t>(std::ceil((max_x - min_x) / resolution_));
    grid.info.height = static_cast<uint32_t>(std::ceil((max_y - min_y) / resolution_));
    grid.info.origin.position.x = min_x;
    grid.info.origin.position.y = min_y;
    grid.info.origin.orientation.w = 1.0;
    grid.data.assign(static_cast<size_t>(grid.info.width) * grid.info.height, 0);

    size_t occupied_points = 0;
    for (const auto & p : cloud.points) {
      if (p.z < min_height_ || p.z > max_height_) {
        continue;
      }
      const auto ix = static_cast<int64_t>(std::floor((p.x - min_x) / resolution_));
      const auto iy = static_cast<int64_t>(std::floor((p.y - min_y) / resolution_));
      if (ix < 0 || iy < 0 || ix >= static_cast<int64_t>(grid.info.width) ||
        iy >= static_cast<int64_t>(grid.info.height))
      {
        continue;
      }
      grid.data[static_cast<size_t>(iy) * grid.info.width + ix] = 100;
      ++occupied_points;
    }

    RCLCPP_INFO(
      get_logger(),
      "Built grid on '%s' from %zu points (%zu in z=[%.2f, %.2f]): %ux%u, resolution=%.3f, "
      "origin=(%.2f, %.2f)",
      topic_name_.c_str(), cloud.size(), occupied_points, min_height_, max_height_,
      grid.info.width, grid.info.height, resolution_, min_x, min_y);
    return grid;
  }

  void publishGrid()
  {
    grid_.header.stamp = now();
    publisher_->publish(grid_);
  }

  std::string frame_id_;
  std::string topic_name_;
  double resolution_ = 0.05;
  double min_height_ = 0.1;
  double max_height_ = 1.0;
  double margin_ = 2.0;
  double publish_rate_ = 1.0;
  nav_msgs::msg::OccupancyGrid grid_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace autonomous_drive

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<autonomous_drive::MapGridPublisherNode>());
  rclcpp::shutdown();
  return 0;
}
