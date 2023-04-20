cmake_minimum_required(VERSION 3.10)
project(simulated_edscorbot_runtime)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++17)

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS_INIT}")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC -DEDS_VERBOSE")
set(LANGUAGE C_STANDARD)

#add_executable(simulated_server simulated_server.cpp)

#add_library(image_generator SHARED src/Image_generator.cpp)

#target_include_directories(mqtt_server PUBLIC "../../json/single_include/")
#target_include_directories(simulated_server PUBLIC "../")
#target_include_directories(mqtt_server PUBLIC "/home/enrique/mosquitto-2.0.14/include")

# commented because i have compiled mosquitto and included the header files in the project
#target_include_directories(mqtt_server PUBLIC "../../mosquitto/include")

#TARGET_LINK_LIBRARIES(simulated_server PRIVATE  /home/adalberto/mosquitto/build/lib/lismosquitto_static.a -lpthread)

SET(CMAKE_BUILD_TYPE "Debug")
