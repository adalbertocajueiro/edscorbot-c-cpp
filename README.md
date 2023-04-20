#Getting started

install basic dependencies:

sudo apt-get update 
sudo apt-get install libcjson-dev
sudo apt-get install xsltproc

option(WITH_STATIC_LIBRARIES "Build static versions of the libmosquitto/pp libraries?" ON)

cd mosqquitto
mkdir build
cd build
cmake ..
make

