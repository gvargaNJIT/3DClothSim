# Steps

1) turn venv on
2) mkdir build
3) cd build
4) conan install .. -g CMakeToolchain -g CMakeDeps -of build --build=missing
5) cmake .. -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
6) cmake --build . (For tests:  cmake --build . --target tests)
7) ./bin/cloth_simulation (For tests:  ./tests)

This is a first pass run, and I am not using not realistic gravity just to use the mouse constraint.