#pragma once

// Exponential-moving-average FPS tracker. Replaces the raw 1/deltaTime
// display, which visibly flickers frame to frame since a single frame's
// timing is noisy; the EMA trades a little responsiveness for a stable
// number, both for the on-screen readout and for benchmark threshold
// detection (see Application.cpp's --benchmark mode).
class FrameStats {
public:
    // decay closer to 1.0 = smoother but slower to react; 0.9 was chosen as a
    // readable middle ground (roughly a ~10-frame effective averaging window).
    explicit FrameStats(float decay = 0.9f) : decay(decay) {}

    void Update(float deltaTime) {
        float instantFPS = deltaTime > 0.0f ? 1.0f / deltaTime : 0.0f;
        if (!initialized) {
            smoothedFPS = instantFPS; // seed from the first sample, no ramp-in bias
            initialized = true;
        } else {
            smoothedFPS = decay * smoothedFPS + (1.0f - decay) * instantFPS;
        }
    }

    float GetSmoothedFPS() const { return smoothedFPS; }

private:
    float decay;
    float smoothedFPS = 0.0f;
    bool initialized = false;
};
