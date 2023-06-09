# Getting Started
This repository contains the simulated server, an application that simulates the robotic arm.

The main use of this project is to provide a first version of the real server to be embedded into the arm. 

The implementation follows the [Async API specification](https://app.swaggerhub.com/apis-docs/ADALBERTOCAJUEIRO_1/ed-scorbot_async/1.0.0), establishing all channels/topics and the data to be exchanged between applications.

### Tools Information
This project has been developed using the following tools:
* CMake 3.26.3
* GNU Make 4.3
* GCC 11.3.0 (Ubuntu 22.04) 
* Visual Studio Code version 1.77.1 with C/C++ Extension Pack.  

### Project structure
* After downloading and extracting the project, open the folder `edscorbot-c-cpp`. It has a specific folder/files structure.
* File `edscorbot-c-cpp/CMakeLists.txt` contains the configuration and goals for this project.
* File `include/server-defs.hpp` contains the objects implementing the schemas defined in the Async API specification. Have a look at them to undestand their features. 
* File `impl/server-impls.cpp` contains some basic implementations.

It is important to have a look at these files jointly with the API specification to understand the adherence between them.

### Installing and Compiling
* You can try to use your own versions of the tools. If it does not work we suggest you to install the above versions
* Download de project and unzip it
* Open the project in Visual Studio Code (vscode). It might be possible vscode offers other extensions to be installed. Just accept it.
* The project depends on other specific projects that must be available or previously compiled or installed:

#### Compiling Mosquitto
As the server connects with Mosquitto broker, its compilation requires the Mosquitto libraries. You can download the [Mosquitto sources](https://github.com/eclipse/mosquitto) and compile it to any target platform. We have compiled it to the local platform considering static libraries. You can customize the compilation options and install/compile the dependencies (this is the recommended approach). In our example, we had run:
* `sudo apt-get update` to update the artifact list  
* `sudo apt-get install openssl libssl-dev` to install openssl and openssl development files
* `sudo apt-get install libcjson-dev` to install libcjson. This lib is required to compile `mosquitto_pub` and `mosquitto_sub` tools 
* `sudo apt-get install xsltproc` to install the support for generating documentation during the compilation
* In file`mosquitto/CMakeLists.txt` lookup the line to set the generation of static libraries: 
    
    `option(WITH_STATIC_LIBRARIES "Build static versions of the libmosquitto/pp libraries?" ON)`

* `cd mosquitto` to enter the mosquitto main folder
* `mkdir build` to create a specific output folder
* `cd build` to enter the output folder
* `cmake ..` to generate build files from the parent folder into current folder
* `make install` to compile and install the library

If everything goes well you should see the generated static lib file is `mosquitto/build/lib/libmosquitto_static.a`. Keep its full path to be used in server compilation.

For more information about compiling Mosquitto, please refer to its official site.

#### Compiling Niels Lohmann JSON library
Although Mosquitto uses cJSON as library for reading/writing JSON, the simulated server uses a more sophisticated library: [Niels Lohmann json lib](https://github.com/nlohmann/json.git). We have cloned the repository and performed the following commands:

* `cd json` to enter the project folder
* `mkdir build` to create a specific output folder
* `cd build` to enter the output folder
* `cmake ..` to generate build files from the parent folder into current folder
* `make install` to compile and install the library

If everything goes well you will have all dependencies ready to compile and run the server.

#### Compiling the simulated server
Before compiling the project we had provided the full path of the static mosquitto library generated by mosquitto compilation. This is done in file `edscorbot-c-cpp/CMakeLists.txt`. Lookup the line:

`TARGET_LINK_LIBRARIES(simulated_server PRIVATE  /home/adalberto/tmp/mosquitto/build/lib/libmosquitto_static.a -lpthread)`
`

and reaplace the full path only with your mosquitto static library full path.

After that, the steps to compile the project are:
* `cd edscorbot-c-cpp` to enter the project folder
* `mkdir build` to create a specific output folder
* `cd build` to enter the output folder
* `cmake ..` to generate build files from the parent folder into current folder
* `make install` to compile and generate the executable of the simulated server.

### Running 
Before runnign the simulated server, you must configure the Mosquitto broker to also accept messages trough Web sockets (required by the Angular front-end). This is achived by providing the following configuration (in a suitable `mosquitto.conf` file):

```
# calls through port 1883 an from any host using default protocol
listener 1883 0.0.0.0 

# calls through port 8080 an from any host using websockets protocol
listener 8080 0.0.0.0
protocol websockets

# to allow applications to connect anonymously
allow_anonymous true
```

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

This means the server has registed the Mosquitto broker successfully, notified the existing comsumers with its meta info and is ready to send/receive messages on channels/topics `ROBOT_NAME/commands` and `ROBOT_NAME/moved`.

### Migrating to the real controller
Tehe code of simulated server has been derived from the real server `mqtt_server.cpp` and adjusted to handle messages according to the new communication model established in the [Async API specification](https://app.swaggerhub.com/apis-docs/ADALBERTOCAJUEIRO_1/ed-scorbot_async/1.0.0). Therefore, you will see a good overlap betqeen these codes. 

A good idea is to first adjust the simulated server file to have your robot characteristics and then inject the code to control your robotic arm. In the function `message_callback` you will see a specific comment where you must put your implementation to do everything as before. 

All the source code is commented. Everything you need to change to customise the code to your robot contains a `TODO:` label. Use it to be faster when customizing.

Furthermore, when putting the executable into the robotic arm's platform you must to handle all dependencies. That is, they must be available in the target platform. The project [Py-EDScorbotTool](https://github.com/RTC-research-group/Py-EDScorbotTool) already does this, as well as has a specific branch (`dev-search_Home`) with an implementation (`mocked_server.cpp`) invoking the functions to move the arm.

### Reference Documentation
For further reference, please consider the following items:
* [EDScorbot Github Project](https://github.com/RTC-research-group/Py-EDScorbotTool) - the main Github project with details about the entire project and the low level code to control the robotic arm
* [EDScorbot Documentation](https://py-edscorbottool.readthedocs.io/en/latest/) - documentation about the entire project