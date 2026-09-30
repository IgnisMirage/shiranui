#ifndef NDT_OMP_NDT_SCAN_MATCHER_HPP
#define NDT_OMP_NDT_SCAN_MATCHER_HPP

#include <Eigen/Core>
#include <pcl/point_cloud.h>
#include <pclomp/ndt_omp.h>

namespace ndt_omp {

/**
 * @brief pclomp NDT plus a match-quality readout taken from the map voxel grid.
 *
 * pcl::Registration::getFitnessScore() is not usable in this node: it runs one
 * nearest-neighbour query per source point against the *target* kd-tree, and the node
 * deliberately never builds that tree (setSearchMethodTarget(tree, true)) because the
 * prior map is tens of millions of points and pclomp's NDT does not use it.
 *
 * NDT already owns a voxel grid of the map, so quality is read straight out of it:
 *   num_matched : aligned points landing in a voxel that has a covariance
 *                 (>= getMinPointPerVoxel() map points, 6 by default)
 *   fitness     : mean distance of those points to their voxel mean, in metres
 *
 * The grid saturates the distance at roughly the NDT resolution, so inlier_ratio is the
 * more discriminative of the two for a gross mismatch; both are published so thresholds
 * can be read off a bag instead of guessed.
 *
 * target_cells_ is protected in pclomp, hence this subclass: include/pclomp and
 * src/pclomp are vendored upstream code (koide3/ndt_omp) and are not edited.
 */
template <typename PointSource, typename PointTarget>
class NdtScanMatcher : public pclomp::NormalDistributionsTransform<PointSource, PointTarget> {
public:
  struct MatchStats {
    double fitness = 0.0;       // [m], mean over matched points only
    double inlier_ratio = 0.0;  // matched / total
    int matched = 0;
    int total = 0;
  };

  /// @param aligned the cloud returned by align(), i.e. already in the map frame
  MatchStats computeMatchStats(const pcl::PointCloud<PointSource>& aligned) {
    MatchStats stats;
    stats.total = static_cast<int>(aligned.size());

    const int min_points = this->target_cells_.getMinPointPerVoxel();
    double sum = 0.0;
    for (const auto& pt : aligned.points) {
      PointTarget query;
      query.x = pt.x;
      query.y = pt.y;
      query.z = pt.z;

      const auto leaf = this->target_cells_.getLeaf(query);
      if (leaf == nullptr || leaf->nr_points < min_points) {
        continue;
      }
      sum += (leaf->getMean() - Eigen::Vector3d(pt.x, pt.y, pt.z)).norm();
      ++stats.matched;
    }

    if (stats.matched > 0) {
      stats.fitness = sum / stats.matched;
    }
    if (stats.total > 0) {
      stats.inlier_ratio = static_cast<double>(stats.matched) / static_cast<double>(stats.total);
    }
    return stats;
  }
};

}  // namespace ndt_omp

#endif  // NDT_OMP_NDT_SCAN_MATCHER_HPP
