cmake_minimum_required(VERSION 3.21)

project(prometheus_sample)

find_package(Boost REQUIRED)
find_package(prometheus-cpp REQUIRED)

add_executable(
    ${PROJECT_NAME}
    main.cpp
)

set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_STANDARD_REQUIRED YES
        CXX_EXTENSIONS NO
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
        prometheus-cpp::prometheus-cpp
        boost::boost
)
