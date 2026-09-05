#include <nav_msgs/msg/occupancy_grid.hpp>
#include <png.h>
#include <rclcpp/rclcpp.hpp>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace autonomous_drive
{

struct MapMetadata
{
  std::string image_path;
  double resolution = 0.05;
  double origin_x = 0.0;
  double origin_y = 0.0;
  double origin_yaw = 0.0;
  double occupied_thresh = 0.65;
  double free_thresh = 0.196;
  bool negate = false;
};

class MapPublisherNode : public rclcpp::Node
{
public:
  MapPublisherNode()
  : Node("map_publisher")
  {
    declare_parameter<std::string>("map_yaml", "");
    declare_parameter<std::string>("frame_id", "map");
    declare_parameter<std::string>("topic_name", "map");
    declare_parameter<double>("publish_rate", 1.0);

    const auto map_yaml = get_parameter("map_yaml").as_string();
    frame_id_ = get_parameter("frame_id").as_string();
    topic_name_ = get_parameter("topic_name").as_string();
    publish_rate_ = get_parameter("publish_rate").as_double();

    if (map_yaml.empty()) {
      throw std::runtime_error("map_yaml parameter is required");
    }

    map_msg_ = loadMap(map_yaml);
    map_msg_.header.frame_id = frame_id_;

    publisher_ = create_publisher<nav_msgs::msg::OccupancyGrid>(
      topic_name_, rclcpp::QoS(1).transient_local().reliable());

    publishMap();

    if (publish_rate_ > 0.0) {
      const auto period = std::chrono::duration<double>(1.0 / publish_rate_);
      timer_ = create_wall_timer(
        std::chrono::duration_cast<std::chrono::nanoseconds>(period),
        std::bind(&MapPublisherNode::publishMap, this));
    }

    RCLCPP_INFO(
      get_logger(),
      "Publishing map on '%s' (%ux%u, resolution=%.3f, frame=%s, rate=%.1f Hz)",
      topic_name_.c_str(),
      map_msg_.info.width,
      map_msg_.info.height,
      map_msg_.info.resolution,
      frame_id_.c_str(),
      publish_rate_);
  }

private:
  static MapMetadata loadMetadata(const std::string & map_yaml_path)
  {
    const YAML::Node config = YAML::LoadFile(map_yaml_path);
    MapMetadata metadata;

    if (!config["image"]) {
      throw std::runtime_error("map yaml missing 'image' field: " + map_yaml_path);
    }

    const std::filesystem::path yaml_path(map_yaml_path);
    metadata.image_path = (yaml_path.parent_path() / config["image"].as<std::string>()).string();
    metadata.resolution = config["resolution"].as<double>();
    metadata.negate = config["negate"].as<int>() != 0;
    metadata.occupied_thresh = config["occupied_thresh"].as<double>();
    metadata.free_thresh = config["free_thresh"].as<double>();

    const auto origin = config["origin"].as<std::vector<double>>();
    if (origin.size() < 3) {
      throw std::runtime_error("map yaml origin must have 3 values");
    }
    metadata.origin_x = origin[0];
    metadata.origin_y = origin[1];
    metadata.origin_yaw = origin[2];
    return metadata;
  }

  static std::vector<uint8_t> loadPngGrayscale(const std::string & image_path, uint32_t & width, uint32_t & height)
  {
    FILE * fp = std::fopen(image_path.c_str(), "rb");
    if (fp == nullptr) {
      throw std::runtime_error("Failed to open map image: " + image_path);
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png_ptr == nullptr) {
      std::fclose(fp);
      throw std::runtime_error("Failed to create png read struct");
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == nullptr) {
      png_destroy_read_struct(&png_ptr, nullptr, nullptr);
      std::fclose(fp);
      throw std::runtime_error("Failed to create png info struct");
    }

    if (setjmp(png_jmpbuf(png_ptr)) != 0) {
      png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
      std::fclose(fp);
      throw std::runtime_error("Failed to decode png: " + image_path);
    }

    png_init_io(png_ptr, fp);
    png_read_info(png_ptr, info_ptr);

    width = png_get_image_width(png_ptr, info_ptr);
    height = png_get_image_height(png_ptr, info_ptr);
    const png_byte color_type = png_get_color_type(png_ptr, info_ptr);
    const png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);

    if (bit_depth == 16) {
      png_set_strip_16(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_PALETTE) {
      png_set_palette_to_rgb(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8) {
      png_set_expand_gray_1_2_4_to_8(png_ptr);
    }
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS)) {
      png_set_tRNS_to_alpha(png_ptr);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGBA ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
      png_set_filler(png_ptr, 0xFF, PNG_FILLER_AFTER);
    }
    if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_RGBA ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    {
      png_set_gray_to_rgb(png_ptr);
    }

    png_read_update_info(png_ptr, info_ptr);

    const png_size_t row_bytes = png_get_rowbytes(png_ptr, info_ptr);
    const int channels = png_get_channels(png_ptr, info_ptr);
    std::vector<png_bytep> row_pointers(height);
    std::vector<uint8_t> raw_data(height * row_bytes);
    for (uint32_t y = 0; y < height; ++y) {
      row_pointers[y] = raw_data.data() + y * row_bytes;
    }

    png_read_image(png_ptr, row_pointers.data());
    png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
    std::fclose(fp);

    std::vector<uint8_t> grayscale(width * height);
    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        const uint8_t * pixel = raw_data.data() + y * row_bytes + x * channels;
        if (channels >= 3) {
          grayscale[y * width + x] = static_cast<uint8_t>(
            (static_cast<int>(pixel[0]) + pixel[1] + pixel[2]) / 3);
        } else {
          grayscale[y * width + x] = pixel[0];
        }
      }
    }
    return grayscale;
  }

  static int8_t occupancyFromPixel(
    uint8_t pixel,
    bool negate,
    double occupied_thresh,
    double free_thresh)
  {
    if (pixel == 205) {
      return -1;
    }

    double occ = (255.0 - static_cast<double>(pixel)) / 255.0;
    if (negate) {
      occ = static_cast<double>(pixel) / 255.0;
    }

    if (occ > occupied_thresh) {
      return 100;
    }
    if (occ < free_thresh) {
      return 0;
    }
    return -1;
  }

  static nav_msgs::msg::OccupancyGrid loadMap(const std::string & map_yaml_path)
  {
    const MapMetadata metadata = loadMetadata(map_yaml_path);
    uint32_t width = 0;
    uint32_t height = 0;
    const std::vector<uint8_t> pixels = loadPngGrayscale(metadata.image_path, width, height);

    nav_msgs::msg::OccupancyGrid grid;
    grid.info.resolution = metadata.resolution;
    grid.info.width = width;
    grid.info.height = height;
    grid.info.origin.position.x = metadata.origin_x;
    grid.info.origin.position.y = metadata.origin_y;
    grid.info.origin.position.z = 0.0;
    grid.info.origin.orientation.z = std::sin(metadata.origin_yaw / 2.0);
    grid.info.origin.orientation.w = std::cos(metadata.origin_yaw / 2.0);
    grid.data.resize(static_cast<size_t>(width) * height);

    for (uint32_t y = 0; y < height; ++y) {
      for (uint32_t x = 0; x < width; ++x) {
        const uint32_t src_index = y * width + x;
        const uint32_t dst_index = (height - y - 1) * width + x;
        grid.data[dst_index] = occupancyFromPixel(
          pixels[src_index],
          metadata.negate,
          metadata.occupied_thresh,
          metadata.free_thresh);
      }
    }

    return grid;
  }

  void publishMap()
  {
    map_msg_.header.stamp = now();
    publisher_->publish(map_msg_);
  }

  std::string frame_id_;
  std::string topic_name_;
  double publish_rate_ = 1.0;
  nav_msgs::msg::OccupancyGrid map_msg_;
  rclcpp::Publisher<nav_msgs::msg::OccupancyGrid>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
};

}  // namespace autonomous_drive

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<autonomous_drive::MapPublisherNode>());
  rclcpp::shutdown();
  return 0;
}
