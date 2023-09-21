# Building

## Prerequisites
1. An ISO C++17 compiler.
2. An ISO C99-compliant compiler.
3. CMake 3.10 and newer.
4. Lack of fear of using command prompts.
5. [GLFW](https://www.glfw.org/)
6. A GPU with drivers capable of comprehending OpenGL 4.5 core.

## The process
1. Clone the repo:
   ```
   git clone https://github.com/voxelius/voxelius.git
   ```
2. Go to the cloned repo:
   ```
   cd voxelius
   ```
3. Generate build files:
   ```
   cmake -B build .
   ```
4. Build the thing:
   ```
   cmake --build build
   ```
