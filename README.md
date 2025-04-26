# House of Atmos
A strategy game about turning a small county into an economical powerhouse.

For questions or development updates, feel free to join my Discord server: https://discord.gg/DYRDg7fq82

## Building
To build this project for the computer you are using, run: 
```
cmake .
make
./house_of_atmos
```

This requires the following dependencies to be installed when building from source:
- `glfw` (version 3.3 or above)
- OpenGL ES 3 and GLE
- `OpenAL` (I use `kcat/openal-soft`)

Additionally, this project uses these dependencies directly (install not needed):
- `stb_image`
- `stb_vorbis`
- `nlohmann/json`
- `syoyo/tinygltf`
- `Reputeless/PerlinNoise`
- `samhocevar/portable-file-dialogs`

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

### Installer
The installer used for releases of House of Atmos can be built using Inno Setup to generate a single installer executable.
However, the game is completely portable and will run fine as as an unzipped zip file.

# Copyright
```
House of Atmos
Copyright (C) 2024 schwalbe_t

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published
by the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
```
