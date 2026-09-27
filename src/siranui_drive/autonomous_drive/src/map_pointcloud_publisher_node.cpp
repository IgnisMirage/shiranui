#include <pcl/filters/voxel_grid.h>
#include <pcl/io/pcd_io.h>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl_conversions/pcl_conversions.h>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include <cmath>
#include <string>

namespace autonomous_drive
{

class MapPointcloudPublisherNode : public rclcpp::Node
{
public:
  MapPointcloudPublisherNode()
  : Node("map_pointcloud_publisher")
  {
    const std::string map_pcd_path = declare_parameter<std::string>("map_pcd_path", "");
    frame_id_ = declare_parameter<std::string>("frame_id", "map");
    topic_name_ = declare_parameter<std::string>("topic_name", "map");
    publish_rate_ = declare_parameter<double>("publish_rate", 1.0);
    min_height_ = declare_parameter<double>("min_height", 0.2);
    max_height_ = declare_parameter<double>("max_height", 2.0);
    crop_radius_ = declare_parameter<double>("crop_radius", 0.0);
    voxel_leaf_size_ = static_cast<float>(declare_parameter<double>("voxel_leaf_size", 0.3));

    if (map_pcd_path.empty()) {
      throw std::runtime_error("map_pcd_path parameter is required");
    }

    cloud_msg_ = loadAndFilter(map_pcd_path);
    cloud_msg_.header.frame_id = frame_id_;

    publisher_ = create_publisher<sensor_msgs::msg::PointCloud2>(
      topic_name_, rclcpp::QoS(1).transient_local().reliable());

    publishCloud();

    if (publish_rate_ > 0.0) {
      const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
      timer_ = create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(period),
        std::bind(&MapPointcloudPublisherNode::publishCloud, this));
    }

    RCLCPP_INFO(
      get_logger(),
      "Publishing map point cloud on '%s' (%u points, z=[%.2f, %.2f] m, frame=%s, voxel=%.2f m)",
      topic_name_.c_str(), cloud_msg_.width, min_height_, max_height_, frame_id_.c_str(),
      voxel_leaf_size_);
  }

private:
  sensor_msgs::msg::PointCloud2 loadAndFilter(const std::string & path)
  {
    pcl::PointCloud<pcl::PointXYZ>::Ptr raw(new pcl::PointCloud<pcl::PointXYZ>());
    if (pcl::io::loadPCDFile(path, *raw) < 0) {
      throw std::runtime_error("Failed to load map PCD: " + path);
    }

    auto filtered = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
    filtered->reserve(raw->size());
    const double crop_r2 = crop_radius_ > 0.0 ? crop_radius_ * crop_radius_ : 0.0;

    for (const auto & p : raw->points) {
      if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) {
        continue;
      }
      if (p.z < min_height_ || p.z > max_height_) {
        continue;
      }
      if (crop_radius_ > 0.0) {
        const double r2 = static_cast<double>(p.x) * p.x + static_cast<double>(p.y) * p.y;
        if (r2 > crop_r2) {
          continue;
        }
      }
      filtered->push_back(p);
    }

    if (filtered->empty()) {
      throw std::runtime_error(
        "No points left after height filter; check min_height/max_height and map PCD");
    }

    if (voxel_leaf_size_ > 0.0F) {
      pcl::VoxelGrid<pcl::PointXYZ> vg;
      vg.setLeafSize(voxel_leaf_size_, voxel_leaf_size_, voxel_leaf_size_);
      vg.setInputCloud(filtered);
      auto downsampled = std::make_shared<pcl::PointCloud<pcl::PointXYZ>>();
      vg.filter(*downsampled);
      filtered = downsampled;
    }

    sensor_msgs::msg::PointCloud2 msg;
    pcl::toROSMsg(*filtered, msg);
    return msg;
  }

  void publishCloud()
  {
    cloud_msg_.header.stamp = now();
    publisher_->publish(cloud_msg_);
  }

  std::string frame_id_;
  std::string topic_name_;
  double publish_rate_;
  double min_height_;
  double max_height_;
  double crop_radius_;
  float voxel_leaf_size_;

  sensor_msgs::msg::PointCloud2 cloud_msg_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace autonomous_drive

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<autonomous_drive::MapPointcloudPublisherNode>());
  rclcpp::shutdown();
  return 0;
}
