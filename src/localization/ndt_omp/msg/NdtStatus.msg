# Per-scan report of the scan-to-map update. Recorded during bag replay to pick the gate
# thresholds: plot fitness/inlier_ratio over a known-good run, with yaw_rate alongside to see
# what a turn does to them.
std_msgs/Header header

bool accepted            # true when map -> odom was updated from this scan
string reason            # "" when accepted, otherwise the gate that rejected it
                         # (no_odom_tf / too_few_points / linear_speed / not_converged /
                         #  fitness / inlier_ratio / trans_probability / step_trans / step_rot)
bool ndt_ran             # false when NDT was skipped (too_few_points or gate A)

bool has_converged
int32 iterations
int32 num_points         # points handed to NDT (after voxel downsampling)
int32 num_matched        # of those, how many landed in a map voxel with a covariance
float32 inlier_ratio     # num_matched / num_points
float32 fitness          # mean distance [m] from matched points to their map voxel mean
float32 trans_probability # NDT score / num_points (higher is better)

float32 step_trans       # [m]   NDT result minus the odometry-propagated guess
float32 step_rot         # [deg] same, rotation angle
float32 yaw_rate         # [rad/s] about base_link z from the odometry, reported only
float32 linear_speed     # [m/s] from odom, for reference
float32 processing_time   # [s] NDT align + gating
