# Space Sim
An n-body space simulation with real-time generated planet surfaces, atmospheric scattering, and more.<br>
This simulation is mainly run on the GPU in compute and even supports Intel UHD 620.
### [Docs (Doxygen)](https://lostnthought.github.io/lostnthought/space-docs/index.html)

## Features
- **Particles:** N-body Barnes-Hut simulation with a stackless LBVH traversal based on radix sorted morton codes.
- **Atmosphere:** Rayleigh-Mie atmospheric scattering is projected onto a sphere in real time and layered with raymarched clouds via precomputed noise volumes.
- **Planet Surfaces:** Real-time chunking system pushes new jobs, which when executed generate a density field which is then meshed with marching cubes.
- **Water:** Surface-level water and its normals is generated via FFT, has transparency utilizing a depth buffer and also reflects the sky.

## Included Tech
- **Language:** C
- **Graphics:** OpenGL, GLSL
- **Libraries:** SDL2 (Windowing), Nuklear (Debug UI), uthash (Hashing), stb (Image Loading)

### Prerequisites
You will need:
- `SDL2`
- `GLEW`

### Build
```bash
make
```
Place glew32.dll, SDL2.dll, and alongside the binary in bin/. If you have any further trouble, try using the pkg-config included in the makefile.

For inputs and controls, refer to input.c.

### Building Documentation
```bash
make docs
```
Doxygen outputs to docs/.
