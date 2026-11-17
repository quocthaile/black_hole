#include "geodesic_reference.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>

namespace {

// Stop the test executable with a readable message when a numerical assertion fails.
void require(bool condition, const std::string& message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

// Build the empty event model used by analytic orbit tests.
reference::ModelParameters emptyModel(double schwarzschildRadius)
{
    return {schwarzschildRadius, 2.2, 5.2, {0.0, 1.0, 0.0}, {}};
}

// Measure a scaled distance between two final states at the same affine parameter.
double stateDifference(const reference::State& left, const reference::State& right)
{
    return std::abs(left.radius - right.radius) +
           std::abs(left.angle - right.angle) +
           std::abs(left.radialVelocity - right.radialVelocity);
}

// Check local-frame initialization, nullness, and normalized Killing constants.
void testNullInitialization()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray ray{};
    require(reference::initializeRay(model, {20.0, 4.0, -1}, ray),
            "local observer initialization succeeds outside the horizon");
    require(ray.energy == 1.0, "Killing energy is normalized to one");
    require(ray.angularMomentum == 4.0, "impact parameter equals |L|/E");
    require(reference::nullConstraintResidual(ray.state, ray.energy,
                                              ray.angularMomentum, 1.0) < 2.0e-15,
            "initialized camera ray satisfies the null constraint");
    require(!reference::initializeRay(model, {1.0, 0.0, -1}, ray),
            "static observer initialization rejects the horizon");
}

// Check the exactly linear radial null geodesic and its horizon event location.
void testRadialNullGeodesic()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray ray{};
    require(reference::initializeRay(model, {10.0, 0.0, -1}, ray),
            "radial inward ray initializes");
    const reference::Result result = reference::integrate(model, {0.1, 200, 100.0}, ray);
    require(result.termination == reference::Termination::EventHorizon,
            "radial ray reaches the horizon");
    require(std::abs(result.ray.state.radius - 1.0) < 1.0e-12,
            "radial horizon radius is located by segment intersection");
    require(std::abs(result.ray.affineParameter - 9.0) < 1.0e-10,
            "radial affine horizon location matches the analytic solution");
}

// Check the unstable circular photon orbit at r=3 rs/2 and b=3 sqrt(3) rs/2.
void testPhotonSphere()
{
    const double criticalImpact = 1.5 * std::sqrt(3.0);
    reference::State state{1.5, 0.0, 0.0};
    reference::State derivative{};
    require(reference::geodesicRHS(state, criticalImpact, 1.0, derivative),
            "photon-sphere RHS is finite");
    require(std::abs(derivative.radialVelocity) < 1.0e-14,
            "circular orbit has zero radial velocity derivative");
    reference::State next{};
    require(reference::rk4Step(state, 0.01, criticalImpact, 1.0, next),
            "photon-sphere RK4 step succeeds");
    require(std::abs(next.radius - 1.5) < 1.0e-14,
            "critical circular orbit retains constant radius");
}

// Check the flat-spacetime limit against a straight line with geometric impact distance b.
void testFlatSpacetimeLimit()
{
    const reference::ModelParameters model = emptyModel(0.0);
    reference::Ray ray{};
    require(reference::initializeRay(model, {20.0, 2.0, -1,
                                              {1.0, 0.0, 0.0},
                                              {0.0, 0.6, 0.8}}, ray),
            "flat-space ray initializes");
    const reference::Result result = reference::integrate(model, {0.01, 5000, 30.0}, ray);
    require(result.termination == reference::Termination::Escaped,
            "outward segment crossing the escape radius is classified escaped");
    double smallestRadius = 1.0e9;
    reference::Vec3 closest{};
    for (const reference::TrajectoryPoint& point : result.trajectory) {
        if (point.state.radius < smallestRadius) {
            smallestRadius = point.state.radius;
            closest = point.position;
        }
    }
    require(std::abs(smallestRadius - 2.0) < 5.0e-4,
            "flat ray's closest approach equals its impact parameter");
    const double directionX = -std::sqrt(0.99);
    const double crossX = closest.y * 0.08 - closest.z * 0.06;
    const double crossY = closest.z * directionX - closest.x * 0.08;
    const double crossZ = closest.x * 0.06 - closest.y * directionX;
    const double impactDistance = std::sqrt(crossX * crossX + crossY * crossY + crossZ * crossZ);
    require(std::abs(impactDistance - 2.0) < 5.0e-4,
            "flat ray remains on the expected straight line at finite camera radius");
    require(std::abs(closest.z / closest.y - 4.0 / 3.0) < 5.0e-4,
            "orbital-plane reconstruction preserves the general 3D tangent direction");
}

// Compare a weak-field scattering angle with the leading 4GM/(bc^2) result.
void testWeakFieldDeflection()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray ray{};
    const double initialRadius = 200.0;
    const double impact = 30.0;
    require(reference::initializeRay(model, {initialRadius, impact, -1}, ray),
            "weak-field scattering ray initializes");
    const reference::Result result = reference::integrate(model, {0.01, 50000, initialRadius}, ray);
    require(result.termination == reference::Termination::Escaped,
            "weak-field ray scatters and escapes");
    const double flatSweep = std::acos(-1.0) -
                             2.0 * std::asin(impact / initialRadius);
    const double measuredDeflection = result.ray.state.angle - flatSweep;
    const double leadingPrediction = 2.0 / impact;
    const double relativeError = std::abs(measuredDeflection - leadingPrediction) /
                                 leadingPrediction;
    std::cout << "weak_field_deflection=" << measuredDeflection
              << " leading_prediction=" << leadingPrediction
              << " relative_error=" << relativeError << '\n';
    require(relativeError < 0.08,
            "weak-field bending agrees with the leading analytic prediction within 8 percent");
}

// Check that impact parameters on either side of the critical value produce capture/scatter.
void testCriticalImpactClassification()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray capturedRay{};
    reference::Ray scatteredRay{};
    require(reference::initializeRay(model, {20.0, 2.50, -1}, capturedRay),
            "subcritical ray initializes");
    require(reference::initializeRay(model, {20.0, 2.70, -1}, scatteredRay),
            "supercritical ray initializes");
    const reference::IntegrationConfig config{0.002, 150000, 100.0};
    const reference::Result captured = reference::integrate(model, config, capturedRay);
    const reference::Result scattered = reference::integrate(model, config, scatteredRay);
    require(captured.termination == reference::Termination::EventHorizon,
            "subcritical inward ray is captured");
    require(scattered.termination == reference::Termination::Escaped,
            "supercritical inward ray turns and escapes");
}

// Verify a planar disk crossing and a distinct first sphere intersection.
void testDiskAndObjectEvents()
{
    reference::ModelParameters diskModel{0.0, 2.5, 3.5, {1.0, 0.0, 0.0}, {}};
    reference::Ray diskRay{};
    require(reference::initializeRay(diskModel, {10.0, 3.0, -1}, diskRay),
            "disk test ray initializes");
    const reference::Result disk = reference::integrate(diskModel, {0.01, 3000, 50.0}, diskRay);
    require(disk.termination == reference::Termination::DiskHit,
            "disk annulus crossing is detected");
    const double expectedPlaneCrossing = 10.0 / std::sqrt(1.0 - 0.3 * 0.3);
    require(std::abs(disk.ray.affineParameter - expectedPlaneCrossing) < 0.02,
            "disk plane event is located within one fixed step");

    reference::ModelParameters objectModel = emptyModel(0.0);
    objectModel.objects.push_back({{5.0, 0.0, 0.0}, 1.0});
    reference::Ray objectRay{};
    require(reference::initializeRay(objectModel, {10.0, 0.0, -1}, objectRay),
            "object test ray initializes");
    const reference::Result object = reference::integrate(objectModel, {0.1, 100, 50.0}, objectRay);
    require(object.termination == reference::Termination::ObjectHit,
            "first segment/sphere intersection is detected");
    require(std::abs(object.ray.state.radius - 6.0) < 1.0e-10,
            "object surface event is located at the sphere entry point");
}

// Check global fourth-order convergence against a much finer fixed-step solution.
void testRk4Convergence()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray ray{};
    require(reference::initializeRay(model, {20.0, 5.0, -1}, ray),
            "convergence ray initializes");
    const auto finalState = [&](double stepSize) {
        reference::State state = ray.state;
        const int stepCount = static_cast<int>(8.0 / stepSize + 0.5);
        for (int index = 0; index < stepCount; ++index) {
            reference::State next{};
            require(reference::rk4Step(state, stepSize, ray.angularMomentum, 1.0, next),
                    "convergence RK4 stage remains finite");
            state = next;
        }
        return state;
    };
    const reference::State coarse = finalState(0.16);
    const reference::State medium = finalState(0.08);
    const reference::State refined = finalState(0.04);
    const reference::State finer = finalState(0.02);
    const reference::State fine = finalState(0.00125);
    const double coarseError = stateDifference(coarse, fine);
    const double mediumError = stateDifference(medium, fine);
    const double refinedError = stateDifference(refined, fine);
    const double finerError = stateDifference(finer, fine);
    std::cout << "rk4_coarse_error=" << coarseError
              << " rk4_medium_error=" << mediumError
              << " rk4_refined_error=" << refinedError
              << " rk4_finer_error=" << finerError
              << " ratios=" << coarseError / mediumError << ','
              << mediumError / refinedError << ','
              << refinedError / finerError << '\n';
    require(mediumError > 0.0 && coarseError / mediumError > 10.0,
            "halving RK4 step reduces smooth-trajectory error at near fourth order");
}

// Check that the accumulated null residual remains small for a smooth exterior trajectory.
void testNullConstraintAlongTrajectory()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray ray{};
    require(reference::initializeRay(model, {20.0, 5.0, -1}, ray),
            "invariant test ray initializes");
    const reference::Result result = reference::integrate(model, {0.01, 5000, 100.0}, ray);
    double maximumResidual = 0.0;
    for (const reference::TrajectoryPoint& point : result.trajectory) {
        maximumResidual = std::max(maximumResidual, point.nullResidual);
    }
    std::cout << "maximum_null_residual=" << maximumResidual << '\n';
    require(maximumResidual < 2.0e-5,
            "normalized null-constraint residual stays below the frozen smooth-orbit tolerance");
    require(result.ray.energy == 1.0 && result.ray.angularMomentum == 5.0,
            "Killing E and plane L remain fixed parameters through integration");
}

// Verify explicit maximum-step and invalid-configuration termination statuses.
void testTerminationStatuses()
{
    const reference::ModelParameters model = emptyModel(1.0);
    reference::Ray ray{};
    require(reference::initializeRay(model, {20.0, 5.0, -1}, ray),
            "termination-status ray initializes");
    const reference::Result limited = reference::integrate(model, {0.01, 1, 100.0}, ray);
    require(limited.termination == reference::Termination::MaximumSteps && limited.steps == 1,
            "maximum-step completion has a distinct status");
    const reference::Result invalid = reference::integrate(
        model, {std::numeric_limits<double>::quiet_NaN(), 1, 100.0}, ray);
    require(invalid.termination == reference::Termination::InvalidState,
            "non-finite integration parameters have an invalid-state status");
    reference::InitialConditions invalidBasis{20.0, 1.0, -1, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0}};
    require(!reference::initializeRay(model, invalidBasis, ray),
            "zero camera basis is rejected instead of producing a ray");
}

} // namespace

// Execute all deterministic R0 analytical, convergence, invariant, and event checks.
int main()
{
    testNullInitialization();
    testRadialNullGeodesic();
    testPhotonSphere();
    testFlatSpacetimeLimit();
    testWeakFieldDeflection();
    testCriticalImpactClassification();
    testDiskAndObjectEvents();
    testRk4Convergence();
    testNullConstraintAlongTrajectory();
    testTerminationStatuses();
    std::cout << "PASS: 10 R0 numerical validation groups\n";
    return EXIT_SUCCESS;
}
