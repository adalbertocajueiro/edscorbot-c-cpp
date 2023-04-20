# Getting Started
This repository contains the simulated server, an application that simulates the robotic arm.

The main use of this project is to provide a first version of the real server to be embedded into the arm. 

### Tools Information
This project has been developed using the following tools:
* CMake 3.26.3
* GNU Make 4.3
* GCC 11.3.0 (Ubuntu 22.04) 
* Visual Studio Code version 1.77.1 with extensions: C/C++ Extension Pack.  

### Project structure
* After downloading and extracting the project, the folder `edscorbot-c-cpp` is the root folder. It has a specific folder/files structure
* File `include/server-defs.hpp` contains the objects implementing the schemas defined in the Async API specification. Have a look at them to undestand their features. 

### Installing
* You can try to use your own versions of the tools. If it does not work we advice to install the above versions
* Download de project and unzip it
* Open the project in Visual Studio Code (vscode). It might be possible vscode offers other extensions to be installed. Just accept it.
* File CMakeLists.txt contains the configuration information to compile the project.

### Compiling
The project depends on other specific projects that must be available or previously installed:

#### Compiling Mosquitto
As the server connects with Mosquitto broker, its compilation requires the Mosquitto libraries are previously installed. You can download the [Mosquitto sources](https://github.com/eclipse/mosquitto) and compile it to any target platform. We have compiled it to the local platform considering static libraries. You can customize the compilation options and install/compile (suggested) the dependencies. In our example, we have installed:
* `sudo apt-get update` to update the artifact list  
* `sudo apt-get install openssl libssl-dev` to install openssl and openssl development files
* `sudo apt-get install libcjson-dev` to install libcjson. this lib is required to compile `mosquitto_pub` and `mosquitto_sub` tools 
* `sudo apt-get install xsltproc` to install the support for generating documentation during the compilation
* As we have compiled the static libraries, we had to set this option in file `mosquitto/CMakeLists.txt`: `option(WITH_STATIC_LIBRARIES "Build static versions of the libmosquitto/pp libraries?" ON)`
* `cd mosqquitto` to enter the mosquitto main folder
* `mkdir build` to create a specific output folder
* `cd build` to enter the output folder
* `cmake ..` to generate build files from the parent folder into current folder
* `make install` to compile and install the library

If everything goes well you should see the generated static lib file is `build/lib/libmosquitto_static.a`. Keep its full path to be used in server compilation.

For more information about compiling Mosquitto, please refer to its official site.

#### Compiling Niels Lohmann JSON library
Although Mosquitto uses cJSON as library for reading/writing JSON, the simulated server uses a more sophisticated library: [Niels Lohmann json lib](https://github.com/nlohmann/json.git). We have cloned the repository and compiled it:

* `cd json` to enter the project folder
* `mkdir build` to create a specific output folder
* `cd build` to enter the output folder
* `cmake ..` to generate build files from the parent folder into current folder
* `make install` to compile and install the library

If everything goes well you will have wverything installed in your machine to compile and run the server.

#### Compiling the simulated server
After cloning the project, follow these steps to compile the project:
* `cd edscorbot-c-cpp` to enter the project folder
* `mkdir build` to create a specific output folder
* `cd build` to enter the output folder
* `cmake ..` to generate build files from the parent folder into current folder
* `make install` to compile and install the library 

### Running 
After compiling, identify the instance of Mosquitto you want to connect and adjust the value of the variable `#define mqtt_host "localhost"` in `simulated_server.cpp`.Visual studio code offers a button to start the server (Debug or Run). Run it. If everything goes well you should se something as bellow:


```
Subscribing on topic metainfo
Subscribing on topic EDScorbot/commands
Metainfo published {
    "joints":[{"maximum":-166.66666666666666,"minimum":150.0},
              {"maximum":-85.1063829787234,"minimum":101.06382978723404},
              {"maximum":-112.90322580645162,"minimum":112.90322580645162},
              {"maximum":-90.849272,"minimum":85.1711925},
              {"maximum":360.0,"minimum":-360.0},
              {"maximum":100.0,"minimum":0.0}],
    "name":"EDScorbot",
    "signal":2}
```    

### Migrating to the real controller
A good idea is to user the simulated server file to fit your robot characteristics. After that, inject the code to control your robotic arm. In the function `message_callback` you will see a specific comment where you must put your implementation to do everything as before.

### Reference Documentation
For further reference, please consider the following items:
* [EDScorbot Github Project](https://github.com/RTC-research-group/Py-EDScorbotTool) - the main Github project with details about the entire project and the low level code to control the robotic arm
* [EDScorbot Documentation](https://py-edscorbottool.readthedocs.io/en/latest/) - documentation about the entire project