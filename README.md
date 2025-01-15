# House of Atmos
A strategy game about turning a small county into an economical powerhouse.

## Building
To build this project for the computer you are using, run: 
```
cmake .
make
./house_of_atmos
```

This requires the following dependencies to be installed when building from source:
- `glfw` (version 3.3 or above)
- `OpenGL`
- `OpenAL` (I use `kcat/openal-soft`)

Additionally, this project uses these dependencies directly (install not needed):
- `glad`
- `stb_image`
- `stb_vorbis`
- `nlohmann/json`
- `syoyo/tinygltf`
- `Reputeless/PerlinNoise`

### Cross-Compilation to Windows

For Windows, implementations of `glfw` and `OpenAL` can be found here:
- https://www.glfw.org/download
- https://openal-soft.org/#download

Create a `windows_libs`-subdirectory at the project root with the following structure:
```sh
windows_libs/
├── bin/
│   ├── glfw3.dll
│   └── OpenAL32.dll # (renamed 'soft_oal.dll' in the case of openal-soft)
├── include/
│   ├── AL/ # directory containing OpenAL headers
│   └── GLFW/ # directory containing GLFW headers
└── lib/
    ├── libglfw3dll.a
    └── libOpenAL32.dll.a
```

The `toolchain-windows.cmake`-file then can be used when cross-compiling to Windows:
```
cmake . -DCMAKE_TOOLCHAIN_FILE=toolchain-windows.cmake
make
```
To run on Windows, make sure the contents of `windows_libs/bin/` are copied into the same directory as the one the executable is in.