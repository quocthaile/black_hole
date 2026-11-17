#include "geodesic_reference.h"

#include <cstdlib>
#include <iostream>
#include <string>

namespace {

// Parse one finite command-line number or preserve the supplied default.
double readNumber(int& index, int argc, char** argv, double fallback)
{
    if (index + 1 >= argc) {
        return fallback;
    }
    ++index;
    return std::strtod(argv[index], nullptr);
}

// Parse one integer command-line number or preserve the supplied default.
int readInteger(int& index, int argc, char** argv, int fallback)
{
    if (index + 1 >= argc) {
        return fallback;
    }
    ++index;
    return std::atoi(argv[index]);
}

// Print the supported options and default camera-ray configuration.
void printUsage(const char* executable)
{
    std::cout << "Usage: " << executable
              << " [--csv path] [--metadata path] [--rs value] [--camera-radius value]"
                 " [--impact value] [--radial-sign -1|1] [--step value]"
                 " [--max-steps count] [--escape-radius value]\n"
              << "All lengths are in the common model coordinate unit; E is normalized to 1.\n";
}

} // namespace

// Run the standalone R0 solver and export its sampled trajectory and configuration.
int main(int argc, char** argv)
{
    std::string csvPath = "trajectory.csv";
    std::string metadataPath = "trajectory.json";
    reference::ModelParameters model{1.0, 2.2, 5.2, {0.0, 1.0, 0.0}, {}};
    reference::InitialConditions initial{10.0, 3.0, -1};
    reference::IntegrationConfig config{0.01, 100000, 100.0};

    for (int index = 1; index < argc; ++index) {
        const std::string option = argv[index];
        if (option == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (option == "--csv" && index + 1 < argc) {
            csvPath = argv[++index];
        } else if (option == "--metadata" && index + 1 < argc) {
            metadataPath = argv[++index];
        } else if (option == "--rs") {
            model.schwarzschildRadius = readNumber(index, argc, argv, model.schwarzschildRadius);
        } else if (option == "--camera-radius") {
            initial.cameraRadius = readNumber(index, argc, argv, initial.cameraRadius);
        } else if (option == "--impact") {
            initial.impactParameter = readNumber(index, argc, argv, initial.impactParameter);
        } else if (option == "--radial-sign") {
            initial.radialSign = readInteger(index, argc, argv, initial.radialSign);
        } else if (option == "--step") {
            config.stepSize = readNumber(index, argc, argv, config.stepSize);
        } else if (option == "--max-steps") {
            config.maximumSteps = readInteger(index, argc, argv, config.maximumSteps);
        } else if (option == "--escape-radius") {
            config.escapeRadius = readNumber(index, argc, argv, config.escapeRadius);
        } else {
            std::cerr << "Unknown or incomplete option: " << option << '\n';
            printUsage(argv[0]);
            return 2;
        }
    }

    reference::Ray ray{};
    if (!reference::initializeRay(model, initial, ray)) {
        std::cerr << "Invalid ray: camera radius, impact parameter, or radial sign is unsupported.\n";
        return 2;
    }
    const reference::Result result = reference::integrate(model, config, ray);
    if (!reference::writeTrajectoryCsv(csvPath, result) ||
        !reference::writeMetadataJson(metadataPath, model, initial, config, result)) {
        std::cerr << "Failed to write trajectory or metadata output.\n";
        return 1;
    }
    std::cout << "termination=" << reference::terminationName(result.termination)
              << " steps=" << result.steps
              << " samples=" << result.trajectory.size()
              << " csv=" << csvPath
              << " metadata=" << metadataPath << '\n';
    return result.termination == reference::Termination::InvalidState ? 1 : 0;
}
