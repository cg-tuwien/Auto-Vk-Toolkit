# How-to update imgui for Gears-Vk
- Checkout [imgui](https://github.com/ocornut/imgui)
- Copy every `.cpp` file from `imgui/` into `external/universal/src`
- Copy `imgui/backends/imgui_impl_glfw.cpp` and `imgui/backends/imgui_impl_vulkan.cpp` into `external/universal/src`
- Copy every `.h` file from `imgui/` into `external/universal/include`
- Copy `imgui/backends/imgui_impl_glfw.h` and `imgui/backends/imgui_impl_vulkan.h` into `external/universal/include`
- Open Powershell, change directory to `external/tools` and execute the script `modify_imgui_for_gearsvk.ps1`
- Check if every `.cpp` file in `external/universal/source` is included in the Visual Studio
  project
