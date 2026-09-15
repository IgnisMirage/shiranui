#include <rclcpp/rclcpp.hpp>
#include <tf2_ros/static_transform_broadcaster.h>
#include <geometry_msgs/msg/transform_stamped.hpp>
#include <fstream>
#include <yaml-cpp/yaml.h>

class TfPublisherNode : public rclcpp::Node {
public:
  TfPublisherNode() : Node("tf_publisher") {
    declare_parameter("transforms_file", "");
    std::string transforms_file = get_parameter("transforms_file").as_string();

    if (transforms_file.empty()) {
      RCLCPP_ERROR(get_logger(), "transforms_file parameter not set");
      return;
    }

    auto broadcaster = std::make_shared<tf2_ros::StaticTransformBroadcaster>(this);
    std::vector<geometry_msgs::msg::TransformStamped> transforms;

    try {
      YAML::Node config = YAML::LoadFile(transforms_file);

      if (config["transforms"]) {
        for (const auto& transform_node : config["transforms"]) {
          geometry_msgs::msg::TransformStamped tf;

          tf.header.stamp = now();
          tf.header.frame_id = transform_node["parent_frame"].as<std::string>();
          tf.child_frame_id = transform_node["child_frame"].as<std::string>();

          auto& translation = tf.transform.translation;
          translation.x = transform_node["translation"]["x"].as<double>(0.0);
          translation.y = transform_node["translation"]["y"].as<double>(0.0);
          translation.z = transform_node["translation"]["z"].as<double>(0.0);

          auto& rotation = tf.transform.rotation;
          if (transform_node["rotation"]["type"].as<std::string>() == "euler") {
            double roll = transform_node["rotation"]["roll"].as<double>(0.0);
            double pitch = transform_node["rotation"]["pitch"].as<double>(0.0);
            double yaw = transform_node["rotation"]["yaw"].as<double>(0.0);

            double cy = cos(yaw * 0.5);
            double sy = sin(yaw * 0.5);
            double cp = cos(pitch * 0.5);
            double sp = sin(pitch * 0.5);
            double cr = cos(roll * 0.5);
            double sr = sin(roll * 0.5);

            rotation.w = cr * cp * cy + sr * sp * sy;
            rotation.x = sr * cp * cy - cr * sp * sy;
            rotation.y = cr * sp * cy + sr * cp * sy;
            rotation.z = cr * cp * sy - sr * sp * cy;
          } else {
            rotation.x = transform_node["rotation"]["x"].as<double>(0.0);
            rotation.y = transform_node["rotation"]["y"].as<double>(0.0);
            rotation.z = transform_node["rotation"]["z"].as<double>(0.0);
            rotation.w = transform_node["rotation"]["w"].as<double>(1.0);
          }

          transforms.push_back(tf);
          RCLCPP_INFO(get_logger(), "Loaded transform: %s -> %s",
                      tf.header.frame_id.c_str(), tf.child_frame_id.c_str());
        }
      }

      broadcaster->sendTransform(transforms);
      RCLCPP_INFO(get_logger(), "Published %zu transforms", transforms.size());

    } catch (const std::exception& e) {
      RCLCPP_ERROR(get_logger(), "Error loading transforms: %s", e.what());
    }
  }
};

int main(int argc, char* argv[]) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<TfPublisherNode>();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}
