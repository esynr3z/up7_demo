RD /s /Q build
mkdir build
cd build
cmake .. -G "Visual Studio 16 2019"
cmake --build .
cd ..
