#pragma once

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <common.h>
#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace mcl3d {

// ─── MAP VALUE CONSTANTS ─────────────────────────────────────────────────────
// Stored in cells_[u][v][w][flat_idx]:
//   MAP_VAL_EXIST_POINT (0)  : occupied voxel (also distance-index 0 after build)
//   1–254                    : index into dists_[u][v][w] (sorted distances [m])
//   MAP_VAL_INVALID (0xFFFF…): no distance information (too far or empty)
constexpr uint32_t MAP_VAL_EXIST_POINT = 0u;
constexpr uint32_t MAP_VAL_INVALID     = 0xFFFFFFFFu;

// ─── GridMap ─────────────────────────────────────────────────────────────────
class GridMap {
public:
    GridMap(const GridMapParams &params, rclcpp::Node::SharedPtr node);

    pcl::PointCloud<pcl::PointXYZI> getNearbyPoints(const Pose &pose, float radius) const;

    const pcl::PointCloud<pcl::PointXYZI> &getCloud() const { return cloud_; }
    size_t size() const { return cloud_.size(); }
    bool isAvailable() const { return isAvailable_; }

private:
    pcl::PointCloud<pcl::PointXYZI> cloud_;
    pcl::KdTreeFLANN<pcl::PointXYZI> kdtree_;
    bool isAvailable_ = false;
};

// ─── DistanceField ───────────────────────────────────────────────────────────
// Hierarchical 3D distance field map for O(1) nearest-obstacle queries.
//
// Improvements over the original mcl3d_ros version:
//   • No external dope / dt library dependencies (Felzenszwalb EDT built-in)
//   • getDistance(x,y,z) — O(1) distance lookup used by the MCL likelihood model
//   • std::fstream binary I/O instead of FILE*
//   • Flat sub-map storage (better cache performance)
//   • std::thread parallel EDT construction
//   • Single constructor: auto-detects load (YAML exists) vs build (PCD)
//   • rclcpp::Logger for all log output
class DistanceField {
public:
    explicit DistanceField(
        const DistanceFieldParams& params,
        rclcpp::Logger logger = rclcpp::get_logger("distance_field"));

    // O(1) nearest-obstacle distance lookup [m].
    // Returns maxDistance_ when the point is outside the map or no info is available.
    float getDistance(float x, float y, float z) const;

    // Returns all occupied map voxel centres inside the given AABB.
    std::vector<Point> getMapPoints(
        float minX, float maxX,
        float minY, float maxY,
        float minZ, float maxZ) const;

    bool   isAvailable()          const { return isAvailable_; }
    size_t getTotalMapPointsNum() const { return totalMapPointsNum_; }

private:
    rclcpp::Logger logger_;
    std::string    rootDirName_, mapFileName_;
    float          resolution_       = 5.0f;
    float          subMapResolution_ = 0.1f;
    std::array<float, 3> origin_     = {};
    int width_       = 0, depth_       = 0, height_       = 0;
    int subMapWidth_ = 0, subMapDepth_ = 0, subMapHeight_ = 0;
    size_t totalMapPointsNum_ = 0;
    bool   isAvailable_       = false;
    int    dfThreadsNum_      = 4;
    float  maxDistance_       = 100.0f;  // [m] returned when no data is available

    // [u][v][w] = total point insertions (0 → cell is empty / uninitialized)
    std::vector<std::vector<std::vector<int>>> counts_;

    // [u][v][w] = sub-map origin {ox, oy, oz} [m]
    std::vector<std::vector<std::vector<std::array<float, 3>>>> origins_;

    // [u][v][w] = flat array of size W*D*H.
    //   Before build : MAP_VAL_EXIST_POINT or MAP_VAL_INVALID
    //   After  build : 0–254 (distance index) or MAP_VAL_INVALID
    std::vector<std::vector<std::vector<std::vector<uint32_t>>>> cells_;

    // [u][v][w][i] = distance [m] for index i.
    //   i = 0   : 0.0 m  (occupied voxel)
    //   i = 1–254: sorted unique non-zero distances
    //   i = 255 : -1.0 (sentinel / end marker)
    std::vector<std::vector<std::vector<std::vector<float>>>> dists_;

    // ── Private helpers ──────────────────────────────────────────────────────
    void initGrid();
    void addPoint(float x, float y, float z);
    bool buildFromPCD(const DistanceFieldParams& p);
    void buildCell(int u, int v, int w);
    bool loadMap();
    bool saveMap()                                const;
    bool saveYaml(const DistanceFieldParams& p)   const;

    // Flat index into a W×D×H sub-map cell
    inline int subIdx(int uu, int vv, int ww) const noexcept {
        return uu * subMapDepth_ * subMapHeight_ + vv * subMapHeight_ + ww;
    }

    static bool xyz2uvw(const std::array<float, 3>& o, float res,
                        int nx, int ny, int nz,
                        float x, float y, float z,
                        int* u, int* v, int* w) noexcept;

    static void uvw2xyz(const std::array<float, 3>& o, float res,
                        int u, int v, int w,
                        float* x, float* y, float* z) noexcept;

    // 1-D squared-distance transform (Felzenszwalb & Huttenlocher 2012)
    // d[q] = min_v { (q-v)^2 + f[v] }
    static void dt1d(const float* f, float* d, int n);

    // 3-D Euclidean DT via three separable 1-D passes.
    // Input/output: field[u*ny*nz + v*nz + w]
    //   0.0f at occupied voxels, 1e18f elsewhere → squared voxel-unit distances
    static void edt3d(std::vector<float>& field, int nx, int ny, int nz);
};

}  // namespace mcl3d
