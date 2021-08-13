RD /s /Q build
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. -G "MinGW Makefiles"
cmake --build .
cd ..
