# ForkerRenderer: Learn How Software Rendering Works

Implement CPU-based software rendering that mimics OpenGL behavior without using any third-party libraries, but use [spdlog](https://github.com/gabime/spdlog)
for logging though :)

![](https://bloggg-1254259681.cos.na-siliconvalley.myqcloud.com/7ecoa.jpg)

## Building

```sh
# Logging
brew install spdlog  # for macOS

# Clone
git clone https://github.com/junhaowww/ForkerRenderer.git
cd ForkerRenderer 

# Compile
mkdir build && cd build
cmake .. && make

# Execute
./ForkerRenderer obj/diablo_pose/diablo_pose.obj [RotateDegreeOnY = 0.0] [Scale = 1.0]
```

## Features

![](https://bloggg-1254259681.cos.na-siliconvalley.myqcloud.com/i892z.jpg)

- [x] Parsing `*.obj` / `*.mtl`
    - `g` defines mesh name; `usemtl` defines material name (comes in order)
    - Support `Ka`, `Kd`, `Ks`, `map_Kd`, `map_Ks`, `map_Bump`, `map_Ao`
    - Position vertex normalization
    - Auto triangulate
- [x] Geometry Template Class
    - Vector: `Vec2f`, `Vec3f`, `Vec4f`, `Vec4i`, ...
    - Matrix: `Mat3f`, `Mat4f`, `Mat3x4f`, ...
    - Support glm-style vector swizzling, e.g. `v.xyz`, `v.xy`
- [x] Rasterization
    - Bresenham's Line Algorithm (used and removed)
    - Bounding Box Method (currently used)
- [x] Shader: Blinn-Phong Shading / Depth Shading
- [x] Light: Point / Directional
- [x] Texture Mapping: Diffuse / Specular / Normal / AO
- [ ] Texture Filtering: Nearest / Linear
- [x] Normal Transformation: TBN Matrix
- [x] Camera: Orthographic / Perspective Projection
- [x] Perspective Correct Interpolation (PCI) `#define PERSPECTIVE_CORRECT_MAPPING`
- [x] Soft-Shadow Mapping with Percentage-Closer Filtering (PCF) `#define SHADOW_MAPPING`
- [x] Anti-Aliasing (SSAA) `#define ANTI_ALIASING_SSAA`
- [ ] Screen Space Ambient Occlusion (SSAO)

## Gallery

Apex Horizon (also [Dr. Mary Somers](https://www.ea.com/games/apex-legends/about/characters/horizon)), Author: Squral

<img src="https://bloggg-1254259681.cos.na-siliconvalley.myqcloud.com/9vm5c.jpg" width="600">

Sci-Fi Welding Vehicle, Author: Berk Gedik

<img src="https://bloggg-1254259681.cos.na-siliconvalley.myqcloud.com/ftuc3.jpg" width="600">

Backpack, Author: Berk Gedik

<img src="https://bloggg-1254259681.cos.na-siliconvalley.myqcloud.com/1zcq0.jpg" width="400">

African Head, Author: Vidar Rapp

<img src="https://bloggg-1254259681.cos.na-siliconvalley.myqcloud.com/vkyah.jpg" width="400">


## Structure

Approximately 3,500 lines of code:

- Rendering: `forkergl.h/cpp` (rasterization), `buffer.h/cpp` (framebuffer & z-buffer)
- Shader: `shader.h`
- Model: `model.h/cpp`, `mesh.h/cpp`, `material.h`
- Camera: `camera.h/cpp`
- Light: `light.h`
- Geometry: `geometry.h/cpp`
- Utility: `tgaimage.h/cpp`, `check.h`

## Console Output

```console
$ ./ForkerRenderer obj/horizon/horizon.obj
[info] Model File: obj/horizon/horizon.obj
[info] v# 25606, f# 48886, vt# 29859, vn# 24460, mesh# 4, mtl# 4 | normalized: true, flipTexCoordY: true
[info] [    Body] f# 14162 | map_Kd[o] map_Ks[o] map_Bump[o] map_Ao[o] | Ka(0.50, 0.50, 0.50), Kd(0.80, 0.80, 0.80), Ks(0.50, 0.50, 0.50)
[info] [    Gear] f# 21151 | map_Kd[o] map_Ks[o] map_Bump[o] map_Ao[o] | Ka(0.50, 0.50, 0.50), Kd(0.80, 0.80, 0.80), Ks(0.40, 0.40, 0.40)
[info] [    Hair] f#  7031 | map_Kd[o] map_Ks[o] map_Bump[o] map_Ao[o] | Ka(0.30, 0.30, 0.30), Kd(0.70, 0.70, 0.70), Ks(0.00, 0.00, 0.00)
[info] [    Head] f#  6542 | map_Kd[o] map_Ks[o] map_Bump[o] map_Ao[o] | Ka(0.40, 0.40, 0.40), Kd(0.70, 0.70, 0.70), Ks(0.30, 0.30, 0.30)
[info] ------------------------------------------------------------
[info] Time Used: 4.74961 Seconds (Model Loaded)
[info] ------------------------------------------------------------
[info] -- [Body] Time Used: 2.1908 Seconds
[info] -- [Gear] Time Used: 1.6237 Seconds
[info] -- [Hair] Time Used: 2.4018 Seconds
[info] -- [Head] Time Used: 0.4043 Seconds
[info] Time Used: 6.6206 Seconds (Model Rendered)
[info] ------------------------------------------------------------
[info] Time Used: 11.3702 Seconds (Total)
[info] ------------------------------------------------------------
```

## Reference

Started by following the basic workflow in _ssloy/TinyRenderer_, improved code style (e.g. `geometry.h`) when referring
to other great resources such as _mmp/pbrt-v3 & v4_, and resolved tons of issues with the help of the Internet. Thank those authors and creators who wrote the articles and built the models!

Rendering:

- [ssloy/TinyRenderer](https://github.com/ssloy/tinyrenderer)
- [mmp/pbrt-v3 & v4](https://github.com/mmp/)
- [lingqi/GAMES101](https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html)
- [Scratchapixel](https://www.scratchapixel.com/)
- [JoeyDeVries/LearnOpenGL](https://learnopengl.com/)
- [OpenGL Projection Matrix](http://www.songho.ca/opengl/gl_projectionmatrix.html)
- [The Normal Matrix](http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/)
- [Perspectively Correct Texture Mapping And Color Interpolation](http://gamma.cs.unc.edu/courses/graphics-s09/Zaferakis/hw6/)
- [Converting quadrilaterals in an OBJ file into triangles?](https://stackoverflow.com/questions/23723993/converting-quadriladerals-in-an-obj-file-into-triangles)
- [OBJ FILE FORMAT](https://www.cs.cmu.edu/~mbz/personal/graphics/obj.html)
- [Object Files (.obj)](http://paulbourke.net/dataformats/obj/)
- More and more articles...

Model Attribution:

- [Apex Horizon](https://ch.3dexport.com/3dmodel-horizon-apex-legends-321212.htm) by Squral
- [Sci-Fi Welding Vehicle](https://sketchfab.com/3d-models/sci-fi-welding-vehicle-40a215bf2244434a96f79684160e0e9f) by Berk Gedik
- [Backpack](https://sketchfab.com/3d-models/survival-guitar-backpack-low-poly-799f8c4511f84fab8c3f12887f7e6b36) by Berk Gedik
- [African Head](https://github.com/ssloy/tinyrenderer/) by Vidar Rapp
- [Diablo Pose](https://github.com/ssloy/tinyrenderer/) by Samuel (arshlevon) Sharit
- Brickwall (for debugging) by MYSELF! :)