cmake -S . -B build-release-gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Release -DENABLE_STATIC_CHECKS=ON
cmake --build build-release-gcc -j
cmake --install build-release-gcc --prefix .

#cmake -S . -B build-debug-gcc -DCMAKE_CXX_COMPILER=g++ -DCMAKE_BUILD_TYPE=Debug -DENABLE_STATIC_CHECKS=ON
#cmake --build build-debug-gcc -j
#cmake --install build-debug-gcc --prefix .
#
#cmake -S . -B build-release-clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DENABLE_STATIC_CHECKS=ON
#cmake --build build-release-clang -j
#
#cmake -S . -B build-debug-clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug -DENABLE_STATIC_CHECKS=ON
#cmake --build build-debug-clang -j

