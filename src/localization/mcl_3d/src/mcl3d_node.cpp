#include "mcl_3d/mcl_node.hpp"
#include <common.h>
#include <mcl3d_ros/DistanceField.h>

MCL3DNode::MCL3DNode(const rclcpp::NodeOptions &options):
    Node("mcl_3d_node", options)
{
    declare_parameter("pcd_file",           "");
    declare_parameter("map_file_name",      "dist_map.bin");
    declare_parameter("yaml_file_path",     "/tmp/dist_map.yaml");
    declare_parameter("resolution",         5.0);
    declare_parameter("sub_map_resolution", 0.1);
    declare_parameter("map_margin",         1.0);
    declare_parameter("voxel_leaf_size",    0.1);

    declare_parameter("use_initial_pose", false);
    declare_parameter("use_imu", false);
    declare_parameter("use_odom", false);

    point_cloud_sub_ = create_subscription<sensor_msgs::msg::PointCloud2>(
        "pointcloud", 10, std::bind(&MCL3DNode::pointCloudCallback, this, std::placeholders::_1));

    mcl3d::DistanceFieldParams dfParams;
    dfParams.pcdFile          = get_parameter("pcd_file").as_string();
    dfParams.mapFileName      = get_parameter("map_file_name").as_string();
    dfParams.yamlFilePath     = get_parameter("yaml_file_path").as_string();
    dfParams.resolution       = (float)get_parameter("resolution").as_double();
    dfParams.subMapResolution = (float)get_parameter("sub_map_resolution").as_double();
    dfParams.mapMargin        = (float)get_parameter("map_margin").as_double();
    dfParams.voxelLeafSize    = (float)get_parameter("voxel_leaf_size").as_double();

    mcl3d::DistanceField distMap(dfParams);
    if (!distMap.isAvailable()) {
        RCLCPP_ERROR(get_logger(), "Failed to build distance field.");
        return;
    }

    RCLCPP_INFO(get_logger(), "MCL3DNode has been started.");
}

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<MCLNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}