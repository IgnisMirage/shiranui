#pragma once

#include <string>

namespace mcl3d {

struct Point {
    float x, y, z;
    Point() : x(0.f), y(0.f), z(0.f) {}
    Point(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
};

struct DistanceFieldParams {
    std::string pcdFile;
    std::string mapFileName;
    std::string yamlFilePath;
    float resolution        = 5.0f;
    float subMapResolution  = 0.1f;
    float mapMargin         = 1.0f;
    float voxelLeafSize     = 0.1f;
};

struct GridMapParams {
    std::string pcdFile;
    std::string mapPointsTopic = "/map_points";
    float voxelLeafSize        = 0.1f;
};


struct MCL3DParams {
    int particleNum = 1000;
    int localizationMode = 1; // 0: no particles, 1: full MCL, 2: pose-only, 3: pose+yaw
};

struct Pose {
    double x;
    double y;
    double z;
    double roll;
    double pitch;
    double yaw;
};


} // namespace mcl3d
