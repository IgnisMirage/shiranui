#include <rclcpp/rclcpp.hpp>
#include <grid_map.h>

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<rclcpp::Node>("distance_field_builder");

    node->declare_parameter("pcd_file",           "");
    node->declare_parameter("map_file_name",      "dist_map.bin");
    node->declare_parameter("yaml_file_path",     "/tmp/dist_map.yaml");
    node->declare_parameter("resolution",         5.0);
    node->declare_parameter("sub_map_resolution", 0.1);
    node->declare_parameter("map_margin",         1.0);
    node->declare_parameter("voxel_leaf_size",    0.1);

    mcl3d::DistanceFieldParams p;
    p.pcdFile          = node->get_parameter("pcd_file").as_string();
    p.mapFileName      = node->get_parameter("map_file_name").as_string();
    p.yamlFilePath     = node->get_parameter("yaml_file_path").as_string();
    p.resolution       = static_cast<float>(node->get_parameter("resolution").as_double());
    p.subMapResolution = static_cast<float>(node->get_parameter("sub_map_resolution").as_double());
    p.mapMargin        = static_cast<float>(node->get_parameter("map_margin").as_double());
    p.voxelLeafSize    = static_cast<float>(node->get_parameter("voxel_leaf_size").as_double());

    mcl3d::DistanceField df(p, node->get_logger());

    if (!df.isAvailable()) {
        RCLCPP_ERROR(node->get_logger(), "Failed to build distance field.");
        rclcpp::shutdown();
        return 1;
    }
    RCLCPP_INFO(node->get_logger(),
        "Distance field ready: %zu voxels.", df.getTotalMapPointsNum());

    rclcpp::shutdown();
    return 0;
}
