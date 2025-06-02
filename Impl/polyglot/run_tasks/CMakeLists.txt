cmake_minimum_required(VERSION 3.10)

set(CMAKE_C_COMPILER gcc-10)
set(CMAKE_CXX_COMPILER g++-10)
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

project(m2bench)

include_directories("src/include")

# Find BLAS
find_package(BLAS REQUIRED)
if(!BLAS_FOUND)
    message(FATAL_ERROR "OpenBLAS not found.")
    return()
endif()

# spdlog
find_package(spdlog CONFIG REQUIRED)

include_directories(${BLAS_INCLUDE_DIRS})

include(FetchContent)

# VelocyDuck
include_directories("thirdparty/velocypack/include")
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
    src/Polyglot/healthcare_func.cpp
    src/Polyglot/healthcare_task.cpp
    src/Polyglot/disaster_func.cpp
    src/Polyglot/disaster_task.cpp
    src/Polyglot/general_func.cpp

    src/Connection/Connection.cpp
    src/json_udf.cpp
)


target_link_libraries(m2bench PUBLIC 
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/velocypack/build/libvelocypack.a"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/libspatialindex/build/src/libspatialindex.so"
    duckdb 
    "$ENV{PREVISION_PATH}/build/libprevision.so"
    spdlog::spdlog
    ${BLAS_LIBRARIES}
    m rt pthread)



