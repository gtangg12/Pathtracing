# Towards Realtime Monte Carlo Pathtracing on a Single CPU
We propose a pipeline that allows interactive path tracing without a GPU. We implement a pathtracer in serial that supports global illumination and ray-bundle tracing as well as a seperate version in parallel that supports global illumination, ray-bundle tracing, and distributed rendering of multiple views in succession (for data generation). The build/search algorithms of the KD-Tree (acceleration structure) are improved to yeild a 40% speed increase and 30% memory usage decrease. A lightweight denoising and a lightweight superresolution network are trained on a scene-by-scene basis to improve the quality of low-ray budget global illumination renderings and integrated into the pipeline as post-processing. Currently four scenes are supported by the pipeline: cbox, car, room, and sponza, and they can be run in the demo below. Our pipeline's renderings are very similiar to those produced by path tracing only, and the total time is improved 1000-5000x.


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
There are two demos: main (runs in serial) and mainOMP (runs in parallel on multiple cores). Both allow you to explore the scene with WASD to move and left/right arrow to turn. Each on has a set of options that can be enabled with optional arguments.

```
./main [scene] -gi -rb
```
```
mpirun ./mainOMP [scene] -gi -rb -rm -pp
```

where [scene] is a placeholder for the available scenes: cbox, car, room, or sponza. The optional arguments are defined below:

```
-gi global illumination
-rb ray-bundle tracing
-rm render multiple views as specified in scenepath.txt (alternative to free exploration)
-pp ai post-processing
```

The current number of cores is set to 6 in mainOMP, the number of cores on a Mac 15 in Pro. You can check the amount of cores your Mac has with

```
htop
```

and dividing by 2 (each core has two threads, which shows up as 'virtual' cores). Then change WORKERS in mainOMP to cores-1 and run make again. If you want to modify and make main, then change mainOMP to main in CMakeLists.txt.

## Project Outline

### main.cpp
Serial implementation of pathtracer. 

### mainOMP.cpp
Parallel implementation of pathtracer that supports more functionalities than serial version.

### util
Contains supporting code for pathtracer.

### scenes
Contains scene information and paths, which is used to specify rendering path for rendering multiple option. 

### ml
Contains machine learning code and models for denoising and superresolution.


