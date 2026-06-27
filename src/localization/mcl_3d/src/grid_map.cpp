#include <grid_map.h>
#include <pcl/io/pcd_io.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl_conversions/pcl_conversions.h>
#include <yaml-cpp/yaml.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <thread>

namespace mcl3d {

// ═══════════════════════════════════════════════════════════════════════════════
//  GridMap
// ═══════════════════════════════════════════════════════════════════════════════

GridMap::GridMap(const GridMapParams &params, rclcpp::Node::SharedPtr node) {
    pcl::PointCloud<pcl::PointXYZI> raw;
    if (pcl::io::loadPCDFile(params.pcdFile, raw) < 0) {
        RCLCPP_ERROR(node->get_logger(), "Failed to load PCD: %s", params.pcdFile.c_str());
        return;
    }
    RCLCPP_INFO(node->get_logger(), "Loaded %zu points from %s", raw.size(), params.pcdFile.c_str());

    pcl::VoxelGrid<pcl::PointXYZI> vg;
    vg.setInputCloud(raw.makeShared());
    vg.setLeafSize(params.voxelLeafSize, params.voxelLeafSize, params.voxelLeafSize);
    vg.filter(cloud_);

    kdtree_.setInputCloud(cloud_.makeShared());

    auto pub = node->create_publisher<sensor_msgs::msg::PointCloud2>(
        params.mapPointsTopic, rclcpp::QoS(1).transient_local());
    sensor_msgs::msg::PointCloud2 msg;
    pcl::toROSMsg(cloud_, msg);
    msg.header.frame_id = "map";
    msg.header.stamp    = node->now();
    pub->publish(msg);
    RCLCPP_INFO(node->get_logger(), "Published map points on %s", params.mapPointsTopic.c_str());
    isAvailable_ = true;
}

pcl::PointCloud<pcl::PointXYZI> GridMap::getNearbyPoints(const Pose &pose, float radius) const {
    pcl::PointXYZI query;
    query.x = static_cast<float>(pose.x);
    query.y = static_cast<float>(pose.y);
    query.z = static_cast<float>(pose.z);

    std::vector<int>   indices;
    std::vector<float> sqDists;
    kdtree_.radiusSearch(query, radius, indices, sqDists);

    pcl::PointCloud<pcl::PointXYZI> nearby;
    nearby.reserve(indices.size());
    for (int idx : indices)
        nearby.push_back(cloud_[idx]);
    return nearby;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — static helpers
// ═══════════════════════════════════════════════════════════════════════════════

bool DistanceField::xyz2uvw(
    const std::array<float, 3>& o, float res,
    int nx, int ny, int nz,
    float x, float y, float z,
    int* u, int* v, int* w) noexcept
{
    *u = static_cast<int>(std::floor((x - o[0]) / res));
    *v = static_cast<int>(std::floor((y - o[1]) / res));
    *w = static_cast<int>(std::floor((z - o[2]) / res));
    return *u >= 0 && *u < nx && *v >= 0 && *v < ny && *w >= 0 && *w < nz;
}

void DistanceField::uvw2xyz(
    const std::array<float, 3>& o, float res,
    int u, int v, int w,
    float* x, float* y, float* z) noexcept
{
    // Return voxel centre
    *x = o[0] + (static_cast<float>(u) + 0.5f) * res;
    *y = o[1] + (static_cast<float>(v) + 0.5f) * res;
    *z = o[2] + (static_cast<float>(w) + 0.5f) * res;
}

// 1-D squared-distance transform  (Felzenszwalb & Huttenlocher, TPAMI 2012)
//   d[q] = min_{v} { (q−v)² + f[v] }
// z[0] = −BIG guarantees the inner do-while never underflows k below 0.
void DistanceField::dt1d(const float* f, float* d, int n)
{
    constexpr float BIG = 1e18f;
    std::vector<int>   v(n);
    std::vector<float> z(n + 1);
    int k = 0;
    v[0] = 0;
    z[0] = -BIG;
    z[1] =  BIG;

    for (int q = 1; q < n; ++q) {
        float s;
        do {
            const int r = v[k];
            s = ((f[q] + static_cast<float>(q * q))
               - (f[r] + static_cast<float>(r * r)))
              / (2.0f * static_cast<float>(q - r));
            if (s > z[k]) break;
            if (k == 0)   break;   // defensive: z[0]=−BIG so s>z[0] is always true
            --k;
        } while (true);
        ++k;
        v[k] = q;
        z[k] = s;
        z[k + 1] = BIG;
    }

    k = 0;
    for (int q = 0; q < n; ++q) {
        while (z[k + 1] < static_cast<float>(q)) ++k;
        const float diff = static_cast<float>(q - v[k]);
        d[q] = diff * diff + f[v[k]];
    }
}

// 3-D exact Euclidean DT via three separable 1-D passes.
// Layout: field[u*ny*nz + v*nz + w], size nx*ny*nz.
// Input : 0.0f at occupied voxels, 1e18f at empty.
// Output: squared voxel-unit distances (multiply by res² for metres²).
void DistanceField::edt3d(std::vector<float>& field, int nx, int ny, int nz)
{
    const int maxDim = std::max({nx, ny, nz});
    std::vector<float> tmp(maxDim), res(maxDim);

    // Pass along X
    for (int v = 0; v < ny; ++v) {
        for (int w = 0; w < nz; ++w) {
            for (int u = 0; u < nx; ++u) tmp[u] = field[u * ny * nz + v * nz + w];
            dt1d(tmp.data(), res.data(), nx);
            for (int u = 0; u < nx; ++u) field[u * ny * nz + v * nz + w] = res[u];
        }
    }
    // Pass along Y
    for (int u = 0; u < nx; ++u) {
        for (int w = 0; w < nz; ++w) {
            for (int v = 0; v < ny; ++v) tmp[v] = field[u * ny * nz + v * nz + w];
            dt1d(tmp.data(), res.data(), ny);
            for (int v = 0; v < ny; ++v) field[u * ny * nz + v * nz + w] = res[v];
        }
    }
    // Pass along Z
    for (int u = 0; u < nx; ++u) {
        for (int v = 0; v < ny; ++v) {
            for (int w = 0; w < nz; ++w) tmp[w] = field[u * ny * nz + v * nz + w];
            dt1d(tmp.data(), res.data(), nz);
            for (int w = 0; w < nz; ++w) field[u * ny * nz + v * nz + w] = res[w];
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — constructor
// ═══════════════════════════════════════════════════════════════════════════════

DistanceField::DistanceField(const DistanceFieldParams& params, rclcpp::Logger logger)
    : logger_(logger)
{
    rootDirName_ = params.yamlFilePath.substr(
        0, params.yamlFilePath.find_last_of('/') + 1);
    mapFileName_ = params.mapFileName;

    // ── Try to load an existing map ──────────────────────────────────────────
    {
        std::ifstream probe(params.yamlFilePath);
        if (probe.good()) {
            RCLCPP_INFO(logger_, "YAML found, loading map: %s",
                params.yamlFilePath.c_str());
            try {
                YAML::Node cfg  = YAML::LoadFile(params.yamlFilePath);
                mapFileName_    = cfg["map_file_name"].as<std::string>();
                resolution_     = cfg["resolution"].as<float>();
                auto org        = cfg["origin"].as<std::vector<float>>();
                origin_         = {org[0], org[1], org[2]};
                width_          = cfg["width"].as<int>();
                depth_          = cfg["depth"].as<int>();
                height_         = cfg["height"].as<int>();
                subMapResolution_ = cfg["sub_map_resolution"].as<float>();
                subMapWidth_    = cfg["sub_map_width"].as<int>();
                subMapDepth_    = cfg["sub_map_depth"].as<int>();
                subMapHeight_   = cfg["sub_map_height"].as<int>();
            } catch (const std::exception& e) {
                RCLCPP_ERROR(logger_, "YAML parse error: %s", e.what());
                return;
            }
            initGrid();
            if (!loadMap()) return;
            isAvailable_ = true;
            RCLCPP_INFO(logger_, "Map loaded: %zu points", totalMapPointsNum_);
            return;
        }
    }

    // ── Build from PCD ───────────────────────────────────────────────────────
    RCLCPP_INFO(logger_, "No existing map found, building from PCD: %s",
        params.pcdFile.c_str());
    if (!buildFromPCD(params)) return;
    if (!saveYaml(params))     return;
    if (!saveMap())            return;
    isAvailable_ = true;
    RCLCPP_INFO(logger_, "Map built and saved: %zu points", totalMapPointsNum_);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — grid initialisation
// ═══════════════════════════════════════════════════════════════════════════════

void DistanceField::initGrid()
{
    totalMapPointsNum_ = 0;

    counts_.assign(width_,
        std::vector<std::vector<int>>(depth_, std::vector<int>(height_, 0)));

    origins_.assign(width_,
        std::vector<std::vector<std::array<float, 3>>>(
            depth_, std::vector<std::array<float, 3>>(
                height_, {0.f, 0.f, 0.f})));

    cells_.assign(width_,
        std::vector<std::vector<std::vector<uint32_t>>>(
            depth_, std::vector<std::vector<uint32_t>>(height_)));

    dists_.assign(width_,
        std::vector<std::vector<std::vector<float>>>(
            depth_, std::vector<std::vector<float>>(height_)));
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — addPoint (used during build)
// ═══════════════════════════════════════════════════════════════════════════════

void DistanceField::addPoint(float x, float y, float z)
{
    int u, v, w;
    if (!xyz2uvw(origin_, resolution_, width_, depth_, height_, x, y, z, &u, &v, &w))
        return;

    if (counts_[u][v][w] == 0) {
        origins_[u][v][w] = {
            origin_[0] + static_cast<float>(u) * resolution_,
            origin_[1] + static_cast<float>(v) * resolution_,
            origin_[2] + static_cast<float>(w) * resolution_};
        cells_[u][v][w].assign(
            subMapWidth_ * subMapDepth_ * subMapHeight_, MAP_VAL_INVALID);
        dists_[u][v][w].resize(256, 0.f);
    }

    int uu, vv, ww;
    if (xyz2uvw(origins_[u][v][w], subMapResolution_,
                subMapWidth_, subMapDepth_, subMapHeight_,
                x, y, z, &uu, &vv, &ww))
    {
        cells_[u][v][w][subIdx(uu, vv, ww)] = MAP_VAL_EXIST_POINT;
        ++counts_[u][v][w];
        ++totalMapPointsNum_;
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — build from PCD file
// ═══════════════════════════════════════════════════════════════════════════════

bool DistanceField::buildFromPCD(const DistanceFieldParams& p)
{
    // Load PCD
    pcl::PointCloud<pcl::PointXYZI> raw;
    if (pcl::io::loadPCDFile(p.pcdFile, raw) < 0) {
        RCLCPP_ERROR(logger_, "Cannot load PCD: %s", p.pcdFile.c_str());
        return false;
    }
    RCLCPP_INFO(logger_, "Loaded %zu points", raw.size());

    // Voxel downsampling
    pcl::PointCloud<pcl::PointXYZI> cloud;
    if (p.voxelLeafSize > 0.f) {
        pcl::VoxelGrid<pcl::PointXYZI> vg;
        vg.setInputCloud(raw.makeShared());
        vg.setLeafSize(p.voxelLeafSize, p.voxelLeafSize, p.voxelLeafSize);
        vg.filter(cloud);
    } else {
        cloud = raw;
    }
    RCLCPP_INFO(logger_, "After voxelisation: %zu points", cloud.size());

    // Bounding box
    float minX = std::numeric_limits<float>::max();
    float minY = minX, minZ = minX;
    float maxX = std::numeric_limits<float>::lowest();
    float maxY = maxX, maxZ = maxX;
    for (const auto& pt : cloud) {
        minX = std::min(minX, pt.x); maxX = std::max(maxX, pt.x);
        minY = std::min(minY, pt.y); maxY = std::max(maxY, pt.y);
        minZ = std::min(minZ, pt.z); maxZ = std::max(maxZ, pt.z);
    }
    minX -= p.mapMargin; minY -= p.mapMargin; minZ -= p.mapMargin;
    maxX += p.mapMargin; maxY += p.mapMargin; maxZ += p.mapMargin;

    resolution_       = p.resolution;
    subMapResolution_ = p.subMapResolution;
    mapFileName_      = p.mapFileName;
    origin_           = {minX, minY, minZ};

    // Grid dimensions
    width_ = depth_ = height_ = 0;
    for (float r = 0.f; r < maxX - minX; r += resolution_) ++width_;
    for (float r = 0.f; r < maxY - minY; r += resolution_) ++depth_;
    for (float r = 0.f; r < maxZ - minZ; r += resolution_) ++height_;
    subMapWidth_ = subMapDepth_ = subMapHeight_ =
        static_cast<int>(resolution_ / subMapResolution_);

    initGrid();

    for (const auto& pt : cloud)
        addPoint(pt.x, pt.y, pt.z);

    RCLCPP_INFO(logger_,
        "Grid %d×%d×%d, sub %d×%d×%d, %zu unique voxels",
        width_, depth_, height_,
        subMapWidth_, subMapDepth_, subMapHeight_, totalMapPointsNum_);

    // Collect occupied cells
    std::vector<std::tuple<int, int, int>> occupied;
    occupied.reserve(width_ * depth_ * height_ / 4);
    for (int u = 0; u < width_; ++u)
        for (int v = 0; v < depth_; ++v)
            for (int w = 0; w < height_; ++w)
                if (counts_[u][v][w] > 0)
                    occupied.emplace_back(u, v, w);

    // Parallel EDT
    const int numCells = static_cast<int>(occupied.size());
    RCLCPP_INFO(logger_, "Computing EDT for %d cells using %d threads…",
        numCells, dfThreadsNum_);
    std::atomic<int> nextCell{0};
    std::atomic<int> doneCount{0};
    const int logStep = std::max(1, numCells / 10);

    std::vector<std::thread> workers;
    workers.reserve(dfThreadsNum_);
    for (int t = 0; t < dfThreadsNum_; ++t) {
        workers.emplace_back([&]() {
            int i;
            while ((i = nextCell.fetch_add(1)) < numCells) {
                auto [u, v, w] = occupied[i];
                buildCell(u, v, w);
                int done = doneCount.fetch_add(1) + 1;
                if (done % logStep == 0 || done == numCells)
                    RCLCPP_INFO(logger_, "EDT: %d/%d cells (%.0f%%)",
                        done, numCells, 100.f * done / numCells);
            }
        });
    }
    for (auto& t : workers) t.join();

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — EDT for a single sub-map cell
// ═══════════════════════════════════════════════════════════════════════════════

void DistanceField::buildCell(int uo, int vo, int wo)
{
    const int nx = subMapWidth_, ny = subMapDepth_, nz = subMapHeight_;
    const int total = nx * ny * nz;
    constexpr float INF = 1e18f;

    // Initialise EDT input from occupancy
    std::vector<float> field(total);
    auto& data = cells_[uo][vo][wo];
    for (int i = 0; i < total; ++i)
        field[i] = (data[i] == MAP_VAL_EXIST_POINT) ? 0.f : INF;

    // 3-D EDT (squared voxel-unit distances)
    edt3d(field, nx, ny, nz);

    // Build sorted unique distance list [cm] (excluding 0 = occupied voxels)
    std::set<int> cmSet;
    for (int i = 0; i < total; ++i) {
        if (field[i] > 0.f && field[i] < INF / 2.f) {
            const int cm = static_cast<int>(
                std::sqrt(field[i]) * subMapResolution_ * 100.f);
            if (cm > 0) cmSet.insert(cm);
        }
    }

    // Fill distance lookup table (indices 0–254; 255 = sentinel)
    auto& dl = dists_[uo][vo][wo];
    dl.assign(256, 0.f);
    std::map<int, uint32_t> inv;
    inv[0] = 0;   // index 0 → 0.0 m (occupied voxel)
    int k = 1;
    for (int cm : cmSet) {
        if (k >= 255) break;
        dl[k] = static_cast<float>(cm) * 0.01f;
        inv[cm] = static_cast<uint32_t>(k);
        ++k;
    }
    dl[255] = -1.f;   // sentinel

    // Maximum trackable distance [cm]
    const int maxCM = (k > 1) ? static_cast<int>(dl[k - 1] * 100.f) : 0;

    // Write distance indices back into the cell
    for (int uu = 0; uu < nx; ++uu) {
        for (int vv = 0; vv < ny; ++vv) {
            for (int ww = 0; ww < nz; ++ww) {
                const int si = subIdx(uu, vv, ww);
                const float fval = field[si];

                if (fval >= INF / 2.f) {
                    data[si] = MAP_VAL_INVALID;
                    continue;
                }
                const int cm = static_cast<int>(
                    std::sqrt(fval) * subMapResolution_ * 100.f);
                if (cm > maxCM) {
                    data[si] = MAP_VAL_INVALID;
                } else {
                    auto it = inv.find(cm);
                    data[si] = (it != inv.end()) ? it->second : 0u;
                }
            }
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — load binary map
// ═══════════════════════════════════════════════════════════════════════════════

bool DistanceField::loadMap()
{
    const std::string path = rootDirName_ + mapFileName_;
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) {
        RCLCPP_ERROR(logger_, "Cannot open map file: %s", path.c_str());
        return false;
    }
    RCLCPP_INFO(logger_, "Loading binary map: %s", path.c_str());

    const int subTotal = subMapWidth_ * subMapDepth_ * subMapHeight_;
    int  di[4];
    float df[3];

    while (ifs.read(reinterpret_cast<char*>(di), sizeof(int) * 4)) {
        const int uo = di[0], vo = di[1], wo = di[2];
        if (uo < 0 || vo < 0 || wo < 0) break;

        counts_[uo][vo][wo] = di[3];
        totalMapPointsNum_ += static_cast<size_t>(di[3]);

        if (!ifs.read(reinterpret_cast<char*>(df), sizeof(float) * 3)) break;
        origins_[uo][vo][wo] = {df[0], df[1], df[2]};

        cells_[uo][vo][wo].assign(subTotal, MAP_VAL_INVALID);
        dists_[uo][vo][wo].resize(256);
        if (!ifs.read(reinterpret_cast<char*>(dists_[uo][vo][wo].data()),
                      sizeof(float) * 256))
            break;

        // Read sub-map entries until the end marker {-1,-1,-1,0}
        while (ifs.read(reinterpret_cast<char*>(di), sizeof(int) * 4)) {
            const int uu = di[0], vv = di[1], ww = di[2];
            if (uu < 0 || vv < 0 || ww < 0) break;
            cells_[uo][vo][wo][subIdx(uu, vv, ww)] =
                static_cast<uint32_t>(di[3]);
        }
    }

    RCLCPP_INFO(logger_, "Binary map loaded: %zu points", totalMapPointsNum_);
    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — save binary map
// ═══════════════════════════════════════════════════════════════════════════════

bool DistanceField::saveMap() const
{
    const std::string path = rootDirName_ + mapFileName_;
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        RCLCPP_ERROR(logger_, "Cannot write map file: %s", path.c_str());
        return false;
    }
    RCLCPP_INFO(logger_, "Saving binary map: %s", path.c_str());

    const int nx = subMapWidth_, ny = subMapDepth_, nz = subMapHeight_;

    for (int u = 0; u < width_; ++u) {
        for (int v = 0; v < depth_; ++v) {
            for (int w = 0; w < height_; ++w) {
                if (counts_[u][v][w] == 0) continue;

                // Cell header
                int di[4] = {u, v, w, counts_[u][v][w]};
                ofs.write(reinterpret_cast<const char*>(di), sizeof(int) * 4);

                float df[3] = {
                    origins_[u][v][w][0],
                    origins_[u][v][w][1],
                    origins_[u][v][w][2]};
                ofs.write(reinterpret_cast<const char*>(df), sizeof(float) * 3);

                // Distance list
                ofs.write(reinterpret_cast<const char*>(dists_[u][v][w].data()),
                          sizeof(float) * 256);

                // Non-invalid voxels
                const auto& data = cells_[u][v][w];
                for (int uu = 0; uu < nx; ++uu) {
                    for (int vv = 0; vv < ny; ++vv) {
                        for (int ww = 0; ww < nz; ++ww) {
                            const uint32_t val = data[subIdx(uu, vv, ww)];
                            if (val != MAP_VAL_INVALID) {
                                int d2[4] = {uu, vv, ww,
                                             static_cast<int>(val)};
                                ofs.write(reinterpret_cast<const char*>(d2),
                                          sizeof(int) * 4);
                            }
                        }
                    }
                }

                // End-of-submap marker
                int end[4] = {-1, -1, -1, 0};
                ofs.write(reinterpret_cast<const char*>(end), sizeof(int) * 4);
            }
        }
    }

    RCLCPP_INFO(logger_, "Binary map saved.");
    return ofs.good();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — save YAML metadata
// ═══════════════════════════════════════════════════════════════════════════════

bool DistanceField::saveYaml(const DistanceFieldParams& p) const
{
    std::ofstream ofs(p.yamlFilePath);
    if (!ofs) {
        RCLCPP_ERROR(logger_, "Cannot write YAML: %s", p.yamlFilePath.c_str());
        return false;
    }
    ofs << "map_file_name: "      << mapFileName_       << "\n"
        << "resolution: "         << resolution_        << "\n"
        << "origin: ["
            << origin_[0] << ", " << origin_[1] << ", " << origin_[2] << "]\n"
        << "width: "              << width_             << "\n"
        << "depth: "              << depth_             << "\n"
        << "height: "             << height_            << "\n"
        << "sub_map_resolution: " << subMapResolution_  << "\n"
        << "sub_map_width: "      << subMapWidth_       << "\n"
        << "sub_map_depth: "      << subMapDepth_       << "\n"
        << "sub_map_height: "     << subMapHeight_      << "\n";
    return ofs.good();
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — getDistance  (key method for MCL likelihood)
// ═══════════════════════════════════════════════════════════════════════════════

float DistanceField::getDistance(float x, float y, float z) const
{
    // Main-grid lookup
    int u, v, w;
    if (!xyz2uvw(origin_, resolution_, width_, depth_, height_, x, y, z, &u, &v, &w))
        return maxDistance_;
    if (counts_[u][v][w] == 0)
        return maxDistance_;

    // Sub-map lookup
    int uu, vv, ww;
    if (!xyz2uvw(origins_[u][v][w], subMapResolution_,
                 subMapWidth_, subMapDepth_, subMapHeight_,
                 x, y, z, &uu, &vv, &ww))
        return maxDistance_;

    const uint32_t val = cells_[u][v][w][subIdx(uu, vv, ww)];
    if (val == MAP_VAL_INVALID) return maxDistance_;

    const float dist = dists_[u][v][w][val];
    return (dist < 0.f) ? maxDistance_ : dist;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  DistanceField — getMapPoints
// ═══════════════════════════════════════════════════════════════════════════════

std::vector<Point> DistanceField::getMapPoints(
    float minX, float maxX,
    float minY, float maxY,
    float minZ, float maxZ) const
{
    std::vector<Point> pts;
    for (int u = 0; u < width_; ++u) {
        for (int v = 0; v < depth_; ++v) {
            for (int w = 0; w < height_; ++w) {
                if (counts_[u][v][w] == 0) continue;
                for (int uu = 0; uu < subMapWidth_; ++uu) {
                    for (int vv = 0; vv < subMapDepth_; ++vv) {
                        for (int ww = 0; ww < subMapHeight_; ++ww) {
                            if (cells_[u][v][w][subIdx(uu, vv, ww)]
                                    != MAP_VAL_EXIST_POINT)
                                continue;
                            float px, py, pz;
                            uvw2xyz(origins_[u][v][w], subMapResolution_,
                                    uu, vv, ww, &px, &py, &pz);
                            if (px >= minX && px <= maxX &&
                                py >= minY && py <= maxY &&
                                pz >= minZ && pz <= maxZ)
                                pts.emplace_back(px, py, pz);
                        }
                    }
                }
            }
        }
    }
    return pts;
}

}  // namespace mcl3d
