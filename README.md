## Prerequisites

### For Running with Docker

To build and run this project using Docker, you will need:

- Docker installed on your machine. Visit the [official Docker documentation](https://docs.docker.com/get-docker/) to get started.

### For Local Development

To build and run this project on your local machine without Docker, you will need:

- CMake: For generating makefiles and managing the build process.
- A C++ compiler (e.g., GCC or Clang) and `build-essential` package on Linux for compiling the project.

On Ubuntu, you can install these with the following commands:

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake
```

## Setup and Build Instructions

Clone this repository to your local machine:

```bash
git clone git@git.chalmers.se:courses/dit638/students/2024-group-16.git
cd 2024-group-16
```

### Using Docker

Build the Docker image:

```bash
docker build -t helloworld .
```

Run the Docker container:

```bash
docker run helloworld
```

Optionally, you can pass a name to customize the output:

```bash
docker run helloworld John
```

### For Local Development

Create a build directory and navigate to it:

```bash
mkdir build && cd build
```

Generate makefiles using CMake:

```bash
cmake ..
```

Build the project:

```bash
make
```

Run the project:

```bash
./helloworld
```

Optionally, you can pass a name to customize the output:

```bash
./helloworld John
```
