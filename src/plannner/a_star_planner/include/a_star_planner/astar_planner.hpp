#ifndef NAVYU_PLANNER__ASTAR_PLANNER_HPP_
#define NAVYU_PLANNER__ASTAR_PLANNER_HPP_

#include "a_star_planner/base_global_planner.hpp"
#include "a_star_planner/node.hpp"

#include <Eigen/Core>

#include <geometry_msgs/msg/pose.hpp>
#include <nav_msgs/msg/occupancy_grid.hpp>

#include <algorithm>
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

  bool plan(Node2D * start_node, Node2D * goal_node, std::vector<Node2D *> & path)
  {
    auto compare = [](Node2D * n1, Node2D * n2) { return n1->f_ > n2->f_; };
    std::priority_queue<Node2D *, std::vector<Node2D *>, decltype(compare)> open_list(compare);
    std::unordered_map<int, Node2D *> close_list;

    open_list.push(start_node);

    while (!open_list.empty()) {
      Node2D * current_node = open_list.top();

      open_list.pop();

      if (close_list.find(current_node->grid_index_) != close_list.end()) continue;

      close_list.insert(std::make_pair(current_node->grid_index_, current_node));

      if (goal_node->x_ == current_node->x_ and goal_node->y_ == current_node->y_) {
        close_list.insert(std::make_pair(goal_node->grid_index_, goal_node));
        path = find_path(close_list, start_node, goal_node);
        return true;
      }

      std::vector<Node2D> motion_list = current_node->get_motion();
      for (auto motion : motion_list) {
        const int next_x = current_node->x_ + motion.x_;
        const int next_y = current_node->y_ + motion.y_;
        if (next_x < 0 || next_y < 0 || next_x >= size_x_ || next_y >= size_y_) {
          continue;
        }

        const int grid_index = get_grid_index(next_x, next_y);
        const int8_t cell_cost = costmap_[grid_index];
        if (cell_cost >= lethal_planning_cost_) {
          continue;
        }

        const double traversal_cost =
          motion.g_ + cost_weight_ * static_cast<double>(cell_cost);
        Node2D * node = new Node2D(
          next_x, next_y, current_node->g_ + traversal_cost, current_node);
        node->set_grid_index(grid_index);

        // already exist in close list
        if (close_list.find(node->grid_index_) != close_list.end()) {
          continue;
        }

        // update heuristic cost of new node
        node->cost(node, goal_node);

        open_list.push(node);
      }
    }
    return false;
  }

  std::vector<Node2D *> find_path(
    std::unordered_map<int, Node2D *> node_list, Node2D * start, Node2D * goal)
  {
    std::vector<Node2D *> path;

    Node2D * current = node_list.find(goal->grid_index_)->second;
    while (current != NULL) {
      path.emplace_back(current);
      current = current->parent_;
    }
    path.emplace_back(start);

    std::reverse(path.begin(), path.end());

    return path;
  }
};

#endif  // NAVYU_PLANNER__ASTAR_PLANNER_HPP_
