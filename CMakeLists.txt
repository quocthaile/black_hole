cmake_minimum_required(VERSION 3.21)
project(BlackHoleSim)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GLEW REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

function(add_black_hole_executable target source)
    add_executable(${target} ${source})
    target_link_libraries(${target} PRIVATE glfw GLEW::GLEW glm::glm OpenGL::GL)
    # Preserve Unicode identifiers in the original sources on Windows.
    if(MSVC)
        target_compile_options(${target} PRIVATE /utf-8)
    endif()
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${target}"
        VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:${target}>"
    )
endfunction()

function(add_black_hole_shaders target directory)
    set(shaders
        "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/geodesic.comp"
        "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/grid.vert"
        "${CMAKE_CURRENT_SOURCE_DIR}/${directory}/grid.frag"
    )
    set(runtime_dir "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/${target}")
    # Also stage on shader-only rebuilds (POST_BUILD alone requires a relink).
    # Use the explicit directory here to avoid a dependency cycle on the target.
    add_custom_target(${target}_shaders
        COMMAND ${CMAKE_COMMAND} -E make_directory "${runtime_dir}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${shaders} "${runtime_dir}"
        DEPENDS ${shaders}
        VERBATIM
    )
    add_dependencies(${target} ${target}_shaders)
    add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${shaders} "$<TARGET_FILE_DIR:${target}>"
        VERBATIM
    )
endfunction()

add_black_hole_executable(BlackHole3D src/black_hole.cpp)
add_black_hole_executable(BlackHole2D src/2D_lensing.cpp)
add_black_hole_executable(BlackHole3D_baseline src/baseline/black_hole.cpp)
add_black_hole_executable(BlackHole2D_baseline src/baseline/2D_lensing.cpp)

add_black_hole_shaders(BlackHole3D shaders)
add_black_hole_shaders(BlackHole3D_baseline src/baseline)
