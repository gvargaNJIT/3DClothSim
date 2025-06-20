cmake_minimum_required(VERSION 3.23)
project(ClothSimulation VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

# Use Conan 2.x toolchain if available (should be called automatically by Conan)
include(./build/build/conan_toolchain.cmake)
include(${CMAKE_BINARY_DIR}/conan_paths.cmake OPTIONAL)

# Define source files
set(SOURCES
    ./src/main.cpp
    ./src/clothgrid.cpp
    ./src/clothsim.cpp
    ./src/openGL.cpp
    ./src/clothwidget.cpp
    ./include/clothwidget.h
)

# Find libraries
find_package(OpenGL REQUIRED)
find_package(glad REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS OpenGLWidgets)


# Add main executable
qt_add_executable(cloth_simulation ${SOURCES})

# Link libraries
target_link_libraries(cloth_simulation PRIVATE
    ${OPENGL_LIBRARIES}
    glad::glad
    glm::glm
    OpenGL::GL
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGL
    Qt6::OpenGLWidgets
)

# Set include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
target_include_directories(cloth_simulation PRIVATE
    ${OPENGL_INCLUDE_DIR}
)

# Define header files
set(HEADERS
    ./include/clothgrid.h
    ./include/clothsim.h
    ./include/openGL.h
    ./include/clothwidget.h
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
qt_add_executable(tests test/test_cloth.cpp src/clothsim.cpp src/clothgrid.cpp)
target_include_directories(tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link Google Test
find_package(GTest REQUIRED)
target_link_libraries(tests PRIVATE GTest::gtest_main)

# Register the test suite
add_test(NAME ClothSimulationTests COMMAND tests)

# Set Qt platform plugin path at runtime
set(QT_PLUGIN_PATH "$ENV{HOME}/.conan2/p/b/qt5a8f643d4b9a4/p/plugins")

set_target_properties(cloth_simulation PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

set_target_properties(cloth_simulation PROPERTIES
    VS_DEBUGGER_ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLUGIN_PATH}"
)

if (UNIX)
    set_target_properties(cloth_simulation PROPERTIES
        BUILD_RPATH "${QT_PLUGIN_PATH}"
    )

    set_tests_properties(ClothSimulationTests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM_PLUGIN_PATH=${QT_PLUGIN_PATH}"
    )
endif()

# Set QT_QPA_PLATFORM_PLUGIN_PATH as a runtime environment variable
add_custom_command(TARGET cloth_simulation POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${QT_PLUGIN_PATH}/platforms"
    "$<TARGET_FILE_DIR:cloth_simulation>/platforms"
)
