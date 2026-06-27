#include <memory>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>

#include "mid360/packet_codec.hpp"
#include "mid360_driver/msg/mid360_packet.hpp"

namespace mid360
{

class PacketToCloudNode : public rclcpp::Node
{
public:
  PacketToCloudNode() : Node("mid360_packet_to_cloud")
  {
    input_topic_ = declare_parameter<std::string>("input_topic", "packet");
    output_topic_ = declare_parameter<std::string>("output_topic", "points");

    cloud_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>(output_topic_, 10);
    packet_sub_ = create_subscription<mid360_driver::msg::Mid360Packet>(
        input_topic_, 10, [this](const mid360_driver::msg::Mid360Packet::SharedPtr msg) {
          cloud_pub_->publish(mid360PacketToPointCloud2(*msg));
        });

    RCLCPP_INFO(get_logger(), "%s -> %s", input_topic_.c_str(), output_topic_.c_str());
  }

private:
  std::string input_topic_;
  std::string output_topic_;
  rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr cloud_pub_;
  rclcpp::Subscription<mid360_driver::msg::Mid360Packet>::SharedPtr packet_sub_;
};

}  // namespace mid360

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<mid360::PacketToCloudNode>());
  rclcpp::shutdown();
  return 0;
}
