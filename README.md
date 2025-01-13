# House of Atmos
A strategy game about turning a small county into an economical powerhouse.

### Building
To build this project, run: 
```
cmake .
make
./house_of_atmos
```

This requires the following dependencies to be installed when building from source:
- `glfw` (version 3.3 or above)
- `OpenGL`
- `OpenAL` (I use `kcat/openal-soft`)

For Windows, implementations of `glfw` and `OpenAL` can be found here:
- https://www.glfw.org/download
- https://openal-soft.org/#download

The `windows-toolchain.cmake`-file can be used when cross-compiling to Windows.


Additionally, this project uses these dependencies directly (install not needed):
- `glad`
- `stb_image`
- `stb_vorbis`
- `nlohmann/json`
- `syoyo/tinygltf`
- `Reputeless/PerlinNoise`