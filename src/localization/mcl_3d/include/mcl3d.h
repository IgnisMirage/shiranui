#pragma once

class MCL3D {
public:
    MCL3D();
    ~MCL3D();

    void init_particles();
    void update();
    void resample();
    void estimate();
    void likelihoodFieldModel();
    void resamplingParticles();
};
