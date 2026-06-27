#ifndef A_STAR_PLANNER__COSTMAP_INFLATION_HPP_
#define A_STAR_PLANNER__COSTMAP_INFLATION_HPP_

#include <geometry_msgs/msg/polygon_stamped.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <utility>
#include <vector>

namespace a_star_planner
{

struct FootprintVertex
{
  double x;
  double y;
};

inline bool pointInPolygon(double x, double y, const std::vector<FootprintVertex> & polygon)
{
  if (polygon.size() < 3) {
    return false;
  }

  bool inside = false;
  for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const auto & pi = polygon[i];
    const auto & pj = polygon[j];
    const bool intersect = ((pi.y > y) != (pj.y > y)) &&
      (x < (pj.x - pi.x) * (y - pi.y) / (pj.y - pi.y) + pi.x);
    if (intersect) {
      inside = !inside;
    }
  }
  return inside;
}

inline std::vector<std::pair<int, int>> buildInflationOffsets(
  const std::vector<FootprintVertex> & footprint,
  double resolution,
  double padding)
{
  if (footprint.empty() || resolution <= 0.0) {
    return {};
  }

  double min_x = footprint.front().x;
  double max_x = footprint.front().x;
  double min_y = footprint.front().y;
  double max_y = footprint.front().y;
  for (const auto & vertex : footprint) {
    min_x = std::min(min_x, vertex.x);
    max_x = std::max(max_x, vertex.x);
    min_y = std::min(min_y, vertex.y);
    max_y = std::max(max_y, vertex.y);
  }

  min_x -= padding;
  max_x += padding;
  min_y -= padding;
  max_y += padding;

  const int min_ix = static_cast<int>(std::floor(min_x / resolution));
  const int max_ix = static_cast<int>(std::ceil(max_x / resolution));
  const int min_iy = static_cast<int>(std::floor(min_y / resolution));
  const int max_iy = static_cast<int>(std::ceil(max_y / resolution));

  std::vector<std::pair<int, int>> offsets;
  offsets.reserve(static_cast<size_t>((max_ix - min_ix + 1) * (max_iy - min_iy + 1)));

  for (int iy = min_iy; iy <= max_iy; ++iy) {
    for (int ix = min_ix; ix <= max_ix; ++ix) {
      const double wx = static_cast<double>(ix) * resolution;
      const double wy = static_cast<double>(iy) * resolution;
      if (pointInPolygon(wx, wy, footprint)) {
        offsets.emplace_back(ix, iy);
      }
    }
  }

  return offsets;
}

inline std::vector<FootprintVertex> footprintFromPolygon(
  const geometry_msgs::msg::Polygon & polygon)
{
  std::vector<FootprintVertex> footprint;
  footprint.reserve(polygon.points.size());
  for (const auto & point : polygon.points) {
    footprint.push_back({static_cast<double>(point.x), static_cast<double>(point.y)});
  }
  return footprint;
}

inline void markFootprintLethal(
  nav_msgs::msg::OccupancyGrid & map,
  const std::vector<int8_t> & source,
  const std::vector<FootprintVertex> & footprint,
  int8_t lethal_cost_threshold,
  int8_t lethal_cost_value)
{
  const auto offsets = buildInflationOffsets(footprint, map.info.resolution, 0.0);
  if (offsets.empty()) {
    return;
  }

  const int width = static_cast<int>(map.info.width);
  const int height = static_cast<int>(map.info.height);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t index = static_cast<size_t>(y * width + x);
      if (source[index] < lethal_cost_threshold) {
        continue;
      }

      map.data[index] = lethal_cost_value;
      for (const auto & [dx, dy] : offsets) {
        const int nx = x + dx;
        const int ny = y + dy;
        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
          continue;
        }
        map.data[static_cast<size_t>(ny * width + nx)] = lethal_cost_value;
      }
    }
  }
}

inline void applyClearanceCostLayer(
  nav_msgs::msg::OccupancyGrid & map,
  int8_t lethal_cost_value,
  double clearance_distance)
{
  if (map.data.empty() || clearance_distance <= 0.0) {
    return;
  }

  const int width = static_cast<int>(map.info.width);
  const int height = static_cast<int>(map.info.height);
  const size_t cell_count = map.data.size();
  const double resolution = map.info.resolution;
  const double diagonal_step = resolution * std::sqrt(2.0);

  std::vector<double> distances(cell_count, std::numeric_limits<double>::infinity());
  std::queue<std::pair<int, int>> queue;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      const size_t index = static_cast<size_t>(y * width + x);
      if (map.data[index] >= lethal_cost_value) {
        distances[index] = 0.0;
        queue.emplace(x, y);
      }
    }
  }

  while (!queue.empty()) {
    const auto [x, y] = queue.front();
    queue.pop();
    const size_t index = static_cast<size_t>(y * width + x);
    const double current_distance = distances[index];

    static constexpr int kNeighborCount = 8;
    static constexpr int kDx[kNeighborCount] = {1, 0, -1, 0, 1, 1, -1, -1};
    static constexpr int kDy[kNeighborCount] = {0, 1, 0, -1, 1, -1, 1, -1};
    static constexpr double kStep[kNeighborCount] = {
      1.0, 1.0, 1.0, 1.0,
      1.4142135623730951, 1.4142135623730951, 1.4142135623730951, 1.4142135623730951};

    for (int i = 0; i < kNeighborCount; ++i) {
      const int nx = x + kDx[i];
      const int ny = y + kDy[i];
      if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
        continue;
      }

      const size_t neighbor_index = static_cast<size_t>(ny * width + nx);
      const double step = (kStep[i] > 1.0) ? diagonal_step : resolution;
      const double candidate_distance = current_distance + step;
      if (candidate_distance + 1e-9 >= distances[neighbor_index]) {
        continue;
      }
      if (candidate_distance > clearance_distance) {
        continue;
      }

      distances[neighbor_index] = candidate_distance;
      queue.emplace(nx, ny);
    }
  }

  for (size_t index = 0; index < cell_count; ++index) {
    if (map.data[index] >= lethal_cost_value) {
      continue;
    }
    const double distance = distances[index];
    if (!std::isfinite(distance) || distance >= clearance_distance) {
      continue;
    }

    const double normalized = 1.0 - (distance / clearance_distance);
    const int8_t proximity_cost = static_cast<int8_t>(std::lround(99.0 * normalized));
    map.data[index] = std::max(map.data[index], proximity_cost);
  }
}

inline nav_msgs::msg::OccupancyGrid buildPlanningCostmap(
  const nav_msgs::msg::OccupancyGrid & map,
  const std::vector<FootprintVertex> & footprint,
  int8_t lethal_cost_threshold,
  int8_t lethal_cost_value,
  double clearance_distance)
{
  nav_msgs::msg::OccupancyGrid planning_map = map;
  if (map.data.empty()) {
    return planning_map;
  }

  const std::vector<int8_t> source = map.data;
  if (footprint.size() >= 3) {
    markFootprintLethal(
      planning_map, source, footprint, lethal_cost_threshold, lethal_cost_value);
  } else {
    for (size_t i = 0; i < source.size(); ++i) {
      if (source[i] >= lethal_cost_threshold) {
        planning_map.data[i] = lethal_cost_value;
      }
    }
  }

  applyClearanceCostLayer(planning_map, lethal_cost_value, clearance_distance);
  return planning_map;
}

}  // namespace a_star_planner

#endif  // A_STAR_PLANNER__COSTMAP_INFLATION_HPP_
