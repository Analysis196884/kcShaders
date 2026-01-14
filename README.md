# kcShaders

kcShaders is a rendering engine based on OpenGL and GLSL, with a focus on real-time graphics and shader development.

<!-- Insert the figure of the UI here -->
![kcShaders_ui](images/ui.png)

## Features
Various rendering modes:
- Rasterization
- Ray Tracing
- Shadertoy

USD scene loading and rendering:
- Scene graph management
- Material and texture support

## Requirements
- OpenGL 4.3 or higher
- [OpenUSD SDK](https://developer.nvidia.cn/openusd)

## Building
```bash
git clone https://github.com/Analysis196884/kcShaders.git
cd kcShaders
mkdir build && cd build
cmake ..
make
```

## Gallery
### Rasterization:
<img src="images/sponza.png" style="width:45%; display:inline-block;"/>
<img src="images/normal.png" style="width:45%; display:inline-block;"/>

### Ray Tracing:
<img src="images/cornell.png" style="width:45%; display:inline-block;"/>
<img src="images/rtw.png" style="width:45%; display:inline-block;"/>

### Shadertoy:
<img src="images/snowfall.png" style="width:45%; display:inline-block;"/>
<img src="images/solarsystem.png" style="width:45%; display:inline-block;"/>
