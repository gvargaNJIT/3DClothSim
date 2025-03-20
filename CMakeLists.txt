cmake_minimum_required(VERSION 3.23)
project(ClothSimulation VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use Conan 2.x toolchain if available (should be called automatically by Conan)
include(conan_toolchain.cmake)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)

# Find libraries
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(glad REQUIRED)
set(GLFW_LIBRARY /usr/lib/x86_64-linux-gnu/libglfw.so.3)
set(GLFW_INCLUDE_DIR /usr/include) 

# Add main executable
add_executable(cloth_simulation src/main.cpp)

# Link libraries
target_link_libraries(cloth_simulation PRIVATE
    ${GLFW_LIBRARY}
    ${OPENGL_LIBRARIES}
    glad::glad
    glm::glm
)

# Set include directories
target_include_directories(cloth_simulation PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${CONAN_INCLUDE_DIRS_GLFW}
    ${GLFW_INCLUDE_DIR}
)

# Set output directory
set_target_properties(cloth_simulation
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Enable warnings
if(MSVC)
    target_compile_options(cloth_simulation PRIVATE /W4)
else()
    target_compile_options(cloth_simulation PRIVATE -Wall -Wextra)
endif()