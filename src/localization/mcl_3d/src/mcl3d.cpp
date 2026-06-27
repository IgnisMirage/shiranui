#include "mcl3d.h"
#include "common.h"
#include "grid_map.h"

MCL3D::MCL3D() {

}

MCL3D::~MCL3D() {
    
}

void MCL3D::init_particles(Pose initialPose, Pose initialNoise) {
    if (localizationMode_ == 0 || localizationMode_ == 3)
        return;

    // 重みを均等に初期化
    double wo = 1.0 / (double)particleNum_;
    for (int i = 0; i < particleNum_; ++i) {
        double x = initialPose.getX() + nrand(initialNoise.getX());
        double y = initialPose.getY() + nrand(initialNoise.getY());
        double z = initialPose.getZ() + nrand(initialNoise.getZ());
        double roll = modAngle(initialPose.getRoll() + nrand(initialNoise.getRoll()));
        double pitch = modAngle(initialPose.getPitch() + nrand(initialNoise.getPitch()));
        double yaw = modAngle(initialPose.getYaw() + nrand(initialNoise.getYaw()));
        particles_[i].setParticle(x, y, z, roll, pitch, yaw, wo);
    }
}


void MCL3D::estimate() {
    if (localizationMode_ == 0)
        return;

    // 重みの正規化
    double wSum = 0.0;
    for (int i = 0; i < particleNum_; ++i)
        wSum += particles_[i].getWeight();
    if (wSum > 0.0) {
        for (int i = 0; i < particleNum_; ++i)
            particles_[i].setWeight(particles_[i].getWeight() / wSum);
    }

    // 推定値の計算
    double xEst = 0.0;
    double yEst = 0.0;
    double zEst = 0.0;
    double rollEst = 0.0;
    double pitchEst = 0.0;
    double yawEst = 0.0;
    for (int i = 0; i < particleNum_; ++i) {
        xEst += particles_[i].getX() * particles_[i].getWeight();
        yEst += particles_[i].getY() * particles_[i].getWeight();
        zEst += particles_[i].getZ() * particles_[i].getWeight();
        rollEst += particles_[i].getRoll() * particles_[i].getWeight();
        pitchEst += particles_[i].getPitch() * particles_[i].getWeight();
        yawEst += particles_[i].getYaw() * particles_[i].getWeight();
    }
    estimatedPose_.setPose(xEst, yEst, zEst, rollEst, pitchEst, yawEst);
}   


vid MCL3D::likelihoodFieldModel(const Pose &pose, const std::vector<pcl::PointXYZI> &scanPoints) {
}
