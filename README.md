WIP.
Simple 3D rendering engine using the Vulkan API.

I intend to use this codebase for my own future projects, hence all "user" code is in `src/render.cpp` and `src/render_data.hpp`.

Current features:
- Model/texture loading
- Profiling
- Bindless descriptors
- Camera movement (WASD, right click to look around)
- Vulkan API abstractions
- Cross platform

To-do:
- Game engine
  - Configurable database for in-game entities (ECS)
- Object movement
- Environment rendering (terrain, grids, etc.)
