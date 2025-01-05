# Conan example

Using `build` folder

## Debug build
```
conan install .. --build=missing -pr:b=default -s build_type=Debug
cmake .. --preset conan-debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cd Debug
make
```
* Use `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to export list of compile commands
* For cmake<3.23 :
```
cd Debug
cmake ../.. -DCMAKE_TOOLCHAIN_FILE=generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Debug
```

## Release build
```
conan install .. --build=missing -pr:b=default -s build_type=Release
cmake .. --preset conan-release
cd Release
make
```

## References
* [conan installation](https://docs.conan.io/2/installation.html)
* [conan: Building for multiple configurations](https://docs.conan.io/2/tutorial/consuming_packages/different_configurations.html)
* [conan grpc](https://conan.io/center/recipes/grpc)
* [conan protobuf](https://conan.io/center/recipes/protobuf)