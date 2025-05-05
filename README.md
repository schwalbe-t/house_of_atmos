# House of Atmos
A strategy game about turning a small county into an economical powerhouse.

For questions or development updates, feel free to join my Discord server: https://discord.gg/DYRDg7fq82

## Building
To build this project for the computer you are using, run: 
```
cmake . -DMODE=release
make
./house_of_atmos
```

Alternatively, Ninja can be used:
```
cmake -B . -G Ninja -DMODE=release
ninja -j$(nproc)
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
