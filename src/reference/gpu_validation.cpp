#include "geodesic_reference.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

constexpr int shaderMaximumSteps = 60000;

struct alignas(16) CameraBlock {
    float position[4];
    float right[4];
    float up[4];
    float forward[4];
    float settings[4];
};

struct alignas(16) DiskBlock {
    float parameters[4];
    float normal[4];
};

struct alignas(16) ObjectsBlock {
    int counts[4];
    float positionRadius[16][4];
    float color[16][4];
};

static_assert(sizeof(CameraBlock) == 80, "GPU validation camera block must match std140");
static_assert(sizeof(DiskBlock) == 32, "GPU validation disk block must match std140");
static_assert(sizeof(ObjectsBlock) == 528, "GPU validation object block must match std140");

GLuint cameraBuffer = 0;
GLuint diskBuffer = 0;
GLuint objectBuffer = 0;

// Read the B1 compute shader and enable its compile-time single-ray diagnostics.
bool loadDiagnosticShader(const std::string& path, std::string& source,
                          int maximumSteps = shaderMaximumSteps)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::ostringstream contents;
    contents << input.rdbuf();
    source = contents.str();
    const std::size_t firstLine = source.find('\n');
    if (firstLine == std::string::npos) {
        return false;
    }
    std::string definitions = "#define B1_DIAGNOSTICS 1\n";
    if (maximumSteps != shaderMaximumSteps) {
        definitions += "#define B1_MAXIMUM_STEPS " + std::to_string(maximumSteps) + "\n";
    }
    source.insert(firstLine + 1, definitions);
    return true;
}

// Compile the diagnostic compute shader and include the complete driver log on failure.
GLuint compileComputeProgram(const std::string& source)
{
    const char* sourcePointer = source.c_str();
    const GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);
    GLint compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (compiled != GL_TRUE) {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        std::cerr << "Compute shader compile failed:\n" << log.data() << '\n';
        glDeleteShader(shader);
        return 0;
    }

    const GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);
    GLint linked = GL_FALSE;
    glGetProgramiv(program, GL_LINK_STATUS, &linked);
    if (linked != GL_TRUE) {
        GLint logLength = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<char> log(static_cast<std::size_t>(std::max(logLength, 1)));
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        std::cerr << "Compute shader link failed:\n" << log.data() << '\n';
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

// Allocate and bind the three fixed-layout uniform buffers used by the B1 shader.
GLuint createUniformBuffer(GLuint binding, GLsizeiptr size, const void* data)
{
    GLuint buffer = 0;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_UNIFORM_BUFFER, buffer);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    glBindBufferBase(GL_UNIFORM_BUFFER, binding, buffer);
    return buffer;
}

// Configure one 1x1 compute dispatch for a camera ray and selected disk plane.
void setRayUniforms(double cameraRadius, double impactParameter, int radialSign,
                    const reference::Vec3& diskNormal,
                    const std::vector<reference::Sphere>& sceneObjects)
{
    const double lapse = 1.0 - 1.0 / cameraRadius;
    const double transverse = impactParameter * std::sqrt(lapse) / cameraRadius;
    const double radial = static_cast<double>(radialSign) *
                          std::sqrt(1.0 - transverse * transverse);
    const double transverseY = 0.6 * transverse;
    const double transverseZ = 0.8 * transverse;
    CameraBlock camera{};
    camera.position[0] = static_cast<float>(cameraRadius);
    camera.position[3] = 1.0f;
    camera.right[2] = -1.0f;
    camera.up[1] = 1.0f;
    camera.forward[0] = static_cast<float>(radial);
    camera.forward[1] = static_cast<float>(transverseY);
    camera.forward[2] = static_cast<float>(transverseZ);
    camera.settings[0] = 0.0f;
    camera.settings[1] = 1.0f;
    camera.settings[2] = 1.0f;
    camera.settings[3] = 1.0f;
    glBindBuffer(GL_UNIFORM_BUFFER, cameraBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(camera), &camera);

    DiskBlock disk{};
    disk.parameters[0] = 2.2f;
    disk.parameters[1] = 5.2f;
    disk.normal[0] = static_cast<float>(diskNormal.x);
    disk.normal[1] = static_cast<float>(diskNormal.y);
    disk.normal[2] = static_cast<float>(diskNormal.z);
    glBindBuffer(GL_UNIFORM_BUFFER, diskBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(disk), &disk);

    ObjectsBlock objects{};
    objects.counts[0] = static_cast<int>(std::min(sceneObjects.size(), std::size_t(16)));
    for (int index = 0; index < objects.counts[0]; ++index) {
        const reference::Sphere& sphere = sceneObjects[static_cast<std::size_t>(index)];
        objects.positionRadius[index][0] = static_cast<float>(sphere.center.x);
        objects.positionRadius[index][1] = static_cast<float>(sphere.center.y);
        objects.positionRadius[index][2] = static_cast<float>(sphere.center.z);
        objects.positionRadius[index][3] = static_cast<float>(sphere.radius);
        objects.color[index][0] = 0.8f;
        objects.color[index][1] = 0.4f;
        objects.color[index][2] = 0.2f;
        objects.color[index][3] = 1.0f;
    }
    glBindBuffer(GL_UNIFORM_BUFFER, objectBuffer);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(objects), &objects);
}

// Dispatch the B1 shader for one ray and return all sampled states and terminal metadata.
bool dispatchRay(GLuint program, double cameraRadius, double impactParameter, int radialSign,
                 const reference::Vec3& diskNormal,
                 const std::vector<reference::Sphere>& sceneObjects,
                 std::vector<float>& samples,
                 float& eventKind, int& steps, double& eventAffine,
                 int maximumSteps = shaderMaximumSteps,
                 unsigned char* outputPixel = nullptr)
{
    setRayUniforms(cameraRadius, impactParameter, radialSign, diskNormal, sceneObjects);
    const std::size_t sampleCount = static_cast<std::size_t>(maximumSteps + 1);
    GLuint diagnosticBuffer = 0;
    glGenBuffers(1, &diagnosticBuffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, diagnosticBuffer);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 static_cast<GLsizeiptr>(sampleCount * 4 * sizeof(float)), nullptr,
                 GL_DYNAMIC_READ);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, diagnosticBuffer);

    GLuint image = 0;
    glGenTextures(1, &image);
    glBindTexture(GL_TEXTURE_2D, image);
    glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1);
    glBindImageTexture(0, image, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
    glUseProgram(program);
    glDispatchCompute(1, 1, 1);
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    samples.resize(sampleCount * 4);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, diagnosticBuffer);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0,
                       static_cast<GLsizeiptr>(samples.size() * sizeof(float)), samples.data());
    const std::size_t metadata = static_cast<std::size_t>(maximumSteps) * 4;
    eventKind = samples[metadata];
    steps = static_cast<int>(samples[metadata + 1]);
    eventAffine = static_cast<double>(samples[metadata + 2]);
    if (outputPixel != nullptr) {
        glBindTexture(GL_TEXTURE_2D, image);
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, outputPixel);
    }
    const GLenum error = glGetError();
    glDeleteTextures(1, &image);
    glDeleteBuffers(1, &diagnosticBuffer);
    return error == GL_NO_ERROR;
}

// Compare a radial GPU trajectory sample-by-sample with the independent CPU R0 solution.
bool compareRadialTrajectory(GLuint program)
{
    const reference::ModelParameters model{1.0, 2.2, 5.2, {0.0, 1.0, 0.0}, {}};
    const reference::InitialConditions initial{10.0, 0.0, -1};
    const reference::IntegrationConfig config{0.01, 60000, 100.0};
    reference::Ray ray{};
    if (!reference::initializeRay(model, initial, ray)) {
        return false;
    }
    const reference::Result cpu = reference::integrate(model, config, ray);
    std::vector<float> gpuSamples;
    float gpuEvent = 0.0f;
    int gpuSteps = 0;
    double gpuEventAffine = 0.0;
    if (!dispatchRay(program, 10.0, 0.0, -1, model.diskNormal, {},
                     gpuSamples, gpuEvent, gpuSteps, gpuEventAffine)) {
        return false;
    }
    if (cpu.termination != reference::Termination::EventHorizon || gpuEvent != 1.0f ||
        std::abs(gpuSteps - cpu.steps) > 1 || gpuSamples.size() < cpu.trajectory.size() * 4) {
        std::cerr << "Radial CPU/GPU event or step count mismatch: cpu="
                  << reference::terminationName(cpu.termination) << '/' << cpu.steps
                  << " gpu=" << gpuEvent << '/' << gpuSteps << '\n';
        return false;
    }
    double maximumStateError = 0.0;
    const std::size_t sharedSamples = std::min(cpu.trajectory.size(),
                                               static_cast<std::size_t>(gpuSteps + 1));
    for (std::size_t index = 0; index < sharedSamples; ++index) {
        const std::size_t gpuOffset = index * 4;
        const reference::TrajectoryPoint& cpuPoint = cpu.trajectory[index];
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuPoint.state.radius - static_cast<double>(gpuSamples[gpuOffset])));
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuPoint.state.angle - static_cast<double>(gpuSamples[gpuOffset + 1])));
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuPoint.state.radialVelocity - static_cast<double>(gpuSamples[gpuOffset + 2])));
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuPoint.affineParameter - static_cast<double>(gpuSamples[gpuOffset + 3])));
    }
    const reference::TrajectoryPoint& cpuFinal = cpu.trajectory.back();
    const std::size_t gpuFinalOffset = static_cast<std::size_t>(gpuSteps) * 4;
    maximumStateError = std::max(maximumStateError,
        std::abs(cpuFinal.state.radius - static_cast<double>(gpuSamples[gpuFinalOffset])));
    maximumStateError = std::max(maximumStateError,
        std::abs(cpuFinal.state.angle - static_cast<double>(gpuSamples[gpuFinalOffset + 1])));
    maximumStateError = std::max(maximumStateError,
        std::abs(cpuFinal.state.radialVelocity - static_cast<double>(gpuSamples[gpuFinalOffset + 2])));
    maximumStateError = std::max(maximumStateError,
        std::abs(cpuFinal.affineParameter - static_cast<double>(gpuSamples[gpuFinalOffset + 3])));
    const double eventLocationError = std::abs(cpuFinal.affineParameter - gpuEventAffine);
    std::cout << "gpu_radial_max_state_error=" << maximumStateError
              << " event_location_error=" << eventLocationError
              << " cpu_gpu_steps=" << gpuSteps << '\n';
    return maximumStateError < 2.0e-4 && eventLocationError < 0.01;
}

// Compare a non-radial 3D-oriented trajectory against R0 through its escape event.
bool compareGeneralTrajectory(GLuint program)
{
    const reference::ModelParameters model{1.0, 2.2, 5.2, {0.0, 0.0, 0.0}, {}};
    const reference::InitialConditions initial{10.0, 3.0, -1,
                                                {1.0, 0.0, 0.0},
                                                {0.0, 0.6, 0.8}};
    const reference::IntegrationConfig config{0.01, 60000, 100.0};
    reference::Ray ray{};
    if (!reference::initializeRay(model, initial, ray)) {
        return false;
    }
    const reference::Result cpu = reference::integrate(model, config, ray);
    std::vector<float> gpuSamples;
    float gpuEvent = 0.0f;
    int gpuSteps = 0;
    double gpuEventAffine = 0.0;
    if (!dispatchRay(program, 10.0, 3.0, -1, model.diskNormal, {},
                     gpuSamples, gpuEvent, gpuSteps, gpuEventAffine) ||
        cpu.termination != reference::Termination::Escaped || gpuEvent != 4.0f ||
        std::abs(gpuSteps - cpu.steps) > 1 ||
        std::abs(cpu.ray.affineParameter - gpuEventAffine) > 0.01) {
        std::cerr << "General 3D CPU/GPU trajectory termination mismatch.\n";
        return false;
    }
    double maximumStateError = 0.0;
    double maximumGpuNullResidual = 0.0;
    const std::size_t sharedSamples = std::min(cpu.trajectory.size(),
                                               static_cast<std::size_t>(gpuSteps + 1));
    for (std::size_t index = 0; index < sharedSamples; ++index) {
        const std::size_t gpuOffset = index * 4;
        const reference::State& cpuState = cpu.trajectory[index].state;
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuState.radius - static_cast<double>(gpuSamples[gpuOffset])));
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuState.angle - static_cast<double>(gpuSamples[gpuOffset + 1])));
        maximumStateError = std::max(maximumStateError,
            std::abs(cpuState.radialVelocity - static_cast<double>(gpuSamples[gpuOffset + 2])));
        const reference::State gpuState{static_cast<double>(gpuSamples[gpuOffset]),
                                        static_cast<double>(gpuSamples[gpuOffset + 1]),
                                        static_cast<double>(gpuSamples[gpuOffset + 2])};
        maximumGpuNullResidual = std::max(maximumGpuNullResidual,
            reference::nullConstraintResidual(gpuState, 1.0, 3.0, 1.0));
    }
    std::cout << "gpu_general3d_max_state_error=" << maximumStateError
              << " gpu_max_null_residual=" << maximumGpuNullResidual
              << " cpu_gpu_steps=" << gpuSteps << '\n';
    return maximumStateError < 0.01 && maximumGpuNullResidual < 0.01;
}

// Compare GPU capture and disk-event classes with R0 for off-axis 3D camera directions.
bool compareEventClassifications(GLuint program)
{
    const reference::InitialConditions initial{20.0, 2.5, -1};
    const reference::IntegrationConfig config{0.01, 60000, 100.0};
    const reference::ModelParameters emptyModel{1.0, 2.2, 5.2, {0.0, 1.0, 0.0}, {}};
    reference::Ray ray{};
    if (!reference::initializeRay(emptyModel, initial, ray)) {
        return false;
    }
    const reference::Result cpuCaptured = reference::integrate(emptyModel, config, ray);
    std::vector<float> gpuSamples;
    float gpuEvent = 0.0f;
    int gpuSteps = 0;
    double gpuEventAffine = 0.0;
    if (!dispatchRay(program, 20.0, 2.5, -1, emptyModel.diskNormal, {},
                     gpuSamples, gpuEvent, gpuSteps, gpuEventAffine) ||
        cpuCaptured.termination != reference::Termination::EventHorizon || gpuEvent != 1.0f ||
        std::abs(cpuCaptured.ray.affineParameter - gpuEventAffine) > 0.01) {
        std::cerr << "Off-axis 3D capture classification differs between CPU and GPU.\n";
        return false;
    }
    double maximumEventLocationError =
        std::abs(cpuCaptured.ray.affineParameter - gpuEventAffine);

    const reference::ModelParameters diskModel{1.0, 2.2, 5.2, {1.0, 0.0, 0.0}, {}};
    const reference::InitialConditions diskInitial{10.0, 3.0, -1,
                                                    {1.0, 0.0, 0.0},
                                                    {0.0, 0.6, 0.8}};
    if (!reference::initializeRay(diskModel, diskInitial, ray)) {
        return false;
    }
    const reference::Result cpuDisk = reference::integrate(diskModel, config, ray);
    if (!dispatchRay(program, 10.0, 3.0, -1, diskModel.diskNormal, {},
                     gpuSamples, gpuEvent, gpuSteps, gpuEventAffine) ||
        cpuDisk.termination != reference::Termination::DiskHit || gpuEvent != 2.0f ||
        std::abs(cpuDisk.ray.affineParameter - gpuEventAffine) > 0.01) {
        std::cerr << "Disk event classification differs between CPU and GPU.\n";
        return false;
    }
    maximumEventLocationError = std::max(maximumEventLocationError,
        std::abs(cpuDisk.ray.affineParameter - gpuEventAffine));

    reference::ModelParameters objectModel{1.0, 2.2, 5.2, {0.0, 1.0, 0.0},
                                            {{{5.0, 0.0, 0.0}, 1.0}}};
    const reference::InitialConditions objectInitial{10.0, 0.0, -1};
    if (!reference::initializeRay(objectModel, objectInitial, ray)) {
        return false;
    }
    const reference::Result cpuObject = reference::integrate(objectModel, config, ray);
    if (!dispatchRay(program, 10.0, 0.0, -1, objectModel.diskNormal,
                     objectModel.objects, gpuSamples, gpuEvent, gpuSteps, gpuEventAffine) ||
        cpuObject.termination != reference::Termination::ObjectHit || gpuEvent != 3.0f ||
        std::abs(cpuObject.ray.affineParameter - gpuEventAffine) > 0.01) {
        std::cerr << "Object event classification differs between CPU and GPU.\n";
        return false;
    }
    maximumEventLocationError = std::max(maximumEventLocationError,
        std::abs(cpuObject.ray.affineParameter - gpuEventAffine));

    const reference::ModelParameters escapeModel{1.0, 2.2, 5.2, {0.0, 1.0, 0.0}, {}};
    const reference::InitialConditions outwardInitial{10.0, 0.0, 1};
    if (!reference::initializeRay(escapeModel, outwardInitial, ray)) {
        return false;
    }
    const reference::Result cpuEscape = reference::integrate(escapeModel, config, ray);
    if (!dispatchRay(program, 10.0, 0.0, 1, escapeModel.diskNormal, {},
                     gpuSamples, gpuEvent, gpuSteps, gpuEventAffine) ||
        cpuEscape.termination != reference::Termination::Escaped || gpuEvent != 4.0f ||
        std::abs(cpuEscape.ray.affineParameter - gpuEventAffine) > 0.01) {
        std::cerr << "Escape classification differs between CPU and GPU.\n";
        return false;
    }
    maximumEventLocationError = std::max(maximumEventLocationError,
        std::abs(cpuEscape.ray.affineParameter - gpuEventAffine));
    std::cout << "gpu_event_classes=event_horizon,disk_hit,object_hit,escaped"
              << " max_event_location_error=" << maximumEventLocationError << '\n';
    return maximumEventLocationError < 0.01;
}

// Verify that a deliberately one-step GPU variant reports the maximum-step state distinctly.
bool compareMaximumStepTermination()
{
    std::string shaderSource;
    if (!loadDiagnosticShader("shaders/b1/geodesic.comp", shaderSource, 1)) {
        return false;
    }
    const GLuint program = compileComputeProgram(shaderSource);
    if (program == 0) {
        return false;
    }
    const reference::ModelParameters model{1.0, 2.2, 5.2, {0.0, 1.0, 0.0}, {}};
    const reference::InitialConditions initial{20.0, 5.0, -1};
    reference::Ray ray{};
    if (!reference::initializeRay(model, initial, ray)) {
        glDeleteProgram(program);
        return false;
    }
    const reference::Result cpu = reference::integrate(model, {0.01, 1, 100.0}, ray);
    std::vector<float> samples;
    float eventKind = -1.0f;
    int steps = 0;
    double eventAffine = 0.0;
    const bool dispatched = dispatchRay(program, 20.0, 5.0, -1, model.diskNormal, {},
                                        samples, eventKind, steps, eventAffine, 1);
    glDeleteProgram(program);
    if (!dispatched || cpu.termination != reference::Termination::MaximumSteps ||
        eventKind != 0.0f || steps != 1 || std::abs(eventAffine - 0.01) > 1.0e-6) {
        std::cerr << "Maximum-step termination did not match the R0 status.\n";
        return false;
    }
    std::cout << "gpu_maximum_step_status=maximum_steps steps=1\n";
    return true;
}

// Confirm a singular camera input produces the shader's visible invalid-state color, not escape.
bool detectInvalidGpuState(GLuint program)
{
    std::vector<float> samples;
    float eventKind = 0.0f;
    int steps = 0;
    double eventAffine = 0.0;
    unsigned char pixel[4] = {};
    const bool dispatched = dispatchRay(program, 0.0, 0.0, -1, {0.0, 1.0, 0.0}, {},
                                        samples, eventKind, steps, eventAffine,
                                        shaderMaximumSteps, pixel);
    if (!dispatched || pixel[0] != 255 || pixel[1] != 0 || pixel[2] != 255 || pixel[3] != 255) {
        std::cerr << "Invalid GPU ray was not rendered with the invalid-state marker.\n";
        return false;
    }
    std::cout << "gpu_invalid_state=magenta_marker\n";
    return true;
}

} // namespace

// Create a hidden OpenGL 4.3 context and compare the production B1 shader against R0.
int main()
{
    if (!glfwInit()) {
        std::cerr << "GLFW initialization failed.\n";
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(1, 1, "B1 GPU validation", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "OpenGL 4.3 validation context creation failed.\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "GLEW initialization failed.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    std::string shaderSource;
    if (!loadDiagnosticShader("shaders/b1/geodesic.comp", shaderSource)) {
        std::cerr << "Could not read shaders/b1/geodesic.comp.\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    const GLuint program = compileComputeProgram(shaderSource);
    if (program == 0) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    CameraBlock camera{};
    DiskBlock disk{};
    ObjectsBlock objects{};
    cameraBuffer = createUniformBuffer(1, sizeof(camera), &camera);
    diskBuffer = createUniformBuffer(2, sizeof(disk), &disk);
    objectBuffer = createUniformBuffer(3, sizeof(objects), &objects);

    const bool trajectoryMatches = compareRadialTrajectory(program);
    const bool generalTrajectoryMatches = compareGeneralTrajectory(program);
    const bool eventsMatch = compareEventClassifications(program);
    const bool maximumStepsMatch = compareMaximumStepTermination();
    const bool invalidStateMarked = detectInvalidGpuState(program);
    glDeleteProgram(program);
    glfwDestroyWindow(window);
    glfwTerminate();
    if (!trajectoryMatches || !generalTrajectoryMatches || !eventsMatch ||
        !maximumStepsMatch || !invalidStateMarked) {
        return 1;
    }
    std::cout << "PASS: B1 GPU radial trajectory and event classifications agree with R0.\n";
    return 0;
}
