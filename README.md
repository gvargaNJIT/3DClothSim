# Steps

1) turn venv on
2) mkdir build
3) cd build
4) conan install .. -g CMakeToolchain -g CMakeDeps -of build --build=missing
5) cmake .. -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
6) cmake --build .
7) ./bin/cloth_simulation