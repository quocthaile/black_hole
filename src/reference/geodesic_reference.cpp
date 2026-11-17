#include "geodesic_reference.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <limits>

namespace reference {
namespace {

struct EventCandidate {
    bool found;
    double fraction;
    Termination termination;
};

// Test all coordinates of a ray state for finite double-precision values.
bool isFiniteState(const State& state)
{
    return std::isfinite(state.radius) && std::isfinite(state.angle) &&
           std::isfinite(state.radialVelocity);
}

// Return the Euclidean length of a vector in the model's spatial coordinates.
double vectorLength(const Vec3& value)
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

// Normalize a spatial vector; false identifies a zero or non-finite input.
bool normalizeVector(const Vec3& value, Vec3& normalized)
{
    const double length = vectorLength(value);
    if (!(length > 0.0) || !std::isfinite(length)) {
        return false;
    }
    normalized = {value.x / length, value.y / length, value.z / length};
    return true;
}

// Compute a three-dimensional dot product in the common model coordinate basis.
double dot(const Vec3& left, const Vec3& right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

// Combine a state and its derivative for an intermediate RK4 stage.
State addScaled(const State& state, const State& derivative, double scale)
{
    return {state.radius + scale * derivative.radius,
            state.angle + scale * derivative.angle,
            state.radialVelocity + scale * derivative.radialVelocity};
}

// Add a possible segment event while retaining the event with the earliest fraction.
void keepEarlierEvent(EventCandidate& selected, double fraction, Termination termination)
{
    if (fraction < 0.0 || fraction > 1.0 || !std::isfinite(fraction)) {
        return;
    }
    if (!selected.found || fraction < selected.fraction) {
        selected = {true, fraction, termination};
    }
}

// Find the first segment intersection with a centered sphere in 3D model coordinates.
bool segmentSphereFraction(const Vec3& start, const Vec3& end, const Vec3& center,
                          double radius, double& fraction)
{
    const Vec3 delta{end.x - start.x, end.y - start.y, end.z - start.z};
    const Vec3 offset{start.x - center.x, start.y - center.y, start.z - center.z};
    const double a = dot(delta, delta);
    const double b = 2.0 * dot(offset, delta);
    const double c = dot(offset, offset) - radius * radius;
    if (!(a > 0.0)) {
        return false;
    }
    const double discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0 || !std::isfinite(discriminant)) {
        return false;
    }
    const double root = std::sqrt(discriminant);
    const double first = (-b - root) / (2.0 * a);
    const double second = (-b + root) / (2.0 * a);
    if (first >= 0.0 && first <= 1.0) {
        fraction = first;
        return true;
    }
    if (second >= 0.0 && second <= 1.0) {
        fraction = second;
        return true;
    }
    return false;
}

// Locate the earliest horizon, disk-annulus, object-sphere, or escape intersection on a segment.
EventCandidate findSegmentEvent(const ModelParameters& model, const IntegrationConfig& config,
                                const Vec3& start, const Vec3& end)
{
    EventCandidate selected{false, 1.0, Termination::MaximumSteps};
    double fraction = 0.0;
    const Vec3 origin{0.0, 0.0, 0.0};

    if (model.schwarzschildRadius > 0.0 &&
        segmentSphereFraction(start, end, origin, model.schwarzschildRadius, fraction)) {
        keepEarlierEvent(selected, fraction, Termination::EventHorizon);
    }
    if (config.escapeRadius > 0.0 &&
        segmentSphereFraction(start, end, origin, config.escapeRadius, fraction)) {
        const double startRadius = vectorLength(start);
        const double endRadius = vectorLength(end);
        if (endRadius > startRadius) {
            keepEarlierEvent(selected, fraction, Termination::Escaped);
        }
    }

    const Vec3 normal = model.diskNormal;
    const Vec3 segment{end.x - start.x, end.y - start.y, end.z - start.z};
    const double startPlane = dot(start, normal);
    const double endPlane = dot(end, normal);
    if (startPlane * endPlane <= 0.0 && startPlane != endPlane) {
        const double diskFraction = startPlane / (startPlane - endPlane);
        const Vec3 diskPoint{start.x + diskFraction * segment.x,
                             start.y + diskFraction * segment.y,
                             start.z + diskFraction * segment.z};
        const double axial = dot(diskPoint, normal);
        const Vec3 inPlane{diskPoint.x - axial * normal.x,
                           diskPoint.y - axial * normal.y,
                           diskPoint.z - axial * normal.z};
        const double diskRadius = vectorLength(inPlane);
        if (diskRadius >= model.diskInnerRadius && diskRadius <= model.diskOuterRadius) {
            keepEarlierEvent(selected, diskFraction, Termination::DiskHit);
        }
    }

    for (const Sphere& object : model.objects) {
        const Vec3 offset{start.x - object.center.x, start.y - object.center.y,
                          start.z - object.center.z};
        if (dot(offset, offset) <= object.radius * object.radius) {
            keepEarlierEvent(selected, 0.0, Termination::ObjectHit);
        } else if (segmentSphereFraction(start, end, object.center, object.radius, fraction)) {
            keepEarlierEvent(selected, fraction, Termination::ObjectHit);
        }
    }
    return selected;
}

// Interpolate a state to a located segment event for reporting and trajectory output.
State interpolateState(const State& start, const State& end, double fraction)
{
    return {start.radius + fraction * (end.radius - start.radius),
            start.angle + fraction * (end.angle - start.angle),
            start.radialVelocity + fraction * (end.radialVelocity - start.radialVelocity)};
}

// Write a JSON string with basic escaping for reproducibility metadata.
void writeJsonString(std::ofstream& output, const std::string& value)
{
    output << '"';
    for (char character : value) {
        if (character == '"' || character == '\\') {
            output << '\\' << character;
        } else if (character == '\n') {
            output << "\\n";
        } else if (character == '\r') {
            output << "\\r";
        } else {
            output << character;
        }
    }
    output << '"';
}

} // namespace

// Create a ray from the local static-observer frame and normalize it to Killing energy E=1.
bool initializeRay(const ModelParameters& model, const InitialConditions& initial, Ray& ray)
{
    if (!(initial.cameraRadius > model.schwarzschildRadius) ||
        (initial.radialSign != -1 && initial.radialSign != 1) ||
        !std::isfinite(initial.cameraRadius) || !std::isfinite(initial.impactParameter) ||
        !std::isfinite(model.schwarzschildRadius)) {
        return false;
    }
    Vec3 radialDirection{};
    Vec3 tangentDirection{};
    if (!normalizeVector(initial.radialDirection, radialDirection) ||
        !normalizeVector(initial.tangentDirection, tangentDirection) ||
        std::abs(dot(radialDirection, tangentDirection)) > 1.0e-12) {
        return false;
    }
    const double lapseSquared = 1.0 - model.schwarzschildRadius / initial.cameraRadius;
    if (!(lapseSquared > 0.0)) {
        return false;
    }

    const double transverse = initial.impactParameter * std::sqrt(lapseSquared) /
                              initial.cameraRadius;
    if (std::abs(transverse) > 1.0) {
        return false;
    }
    const double radial = static_cast<double>(initial.radialSign) *
                          std::sqrt(std::max(0.0, 1.0 - transverse * transverse));
    ray.initialRadial = radialDirection;
    ray.initialTangent = tangentDirection;
    ray.state = {initial.cameraRadius, 0.0, radial};
    ray.angularMomentum = initial.impactParameter;
    ray.energy = 1.0;
    ray.affineParameter = 0.0;
    return true;
}

// Evaluate the null-constrained planar geodesic RHS using the differentiated first integral.
bool geodesicRHS(const State& state, double angularMomentum,
                 double schwarzschildRadius, State& derivative)
{
    if (!(state.radius > 0.0) || !isFiniteState(state) ||
        !std::isfinite(angularMomentum) || !std::isfinite(schwarzschildRadius)) {
        return false;
    }
    const double radiusSquared = state.radius * state.radius;
    derivative.radius = state.radialVelocity;
    derivative.angle = angularMomentum / radiusSquared;
    derivative.radialVelocity = angularMomentum * angularMomentum /
                                (radiusSquared * state.radius) *
                                (1.0 - 1.5 * schwarzschildRadius / state.radius);
    return isFiniteState(derivative);
}

// Advance a planar null-geodesic state by classical RK4 with exactly four RHS evaluations.
bool rk4Step(const State& state, double stepSize, double angularMomentum,
             double schwarzschildRadius, State& nextState)
{
    if (!(stepSize > 0.0) || !std::isfinite(stepSize)) {
        return false;
    }
    State k1{};
    State k2{};
    State k3{};
    State k4{};
    if (!geodesicRHS(state, angularMomentum, schwarzschildRadius, k1)) {
        return false;
    }
    const State stage2 = addScaled(state, k1, 0.5 * stepSize);
    if (!geodesicRHS(stage2, angularMomentum, schwarzschildRadius, k2)) {
        return false;
    }
    const State stage3 = addScaled(state, k2, 0.5 * stepSize);
    if (!geodesicRHS(stage3, angularMomentum, schwarzschildRadius, k3)) {
        return false;
    }
    const State stage4 = addScaled(state, k3, stepSize);
    if (!geodesicRHS(stage4, angularMomentum, schwarzschildRadius, k4)) {
        return false;
    }
    nextState.radius = state.radius + stepSize / 6.0 *
        (k1.radius + 2.0 * k2.radius + 2.0 * k3.radius + k4.radius);
    nextState.angle = state.angle + stepSize / 6.0 *
        (k1.angle + 2.0 * k2.angle + 2.0 * k3.angle + k4.angle);
    nextState.radialVelocity = state.radialVelocity + stepSize / 6.0 *
        (k1.radialVelocity + 2.0 * k2.radialVelocity +
         2.0 * k3.radialVelocity + k4.radialVelocity);
    return isFiniteState(nextState);
}

// Reconstruct the spatial Cartesian position from the ray's plane basis and in-plane angle.
Vec3 positionAt(const Ray& ray, const State& state)
{
    const double radialScale = state.radius * std::cos(state.angle);
    const double tangentScale = state.radius * std::sin(state.angle);
    return {radialScale * ray.initialRadial.x + tangentScale * ray.initialTangent.x,
            radialScale * ray.initialRadial.y + tangentScale * ray.initialTangent.y,
            radialScale * ray.initialRadial.z + tangentScale * ray.initialTangent.z};
}

// Evaluate the null residual normalized by the sum of absolute metric contributions.
double nullConstraintResidual(const State& state, double energy,
                              double angularMomentum, double schwarzschildRadius)
{
    const double lapse = 1.0 - schwarzschildRadius / state.radius;
    if (!(state.radius > schwarzschildRadius) || !(lapse > 0.0) ||
        !isFiniteState(state) || !std::isfinite(energy) || !std::isfinite(angularMomentum)) {
        return std::numeric_limits<double>::infinity();
    }
    const double timeTerm = energy * energy / lapse;
    const double radialTerm = state.radialVelocity * state.radialVelocity / lapse;
    const double angularTerm = angularMomentum * angularMomentum /
                               (state.radius * state.radius);
    const double scale = timeTerm + radialTerm + angularTerm;
    if (!(scale > 0.0) || !std::isfinite(scale)) {
        return std::numeric_limits<double>::infinity();
    }
    return std::abs(-timeTerm + radialTerm + angularTerm) / scale;
}

// Integrate fixed RK4 steps and stop at the earliest segment event or configured limit.
Result integrate(const ModelParameters& model, const IntegrationConfig& config, Ray ray)
{
    Result result{ray, Termination::MaximumSteps, 0, 1.0, {}};
    if (!(config.stepSize > 0.0) || config.maximumSteps < 0 ||
        !(config.escapeRadius > model.schwarzschildRadius) || !isFiniteState(ray.state)) {
        result.termination = Termination::InvalidState;
        return result;
    }
    result.trajectory.push_back({ray.affineParameter, ray.state,
                                 positionAt(ray, ray.state),
                                 nullConstraintResidual(ray.state, ray.energy,
                                                        ray.angularMomentum,
                                                        model.schwarzschildRadius)});
    for (int step = 0; step < config.maximumSteps; ++step) {
        State next{};
        if (!rk4Step(ray.state, config.stepSize, ray.angularMomentum,
                     model.schwarzschildRadius, next)) {
            result.termination = Termination::InvalidState;
            result.steps = step;
            result.ray = ray;
            return result;
        }
        const Vec3 startPosition = positionAt(ray, ray.state);
        const Vec3 endPosition = positionAt(ray, next);
        const EventCandidate event = findSegmentEvent(model, config, startPosition, endPosition);
        ++result.steps;
        if (event.found) {
            ray.state = interpolateState(ray.state, next, event.fraction);
            ray.affineParameter += config.stepSize * event.fraction;
            result.eventFraction = event.fraction;
            result.termination = event.termination;
            result.ray = ray;
            result.trajectory.push_back({ray.affineParameter, ray.state,
                                         positionAt(ray, ray.state),
                                         nullConstraintResidual(ray.state, ray.energy,
                                                                ray.angularMomentum,
                                                                model.schwarzschildRadius)});
            return result;
        }
        ray.state = next;
        ray.affineParameter += config.stepSize;
        result.trajectory.push_back({ray.affineParameter, ray.state, endPosition,
                                     nullConstraintResidual(ray.state, ray.energy,
                                                            ray.angularMomentum,
                                                            model.schwarzschildRadius)});
    }
    result.ray = ray;
    if (vectorLength(positionAt(ray, ray.state)) >= config.escapeRadius) {
        result.termination = Termination::Escaped;
    }
    return result;
}

// Return a stable lowercase event name for CSV and metadata output.
const char* terminationName(Termination termination)
{
    switch (termination) {
    case Termination::EventHorizon: return "event_horizon";
    case Termination::DiskHit: return "disk_hit";
    case Termination::ObjectHit: return "object_hit";
    case Termination::Escaped: return "escaped";
    case Termination::MaximumSteps: return "maximum_steps";
    case Termination::InvalidState: return "invalid_state";
    }
    return "invalid_state";
}

// Export dimensionless trajectory samples and null residuals in a stable CSV column order.
bool writeTrajectoryCsv(const std::string& path, const Result& result)
{
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << "affine_parameter,r,angle,dr_dsigma,x,y,z,null_residual\n";
    output << std::setprecision(17);
    for (const TrajectoryPoint& point : result.trajectory) {
        output << point.affineParameter << ',' << point.state.radius << ','
               << point.state.angle << ',' << point.state.radialVelocity << ','
               << point.position.x << ',' << point.position.y << ','
               << point.position.z << ',' << point.nullResidual << '\n';
    }
    return static_cast<bool>(output);
}

// Export input parameters, fixed-step configuration, and measured termination metadata as JSON.
bool writeMetadataJson(const std::string& path, const ModelParameters& model,
                       const InitialConditions& initial, const IntegrationConfig& config,
                       const Result& result)
{
    std::ofstream output(path, std::ios::out | std::ios::trunc);
    if (!output) {
        return false;
    }
    output << std::setprecision(17)
           << "{\n  \"coordinate_units\": \"lengths divided by L0; affine parameter sigma\",\n"
           << "  \"metric\": \"Schwarzschild, signature (-,+,+,+)\",\n"
           << "  \"energy_normalization\": " << result.ray.energy << ",\n"
           << "  \"schwarzschild_radius\": " << model.schwarzschildRadius << ",\n"
           << "  \"camera_radius\": " << initial.cameraRadius << ",\n"
           << "  \"impact_parameter\": " << initial.impactParameter << ",\n"
           << "  \"radial_sign\": " << initial.radialSign << ",\n"
           << "  \"radial_direction\": [" << initial.radialDirection.x << ','
           << initial.radialDirection.y << ',' << initial.radialDirection.z << "],\n"
           << "  \"tangent_direction\": [" << initial.tangentDirection.x << ','
           << initial.tangentDirection.y << ',' << initial.tangentDirection.z << "],\n"
           << "  \"disk_inner_radius\": " << model.diskInnerRadius << ",\n"
           << "  \"disk_outer_radius\": " << model.diskOuterRadius << ",\n"
           << "  \"disk_normal\": [" << model.diskNormal.x << ','
           << model.diskNormal.y << ',' << model.diskNormal.z << "],\n"
           << "  \"object_count\": " << model.objects.size() << ",\n"
           << "  \"step_size\": " << config.stepSize << ",\n"
           << "  \"maximum_steps\": " << config.maximumSteps << ",\n"
           << "  \"escape_radius\": " << config.escapeRadius << ",\n"
           << "  \"termination\": ";
    writeJsonString(output, terminationName(result.termination));
    output << ",\n  \"steps\": " << result.steps
           << ",\n  \"trajectory_samples\": " << result.trajectory.size()
           << ",\n  \"event_fraction\": " << result.eventFraction
           << ",\n  \"final_null_residual\": ";
    if (result.trajectory.empty()) {
        output << "null";
    } else {
        const double finalResidual = result.trajectory.back().nullResidual;
        if (std::isfinite(finalResidual)) {
            output << finalResidual;
        } else {
            output << "null";
        }
    }
    output << "\n}\n";
    return static_cast<bool>(output);
}

} // namespace reference
