# Towards Realtime Monte Carlo Pathtracing on a Single CPU



## Getting Started

These instructions will allow you to run a demo of the main pipeline on MacOS.

### Prerequisites

You must have the following libraries. To install them, run the folowing commands:

```
pip3 install opencv-python
pip3 install tensorflow
pip3 install keras
brew install cmake
brew install opencv
brew install openmpi
```

In addition, MacOS lacks the #include <bits/stdc++.h> header. Do the following to add it

```
cd /usr/local/include/
mkdir bits
```
Copy the file from here into bits: https://gist.github.com/reza-ryte-club/97c39f35dab0c45a5d924dd9e50c445f.

Next do
```
cmake .
make
```

### Running Demo
There are two demos: main (runs in serial) and mainOMP (runs in parallel on multiple cores). Both allow you to explore the scene with WASD to move and left/right arrow to turn. Each on has a set of options that can be enabled with optional arguments:

```
./main [scene] -gi -rb
mpirun ./mainOMP [scene] -gi -rb -rm -pp
```

where [scene] is a placeholder for the available scenes: cbox, car, room, or sponza. The optional arguments are defined below:

```
-gi global illumination
-rb ray-bundle tracing
-rm render multiple view as specified in scenepath.txt (alternative to free exploration)
-pp ai post-processing
```

The current number of cores is set to 6 in mainOMP, the number of cores on a Mac 15 in Pro. You can check the amount of cores your Mac has with

```
htop
```

and dividing by 2 (each core has two threads, which shows up as 'virtual' cores). Then change WORKERS in mainOMP to cores-1 and run make again. If you want to modify and make main, then change mainOMP to main in CMakeLists.txt.

## Project Outline

### main.cpp


### mainOMP.cpp


### util


### scenes


### ml


