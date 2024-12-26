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

Additionally, this project uses these dependencies directly (install not needed):
- `glad`
- `stb_image`
- `stb_vorbis`
- `nlohmann/json`
- `syoyo/tinygltf`