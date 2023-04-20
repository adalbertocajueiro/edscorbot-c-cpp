cmake_minimum_required(VERSION 3.10)
project(simulated_edscorbot_runtime)


## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++17)

add_subdirectory("src/mqtt_server")
#set(nlohmann_json_DIR "/home/enrique/Trabajo/Py-EDScorbotTool/c/src/json/cmake")
#find_package(nlohmann_json 3.10.5 REQUIRED)
## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages

SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fPIC --static  -DEDS_VERBOSE")
SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} --static -DEDS_VERBOSE")

#cmake_path(SET project_base_dir ..)
#message("Directorios: ${CMAKE_BINARY_DIR}\t${CMAKE_SOURCE_DIR}\t${CMAKE_CURRENT_BINARY_DIR}\t${CMAKE_CURRENT_SOURCE_DIR}")
#message("DIRECTORIO BASE PROYECTO: ${project_base_dir}")
# Custom CMake base module

#add_executable(read_joints src/EDScorbot/EDScorbot.cpp src/scripts/read_joints.cpp src/EDScorbot/devmem.cpp ${HEADER_FILES})
#add_executable(test_arrays src/test_scripts/test_arrays.cpp)
#add_executable(test_argparse src/test_scripts/test_argparse.cpp)

#add_executable(test_json src/test_scripts/test_json.cpp)
#add_executable(home src/scripts/home.cpp src/EDScorbot/EDScorbot.cpp src/EDScorbot/devmem.cpp ${HEADER_FILES})
#add_executable(reset src/scripts/reset_all.cpp src/EDScorbot/EDScorbot.cpp src/EDScorbot/devmem.cpp ${HEADER_FILES})
#add_executable(send_home src/scripts/send_0.cpp src/EDScorbot/EDScorbot.cpp src/EDScorbot/devmem.cpp ${HEADER_FILES})
#add_executable(sendRef src/scripts/sendRefJx.cpp src/EDScorbot/EDScorbot.cpp src/EDScorbot/devmem.cpp ${HEADER_FILES})

add_executable(simulated_server src/mqtt_server/simulated_server.cpp)

#add_executable(tests_json_objects src/test_scripts/test_json_objects.cpp)

#add_executable(test_pthread src/test_scripts/test_pthread.c)

#add_library(image_generator SHARED src/Image_generator.cpp)

#arget_include_directories(read_joints PUBLIC "src")

#target_include_directories(test_json PUBLIC "src")

#target_include_directories(home PUBLIC "src" "./" "${CMAKE_CURRENT_SOURCE_DIR}/json/single_include/")


#target_include_directories(reset PUBLIC "src" "./")
#target_include_directories(send_home PUBLIC "src" "./")

#target_include_directories(test_argparse PUBLIC "src" "./")

#target_include_directories(sendRef PUBLIC "src" "./")

target_include_directories(simulated_server PUBLIC "src" "./")
#target_include_directories(tests_json_objects PUBLIC "src" "${CMAKE_CURRENT_SOURCE_DIR}/json/single_include/")

#target_include_directories(test_pthread PUBLIC "src" "${CMAKE_CURRENT_SOURCE_DIR}/json/single_include/")

#TARGET_LINK_LIBRARIES(mocked_server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compiled_libs_x86/libmosquitto_static.a" -lpthread )
#TARGET_LINK_LIBRARIES(mocked_server PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/compiled_libs_armhf/libmosquitto.a" -lpthread )
#TARGET_LINK_LIBRARIES(mocked_server PRIVATE /home/adalberto/Downloads/tmp/mosquitto/lib/libmosquitto.a -lpthread )

#TARGET_LINK_LIBRARIES(test_pthread PRIVATE -lpthread )
TARGET_LINK_LIBRARIES(simulated_server PRIVATE  /home/adalberto/tmp/mosquitto/build/lib/libmosquitto_static.a -lpthread)


SET(CMAKE_BUILD_TYPE "Debug")
