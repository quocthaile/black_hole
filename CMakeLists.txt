
cmake_minimum_required(VERSION 3.21)

project(BlackHoleSim LANGUAGES CXX)

# ============================================================
# C++ Standard
# ============================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# Dependencies (vcpkg)
# ============================================================

find_package(GLEW CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

# ============================================================
# Executable Configuration
# ============================================================

function(add_black_hole_executable target source)

    add_executable(${target} ${source})

    target_link_libraries(${target}
        PRIVATE
            glfw
            GLEW::GLEW
            glm::glm
            OpenGL::GL
    )

    # Preserve Unicode identifiers in original sources on Windows.
    if(MSVC)
        target_compile_options(${target}
            PRIVATE
                /utf-8
        )
    endif()

    # Output:
    # build/msvc-vs2026/bin/Debug/<target>/<target>.exe
    # build/msvc-vs2026/bin/Release/<target>/<target>.exe

    set_target_properties(${target} PROPERTIES

        RUNTIME_OUTPUT_DIRECTORY
            "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${target}"

        VS_DEBUGGER_WORKING_DIRECTORY
            "$<TARGET_FILE_DIR:${target}>"

    )

endfunction()

# ============================================================
# Shader Configuration
# ============================================================

function(add_black_hole_shaders target directory)

    set(shaders
        "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/geodesic.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/grid.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/grid.frag"
    )

    set(runtime_dir
        "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${target}"
    )

    # Stage shaders even when only shader files change.
    add_custom_target(${target}_shaders

        COMMAND ${CMAKE_COMMAND} -E make_directory
            "${runtime_dir}"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${shaders}
            "${runtime_dir}"

        DEPENDS
            ${shaders}

        VERBATIM
    )

    # Ensure shaders are staged before executable build.
    add_dependencies(${target} ${target}_shaders)

    # Also stage shaders after executable relinking.
    add_custom_command(TARGET ${target} POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${shaders}
            "$<TARGET_FILE_DIR:${target}>"

        VERBATIM
    )

endfunction()

# ============================================================
# 3D / 2D Executables
# ============================================================

# Optimized versions

add_black_hole_executable(
    BlackHole3D
    src/black_hole.cpp
)

add_black_hole_executable(
    BlackHole3D_B1
    src/b1/black_hole.cpp
)

# Baseline versions

add_black_hole_executable(
    BlackHole3D_baseline
    src/baseline/black_hole.cpp
)

add_black_hole_executable(
    BlackHole2D_baseline
    src/baseline/2D_lensing.cpp
)

# Independent CPU double-precision solver and its analytical/convergence checks.
add_executable(
    BlackHoleGeodesicReference
    src/reference/reference_main.cpp
    src/reference/geodesic_reference.cpp
)

add_executable(
    BlackHoleGeodesicReferenceTests
    src/reference/reference_tests.cpp
    src/reference/geodesic_reference.cpp
)

add_executable(
    BlackHoleB1GpuValidation
    src/reference/gpu_validation.cpp
    src/reference/geodesic_reference.cpp
)
target_link_libraries(BlackHoleB1GpuValidation
    PRIVATE glfw GLEW::GLEW glm::glm OpenGL::GL
)

enable_testing()
add_test(NAME BlackHoleGeodesicReferenceTests COMMAND BlackHoleGeodesicReferenceTests)
add_test(NAME BlackHoleB1GpuValidation COMMAND BlackHoleB1GpuValidation)
set_tests_properties(BlackHoleB1GpuValidation PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# ============================================================
# Shader Staging
# ============================================================

# Optimized 3D

add_black_hole_shaders(
    BlackHole3D
    shaders/b1
)

add_black_hole_shaders(
    BlackHole3D_B1
    shaders/b1
)

# Baseline 3D

add_black_hole_shaders(
    BlackHole3D_baseline
    src/baseline
)
