# ForkerRenderer: CPU-Based Software Rasterizer, A Tiny OpenGL 🐼

Implement CPU-based software rasterizer that mimics OpenGL behavior without using any third-party libraries, but use [spdlog](https://github.com/gabime/spdlog)
for logging though :)

![](https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Head_1.jpg)

![](https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Head_2.jpg)

## Building & Usage 🔨

```sh
# Logging
brew install spdlog  # for macOS

# Clone
git clone https://github.com/junhaowww/ForkerRenderer.git
cd ForkerRenderer 

# Compile
mkdir build && cd build
cmake .. && make

# Usage: <filename> <rotate degree on y-axis>(optional) <scale factor>(optional)
./ForkerRenderer ../obj/diablo_pose/diablo_pose.obj -10.0 1.0
```

## Features ⭐

![](https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Mapping.jpg)

- [x] Parsing `*.obj` / `*.mtl`
    - `g` defines mesh name; `usemtl` defines m_Material name (comes in order)
    - [x] Support `Ka`, `Kd`, `Ks`, `map_Kd`, `map_Ks`, `map_Bump`, `map_Ao`
    - [x] Position vertex normalization
    - [x] Auto triangulation
- [x] Geometry Template Class
    - Vector: `Vector2f`, `Vector3f`, `Vector4f`, `Vector4i`, ...
    - Matrix: `Matrix3f`, `Matrix4f`, `Matrix3x4f`, ...
    - [x] Support glm-style vector swizzling, e.g. `v.xyz`, `v.xy`
- [x] Rasterization
    - [x] Bresenham's Line Algorithm (used and removed)
    - [x] Bounding Box Method (currently used)
- [x] Shader: Blinn-Phong Shading / Depth Shading
- [x] Light: Point / Directional
  - AreaLight is defined by `AREA_LIGHT_SIZE` in shadow mapping)
- [x] Texture Mapping: Diffuse / Specular / Normal / Ambient Occlusion
- [x] Texture Wrapping: NoWrap / ClampToEdge / Repeat / MirroredRepeat `Texture::WrapMode`

![](https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Wrap.jpeg)

- [x] Texture Filtering: Nearest / Linear (Bilinear) `Texture::FilterMode`

<img src="https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Filter.jpg" m_Height="230">

- [x] Normal Transformation: TBN Matrix
  - Generate and average m_Tangents for each vertex when loading the model
- [x] Camera: Orthographic / Perspective Projection
- [x] Perspective Correct Interpolation (PCI) `#define PERSPECTIVE_CORRECT_INTERPOLATION`

![](https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Shadow.jpg)

- [x] Shadow Effect
  - [x] Hard Shadow: Shadow Mapping `#define SHADOW_PASS`
  - [x] Soft Shadow:
    - [x] Percentage-Closer Filtering (PCF) `#define SOFT_SHADOW_PCF`
    - [x] Percentage-Closer Soft Shadow (PCSS) `#define SOFT_SHADOW_PCSS`
```cpp
#ifdef SOFT_SHADOW_PCF
    // Percentage-Closer Filtering (PCF)
    visibility = PCF(uShadowBuffer, shadowCoord, bias, PCF_FILTER_SIZE);
#elif defined(SOFT_SHADOW_PCSS)
    // Percentage-Closer Soft Shadow (PCSS)
    visibility = PCSS(uShadowBuffer, shadowCoord, bias);
#else
    // Hard Shadow
    visibility = hardShadow(uShadowBuffer, shadowCoord, bias);
#endif
```
- [x] Anti-Aliasing (AA)
  - [x] Trivial anti-aliasing `#define ANTI_ALIASING`
  - [ ] MSAA
  - [ ] TSA
- [ ] Screen Space Ambient Occlusion (SSAO)

## Gallery 🖼️

Apex Horizon (also [Dr. Mary Somers](https://www.ea.com/games/apex-legends/about/characters/horizon)), author: Squral

<img src="https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Gallery_1.jpg" m_Width="600">

Mary, author: TAs from [GAMES202: Real-time High Quality Rendering](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)

<img src="https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Gallery_5.jpg" m_Width="600">

Sci-Fi Welding Vehicle, author: Berk Gedik

<img src="https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Gallery_2.jpg" m_Width="600">

Backpack, author: Berk Gedik

<img src="https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Gallery_3.jpg" m_Width="400">

African Head, author: Vidar Rapp

<img src="https://raw.githubusercontent.com/junhaowww/StorageBaseWithoutCatNotice/main/ForkerRendererPic/ForkerRenderer_Gallery_4.jpg" m_Width="400">


## Structure 📁

Approximately **4,300** lines of code:

- Rendering: `forkergl.h/cpp` (rasterization), `buffer.h/cpp` (framebuffer & z-buffer)
- Shader: `shader.h`
- Model: `model.h/cpp`, `mesh.h/cpp`, `m_Material.h`, `texture.h`
- Camera: `camera.h/cpp`
- Light: `light.h`
- Color: `color.h`
- Geometry: `geometry.h/cpp`
- Utility: `tgaimage.h/cpp`, `check.h`, `utility.h`, `constant.h`, `stringprint.h` (PBRT-v3)

## Console Output 📜

```console
$ ./ForkerRenderer obj/diablo_pose/diablo_pose.obj -10
[info] Model File: obj/diablo_pose/diablo_pose.obj
[info] v# 2519, f# 5022, vt# 3263, vn# 2519, tg# 2519, mesh# 1, mtl# 1 | normalized: true, generateTangent: true, flipTexCoordY: true
[info] [Diablo_Pose] f#  5022 | map_Kd[o] map_Ks[o] map_Bump[o] map_Ao[x] | Ka(0.10, 0.10, 0.10), Kd(0.81, 0.81, 0.81), Ks(0.20, 0.20, 0.20)
[info] ------------------------------------------------------------
[info] <Time Used: 0.381728 Seconds (Model Loaded)>
[info] 
[info] Shadow Pass:
[info] --> [Diablo_Pose] Time Used: 0.155560 Seconds
[info] Output TGA File: output/output_shadowmap.tga
[info] <Time Used: 0.768311 Seconds (Shadow Mapping Finished)>
[info] 
[info] Color Pass:
[info] --> [Diablo_Pose] Time Used: 3.55731 Seconds
[info] <Time Used: 3.56657 Seconds (Model Rendered)>
[info] Output TGA File: output/output_framebuffer.tga
[info] Output TGA File: output/output_zbuffer.tga
```

## Reference 📚

Started by following the basic workflow in _ssloy/TinyRenderer_, improved code style (e.g. `geometry.h`) when referring
to other great resources such as _mmp/pbrt-v3 & v4_, and resolved tons of issues with the help of the Internet. Thank those authors and creators who wrote the articles and built the models!

Rendering:

- [ssloy/TinyRenderer](https://github.com/ssloy/tinyrenderer)
- [mmp/pbrt-v3 & v4](https://github.com/mmp/)
- [lingqi/GAMES101](https://sites.cs.ucsb.edu/~lingqi/teaching/games101.html)
- [lingqi/GAMES202](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)
- [Scratchapixel](https://www.scratchapixel.com/)
- [JoeyDeVries/LearnOpenGL](https://learnopengl.com/)
- [OpenGL Projection Matrix](http://www.songho.ca/opengl/gl_projectionmatrix.html)
- [The Normal Matrix](http://www.lighthouse3d.com/tutorials/glsl-12-tutorial/the-normal-matrix/)
- [Perspectively Correct Texture Mapping And color Interpolation](http://gamma.cs.unc.edu/courses/graphics-s09/Zaferakis/hw6/)
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
- Mary by TAs in [GAMES202: Real-time High Quality Rendering](https://sites.cs.ucsb.edu/~lingqi/teaching/games202.html)
- Brickwall, Plane, Catbox (for debugging) by MYSELF! :)
