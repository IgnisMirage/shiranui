#ifndef NAVYU_PATH_PLANNER__SMOOTHER_HPP_
#define NAVYU_PATH_PLANNER__SMOOTHER_HPP_

#include <nav_msgs/msg/path.hpp>

#include <cmath>
#include <vector>

class Smoother
{
public:
  Smoother(double displacement_threshold) 
  { 
    displacement_threshold_ = displacement_threshold; 
  }

  nav_msgs::msg::Path smooth(nav_msgs::msg::Path path)
  {
    std::vector<nav_msgs::msg::Path> segments_path = split_path_with_dist(path);
    nav_msgs::msg::Path optimized_path;
    optimized_path.header = path.header;

    for (const auto & segment : segments_path) {
      const int path_size = static_cast<int>(segment.poses.size());
      if (path_size <= 0) {
        continue;
      }

      const int sample_count = std::max(path_size, 2);
      for (int sample_idx = 0; sample_idx <= sample_count; ++sample_idx) {
        const double t = static_cast<double>(sample_idx) / static_cast<double>(sample_count);
        geometry_msgs::msg::PoseStamped pose;
        pose.header = path.header;
        pose.pose.position = evaluateBezierPoint(segment, t);
        optimized_path.poses.emplace_back(pose);
      }
    }

    return optimized_path;
  }

  static geometry_msgs::msg::Point evaluateBezierPoint(
    const nav_msgs::msg::Path & segment, double t)
  {
    std::vector<double> xs;
    std::vector<double> ys;
    xs.reserve(segment.poses.size());
    ys.reserve(segment.poses.size());
    for (const auto & pose_stamped : segment.poses) {
      xs.push_back(pose_stamped.pose.position.x);
      ys.push_back(pose_stamped.pose.position.y);
    }

    const size_t degree = xs.size();
    for (size_t r = 1; r < degree; ++r) {
      for (size_t i = 0; i + r < degree; ++i) {
        xs[i] = (1.0 - t) * xs[i] + t * xs[i + 1];
        ys[i] = (1.0 - t) * ys[i] + t * ys[i + 1];
      }
    }

    geometry_msgs::msg::Point point;
    point.x = xs.front();
    point.y = ys.front();
    point.z = 0.0;
    return point;
  }

  std::vector<nav_msgs::msg::Path> split_path_with_dist(nav_msgs::msg::Path path)
  {
    std::vector<nav_msgs::msg::Path> segments_path;

    auto calculate_distance =
      [](geometry_msgs::msg::Point p1, geometry_msgs::msg::Point p2) -> double {
      return std::hypot((p2.x - p1.x), (p2.y - p1.y));
    };

    double displacement = 0.0;
    nav_msgs::msg::Path current_segments_path;
    for (auto pose : path.poses) {
      current_segments_path.poses.emplace_back(pose);

      if (2 <= current_segments_path.poses.size()) {
        displacement += calculate_distance(
          current_segments_path.poses[current_segments_path.poses.size() - 2].pose.position,
          pose.pose.position);
      }

      if (displacement_threshold_ < displacement) {
        segments_path.emplace_back(current_segments_path);
        current_segments_path.poses.clear();
        displacement = 0.0;
      }
    }

    current_segments_path.poses.emplace_back(path.poses[path.poses.size() - 1]);
    segments_path.emplace_back(current_segments_path);

    return segments_path;
  }

  std::vector<nav_msgs::msg::Path> split_path(nav_msgs::msg::Path path)
  {
    std::vector<nav_msgs::msg::Path> segments_path;
    nav_msgs::msg::Path current_segments_path;

    // detect cusp and split path
    for (int i = 0; i < path.poses.size() - 1; i++) {
      // cross product
      const double ux = path.poses[i].pose.position.x - path.poses[i - 1].pose.position.x;
      const double uy = path.poses[i].pose.position.y - path.poses[i - 1].pose.position.y;
      const double vx = path.poses[i + 1].pose.position.x - path.poses[i].pose.position.x;
      const double vy = path.poses[i + 1].pose.position.y - path.poses[i].pose.position.y;
      const double cp = ux * vy - uy * vx;
      // const double cp = ux * vx + uy * vy;

      if (0.1 < std::abs(cp)) {
        segments_path.emplace_back(current_segments_path);
        current_segments_path.poses.clear();
      }
      current_segments_path.poses.emplace_back(path.poses[i]);
    }

    // insert goal node
    current_segments_path.poses.emplace_back(path.poses[path.poses.size() - 1]);
    segments_path.emplace_back(current_segments_path);

    return segments_path;
  }

private:
  double displacement_threshold_;
};

#endif
