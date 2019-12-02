# Towards Realtime Monte Carlo Pathtracing



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


## Project Outline

### main.cpp


### mainOMP.cpp


### util


### scenes


### ml


