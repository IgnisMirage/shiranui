#include <mcl3d_ros/IMU.h>

namespace mcl3d {

void IMU::init(void) {
    q0_ = 1.0;
    q1_ = 0.0;
    q2_ = 0.0;
    q3_ = 0.0;

    roll_ = 0.0;
    pitch_ = 0.0;
    yaw_ = 0.0;

    twoKp_ = 0.0;
    twoKi_ = 0.0;
    integralFBx_ = 0.0;
    integralFBy_ = 0.0;
    integralFBz_ = 0.0;

    beta_ = 0.1;

    filterType_ = MAHONY;

    sampleFreq_ = 100.0;
}

void IMU::updateOrientation(void) {
    if (filterType_ == MAHONY)
        updateOrientationMahony();
    else
        updateOrientationMadgwick();
}

void IMU::updateOrientationMahony(void) {
    if (ax_ != 0.0 || ay_ != 0.0 || az_ != 0.0) {
        double recipNorm = invSqrt(ax_ * ax_ + ay_ * ay_ + az_ * az_);
        ax_ *= recipNorm;
        ay_ *= recipNorm;
        az_ *= recipNorm;

        double halfvx = q1_ * q3_ - q0_ * q2_;
        double halfvy = q0_ * q1_ + q2_ * q3_;
        double halfvz = q0_ * q0_ - 0.5 + q3_ * q3_;

        double halfex = (ay_ * halfvz - az_ * halfvy);
        double halfey = (az_ * halfvx - ax_ * halfvz);
        double halfez = (ax_ * halfvy - ay_ * halfvx);

        if (twoKi_ > 0.0) {
            integralFBx_ += twoKi_ * halfex * (1.0 / sampleFreq_);
            integralFBy_ += twoKi_ * halfey * (1.0 / sampleFreq_);
            integralFBz_ += twoKi_ * halfez * (1.0 / sampleFreq_);
            gx_ += integralFBx_;
            gy_ += integralFBy_;
            gz_ += integralFBz_;
        } else {
            integralFBx_ = 0.0;
            integralFBy_ = 0.0;
            integralFBz_ = 0.0;
        }

        gx_ += twoKp_ * halfex;
        gy_ += twoKp_ * halfey;
        gz_ += twoKp_ * halfez;
    }

    gx_ *= (0.5 * (1.0 / sampleFreq_));
    gy_ *= (0.5 * (1.0 / sampleFreq_));
    gz_ *= (0.5 * (1.0 / sampleFreq_));
    double qa = q0_;
    double qb = q1_;
    double qc = q2_;
    q0_ += (-qb * gx_ - qc * gy_ - q3_ * gz_);
    q1_ += (qa * gx_ + qc * gz_ - q3_ * gy_);
    q2_ += (qa * gy_ - qb * gz_ + q3_ * gx_);
    q3_ += (qa * gz_ + qb * gy_ - qc * gx_);

    double recipNorm = invSqrt(q0_ * q0_ + q1_ * q1_ + q2_ * q2_ + q3_ * q3_);
    q0_ *= recipNorm;
    q1_ *= recipNorm;
    q2_ *= recipNorm;
    q3_ *= recipNorm;

    double q0q0 = q0_ * q0_;
    double q1q1 = q1_ * q1_;
    double q2q2 = q2_ * q2_;
    double q3q3 = q3_ * q3_;
    double q0q1 = q0_ * q1_;
    double q0q2 = q0_ * q2_;
    double q0q3 = q0_ * q3_;
    double q1q2 = q1_ * q2_;
    double q1q3 = q1_ * q3_;
    double q2q3 = q2_ * q3_;
    roll_ = atan2((2.0 * (q2q3 + q0q1)), (q0q0 - q1q1 - q2q2 + q3q3));
    pitch_ = -asin((2.0 * (q1q3 - q0q2)));
    yaw_ = atan2((2.0 * (q1q2 + q0q3)), (q0q0 + q1q1 - q2q2 - q3q3));
}

void IMU::updateOrientationMadgwick(void) {
    // quaternion derivative from gyroscope
    double qDot0 = 0.5 * (-q1_ * gx_ - q2_ * gy_ - q3_ * gz_);
    double qDot1 = 0.5 * ( q0_ * gx_ + q2_ * gz_ - q3_ * gy_);
    double qDot2 = 0.5 * ( q0_ * gy_ - q1_ * gz_ + q3_ * gx_);
    double qDot3 = 0.5 * ( q0_ * gz_ + q1_ * gy_ - q2_ * gx_);

    if (ax_ != 0.0 || ay_ != 0.0 || az_ != 0.0) {
        // normalize accelerometer
        double recipNorm = invSqrt(ax_ * ax_ + ay_ * ay_ + az_ * az_);
        double a0 = ax_ * recipNorm;
        double a1 = ay_ * recipNorm;
        double a2 = az_ * recipNorm;

        // objective function for gravity: f = R^T * g_world - g_body
        double f0 = 2.0 * (q1_ * q3_ - q0_ * q2_) - a0;
        double f1 = 2.0 * (q0_ * q1_ + q2_ * q3_) - a1;
        double f2 = 2.0 * (0.5 - q1_ * q1_ - q2_ * q2_) - a2;

        // gradient = Jacobian^T * f
        double s0 = -2.0 * q2_ * f0 + 2.0 * q1_ * f1;
        double s1 =  2.0 * q3_ * f0 + 2.0 * q0_ * f1 - 4.0 * q1_ * f2;
        double s2 = -2.0 * q0_ * f0 + 2.0 * q3_ * f1 - 4.0 * q2_ * f2;
        double s3 =  2.0 * q1_ * f0 + 2.0 * q2_ * f1;

        // normalize gradient and subtract from quaternion derivative
        recipNorm = invSqrt(s0 * s0 + s1 * s1 + s2 * s2 + s3 * s3);
        qDot0 -= beta_ * s0 * recipNorm;
        qDot1 -= beta_ * s1 * recipNorm;
        qDot2 -= beta_ * s2 * recipNorm;
        qDot3 -= beta_ * s3 * recipNorm;
    }

    // integrate
    double dt = 1.0 / sampleFreq_;
    q0_ += qDot0 * dt;
    q1_ += qDot1 * dt;
    q2_ += qDot2 * dt;
    q3_ += qDot3 * dt;

    // normalize quaternion
    double recipNorm = invSqrt(q0_ * q0_ + q1_ * q1_ + q2_ * q2_ + q3_ * q3_);
    q0_ *= recipNorm;
    q1_ *= recipNorm;
    q2_ *= recipNorm;
    q3_ *= recipNorm;

    // quaternion to Euler angles
    double q0q0 = q0_ * q0_;
    double q1q1 = q1_ * q1_;
    double q2q2 = q2_ * q2_;
    double q3q3 = q3_ * q3_;
    roll_  = atan2(2.0 * (q2_ * q3_ + q0_ * q1_), q0q0 - q1q1 - q2q2 + q3q3);
    pitch_ = -asin(2.0 * (q1_ * q3_ - q0_ * q2_));
    yaw_   = atan2(2.0 * (q1_ * q2_ + q0_ * q3_), q0q0 + q1q1 - q2q2 - q3q3);
}

} // namespace mcl3d