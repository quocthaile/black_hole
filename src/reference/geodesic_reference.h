#ifndef BLACK_HOLE_GEODESIC_REFERENCE_H
#define BLACK_HOLE_GEODESIC_REFERENCE_H

#include <string>
#include <vector>

namespace reference {

struct Vec3 {
    double x;
    double y;
    double z;
};

struct Sphere {
    Vec3 center;
    double radius;
};

struct ModelParameters {
    double schwarzschildRadius;
    double diskInnerRadius;
    double diskOuterRadius;
    Vec3 diskNormal;
    std::vector<Sphere> objects;
};

struct IntegrationConfig {
    double stepSize;
    int maximumSteps;
    double escapeRadius;
};

struct InitialConditions {
    double cameraRadius;
    double impactParameter; // signed L/E relative to the supplied oriented plane basis
    int radialSign;
    Vec3 radialDirection = {1.0, 0.0, 0.0};
    Vec3 tangentDirection = {0.0, 1.0, 0.0};
};

struct State {
    double radius;
    double angle;
    double radialVelocity;
};

struct Ray {
    State state;
    Vec3 initialRadial;
    Vec3 initialTangent;
    double angularMomentum;
    double energy;
    double affineParameter;
};

enum class Termination {
    EventHorizon,
    DiskHit,
    ObjectHit,
    Escaped,
    MaximumSteps,
    InvalidState
};

struct TrajectoryPoint {
    double affineParameter;
    State state;
    Vec3 position;
    double nullResidual;
};

struct Result {
    Ray ray;
    Termination termination;
    int steps;
    double eventFraction;
    std::vector<TrajectoryPoint> trajectory;
};

// Create a camera ray from a static local observer. Lengths use the model's common coordinate unit.
// impactParameter is L/E, radialSign is +1 for outward and -1 for inward propagation.
// Returns false when the camera is at or inside the horizon or the requested direction is invalid.
bool initializeRay(const ModelParameters& model, const InitialConditions& initial, Ray& ray);

// Compute d(radius)/d(sigma), d(angle)/d(sigma), and d2(radius)/d(sigma) for a null ray.
// The state uses Schwarzschild areal radius and an orbital-plane angle; energy and angular momentum are constants.
// Returns false when the RHS would be evaluated outside the supported exterior domain.
bool geodesicRHS(const State& state, double angularMomentum,
                 double schwarzschildRadius, State& derivative);

// Advance one state by classical fixed-step RK4 using four RHS evaluations.
// The normalized affine step is constant; false reports an invalid intermediate stage.
bool rk4Step(const State& state, double stepSize, double angularMomentum,
             double schwarzschildRadius, State& nextState);

// Reconstruct a 3D position from the orbital-plane state and its orthonormal basis.
// Output coordinates use the same dimensionless length unit as the model.
Vec3 positionAt(const Ray& ray, const State& state);

// Evaluate the normalized null-constraint residual for a state outside the horizon.
// Returns infinity when the metric domain or input values are invalid.
double nullConstraintResidual(const State& state, double energy,
                              double angularMomentum, double schwarzschildRadius);

// Integrate until the first configured event, escape, invalid state, or step limit.
// Events are located on each straight segment between consecutive RK4 positions.
Result integrate(const ModelParameters& model, const IntegrationConfig& config, Ray ray);

// Convert a termination enum to a stable metadata string.
const char* terminationName(Termination termination);

// Write the sampled trajectory and integration outcome as CSV; returns false on I/O failure.
bool writeTrajectoryCsv(const std::string& path, const Result& result);

// Write reproducibility configuration and result metadata as JSON; returns false on I/O failure.
bool writeMetadataJson(const std::string& path, const ModelParameters& model,
                       const InitialConditions& initial, const IntegrationConfig& config,
                       const Result& result);

} // namespace reference

#endif
