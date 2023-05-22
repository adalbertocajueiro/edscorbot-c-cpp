cmake_minimum_required(VERSION 3.10)
project(simulated_edscorbot_runtime)


## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++17)

execute_process(
    COMMAND bash "-c" "'${CMAKE_CURRENT_SOURCE_DIR}/mosquitto_build.sh'" 
    OUTPUT_VARIABLE FOO
)

add_subdirectory("src/mqtt_server")
add_subdirectory("json")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC --static  -DEDS_VERBOSE")
SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} --static -DEDS_VERBOSE")
set(LANGUAGE C_STANDARD)

add_executable(simulated_server src/mqtt_server/simulated_server.cpp)

target_include_directories(simulated_server PUBLIC "src" "./" "json/single_include/")

#TODO
# Replace the static lib full path with the full path of your machine
# when you compiled mosquitto
TARGET_LINK_LIBRARIES(simulated_server PRIVATE  "${CMAKE_CURRENT_SOURCE_DIR}/lib/libmosquitto_static.a" -lpthread)


SET(CMAKE_BUILD_TYPE "Debug")