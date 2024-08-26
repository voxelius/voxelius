# Building Voxelius
## Generating project files
* Supply `CMAKE_BUILD_TYPE` to specify debug/release/something-else build type.  
* Supply `BUILD_CLIENT` to manually control whether you want to build a client-side executable or not.  
* Supply `BUILD_SERVER` to manually control whether you want to build a server-side executable or not.  

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

## Building the project
* If your environment supports redirecting command output as a string argument and you have a POSIX `nproc`
utility installed, you can enable threaded building by passing `-j $(nproc)` to the command line.  
* For compatibility it is also recommended to specify `--config <buildtype>` on Win32 and compatible systems.  

```
cmake --build build --config Release -j $(nproc)
```

## Commands to clone and build RELEASE on Linux
```
git clone https://github.com/voxelius/voxelius.git && cd voxelius
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j $(nproc)
```

## Commands to clone and build RELEASE on Windows
```
git clone https://github.com/voxelius/voxelius.git
cd voxelius
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
