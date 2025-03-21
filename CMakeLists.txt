cmake_minimum_required(VERSION 3.23)
project(ClothSimulation VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Use Conan 2.x toolchain if available (should be called automatically by Conan)
include(./build/build/conan_toolchain.cmake)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)

# Define source files
set(SOURCES
    ./src/main.cpp
    ./src/clothgrid.cpp
    ./src/clothsim.cpp
    ./src/openGL.cpp
)

# Find libraries
find_package(OpenGL REQUIRED)
find_package(glm REQUIRED)
find_package(glad REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)
set(GLFW_LIBRARY /usr/lib/x86_64-linux-gnu/libglfw.so.3)
set(GLFW_INCLUDE_DIR /usr/include) 

# Add main executable
add_executable(cloth_simulation ${SOURCES})

# Link libraries
target_link_libraries(cloth_simulation PRIVATE
    ${GLFW_LIBRARY}
    ${OPENGL_LIBRARIES}
    glad::glad
    glm::glm
    ${GLEW_LIBRARIES}
    GLEW::GLEW
    OpenGL::GL
)

# Set include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
target_include_directories(cloth_simulation PRIVATE
    ${OPENGL_INCLUDE_DIR}
    ${CONAN_INCLUDE_DIRS_GLFW}
    ${GLFW_INCLUDE_DIR}
)

# Define header files
set(HEADERS
    ./include/clothgrid.h
    ./include/clothsim.h
    ./include/openGL.h
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

# Enable testing framework
enable_testing()

# Add Google Test executable
add_executable(tests test/test_cloth.cpp src/clothsim.cpp src/clothgrid.cpp)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link Google Test
find_package(GTest REQUIRED)
target_link_libraries(tests PRIVATE GTest::gtest_main)

# Register the test suite
add_test(NAME ClothSimulationTests COMMAND tests)