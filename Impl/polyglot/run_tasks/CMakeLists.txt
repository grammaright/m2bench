cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER gcc-10)
set(CMAKE_CXX_COMPILER g++-10)
set(CMAKE_CXX_STANDARD 20)

project(m2bench)

set(CMAKE_BUILD_TYPE Debug)

include_directories("src/include")

# Boost
find_package (Boost 1.70.0 REQUIRED COMPONENTS unit_test_framework)
include_directories(${Boost_INCLUDE_DIR})
find_library(Boost_LIBRARIES boost ${Boost_LIBRARIES_DIR})


# Find BLAS
find_package(BLAS REQUIRED)
if(!BLAS_FOUND)
    message(FATAL_ERROR "OpenBLAS not found.")
    return()
endif()

include_directories(${BLAS_INCLUDE_DIRS})

include(FetchContent)

# m2bench
include_directories(include)
include_directories(src)

# VelocyDuck
include_directories("thirdparty/velocypack/include")
include_directories("thirdparty/RTree")
include_directories("thirdparty/libspatialindex/include")

include_directories("$ENV{PREVISION_PATH}/src/include")
include_directories("$ENV{DUCKDB_PATH}/src/include")

# DuckDB
link_directories("$ENV{DUCKDB_PATH}/build/release/src")
# link_directories("$ENV{DUCKDB_PATH}/build/debug/src")


add_executable(m2bench 
    main.cpp

    src/Polyglot/ecommerce_func.cpp
    src/Polyglot/ecommerce_task.cpp
    src/Polyglot/healthcare_task.cpp
    src/Polyglot/disaster_task.cpp

    src/Connection/Connection.cpp
    src/json_udf.cpp
)


target_link_libraries(m2bench PUBLIC ${Boost_LIBRARIES} 
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/velocypack/build/libvelocypack.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libspatialindex/build/src/libspatialindex.so"
    duckdb 
    "$ENV{PREVISION_PATH}/build/libprevision.so"
    ${BLAS_LIBRARIES}
    m rt pthread)



