# JT808

A command generation and parsing library for the JT/T808 protocol, which is a transportation industry standard in the People's Republic of China.

## Quick Start

This project uses the `CMake` build tool, and the compiler is `G++` (for Windows systems, it is `MinGW G++`).

### Compilation Preparation

#### Ubuntu System

```bash
sudo apt-get install cmake cmake-curses-gui
```

#### Windows System

Download [MinGW](http://www.mingw.org/) from the official website, install it, and set up the environment variables.
Download [CMake](https://cmake.org/download/) from the official website, install it, and set up the environment variables.

### Compilation

```bash
cmake .. && make
```

### Compile Example Programs

```bash
cmake .. -DJT808_BUILD_EXAMPLES=ON && make
```

The compiled output files are located in the `build/examples` directory.

### Generate Debug and Release Versions of the Program

```bash
ccmake ..
```

Edit the `CMAKE_BUILD_TYPE` line and enter `Debug` or `Release`.
