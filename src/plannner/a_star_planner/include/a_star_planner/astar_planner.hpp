#ifndef NAVYU_PLANNER__ASTAR_PLANNER_HPP_
#define NAVYU_PLANNER__ASTAR_PLANNER_HPP_

#include "a_star_planner/base_global_planner.hpp"
#include "a_star_planner/node.hpp"

#include <Eigen/Core>

#include <geometry_msgs/msg/pose.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>

#include <algorithm>
#include <deque>
#include <string>
#include <queue>
#include <unordered_map>
#include <vector>

class AstarPlanner : public BaseGlobalPlanner
{
public:
  AstarPlanner(int size_x, int size_y, double resolution, double origin_x, double origin_y)
  : BaseGlobalPlanner(size_x, size_y, resolution, origin_x, origin_y)
  {
  }
  ~AstarPlanner() {}

  // path が指すノードは次の plan() 呼び出しまで有効 (node_pool_ が所有する)
  bool plan(const Node2D & start, const Node2D & goal, std::vector<Node2D *> & path)
  {
    path.clear();
    node_pool_.clear();
    error_.clear();

    if (!in_bounds(start.x_, start.y_)) {
      error_ = "start is outside of the map";
      return false;
    }
    if (!in_bounds(goal.x_, goal.y_)) {
      error_ = "goal is outside of the map";
      return false;
    }
    if (!is_traversable(get_grid_index(start.x_, start.y_))) {
      error_ = "start is inside an obstacle or unknown area";
      return false;
    }
    if (!is_traversable(get_grid_index(goal.x_, goal.y_))) {
      error_ = "goal is inside an obstacle or unknown area";
      return false;
    }

    Node2D * start_node = new_node(start.x_, start.y_, 0.0, nullptr);
    Node2D * goal_node = new_node(goal.x_, goal.y_, 0.0, nullptr);

    auto compare = [](Node2D * n1, Node2D * n2) { return n1->f_ > n2->f_; };
    std::priority_queue<Node2D *, std::vector<Node2D *>, decltype(compare)> open_list(compare);
    std::unordered_map<int, Node2D *> close_list;

    start_node->cost(start_node, goal_node);
    open_list.push(start_node);

    while (!open_list.empty()) {
      Node2D * current_node = open_list.top();

      open_list.pop();

      if (close_list.find(current_node->grid_index_) != close_list.end()) continue;

      close_list.insert(std::make_pair(current_node->grid_index_, current_node));

      if (goal_node->x_ == current_node->x_ and goal_node->y_ == current_node->y_) {
        path = find_path(current_node);
        return true;
      }

      const std::vector<Node2D> motion_list = current_node->get_motion();
      for (const auto & motion : motion_list) {
        const int next_x = current_node->x_ + motion.x_;
        const int next_y = current_node->y_ + motion.y_;
        if (!in_bounds(next_x, next_y)) {
          continue;
        }

        const int grid_index = get_grid_index(next_x, next_y);
        if (!is_traversable(grid_index)) {
          continue;
        }

        // already exist in close list
        if (close_list.find(grid_index) != close_list.end()) {
          continue;
        }

        const double traversal_cost =
          motion.g_ + cost_weight_ * static_cast<double>(costmap_[grid_index]);
        Node2D * node = new_node(next_x, next_y, current_node->g_ + traversal_cost, current_node);

        // update heuristic cost of new node
        node->cost(node, goal_node);

        open_list.push(node);
      }
    }
    error_ = "no path found";
    return false;
  }

  const std::string & error() const { return error_; }

private:
  bool in_bounds(int x, int y) const { return x >= 0 && y >= 0 && x < size_x_ && y < size_y_; }

  // 未知セル (負値) と lethal セルは通行不可
  bool is_traversable(int grid_index) const
  {
    const int8_t cell_cost = costmap_[grid_index];
    return cell_cost >= 0 && cell_cost < lethal_planning_cost_;
  }

  Node2D * new_node(int x, int y, double g, Node2D * parent)
  {
    node_pool_.emplace_back(x, y, g, parent);
    Node2D * node = &node_pool_.back();
    node->set_grid_index(get_grid_index(x, y));
    return node;
  }

  static std::vector<Node2D *> find_path(Node2D * goal_reached)
  {
    std::vector<Node2D *> path;
    for (Node2D * current = goal_reached; current != nullptr; current = current->parent_) {
      path.emplace_back(current);
    }
    std::reverse(path.begin(), path.end());
    return path;
  }

  std::deque<Node2D> node_pool_;
  std::string error_;
};

#endif  // NAVYU_PLANNER__ASTAR_PLANNER_HPP_
