# Usage of ImGui for User Interfaces

_Gears-Vk_ offers support for usage of the graphical user interface library [Dear ImGui](https://github.com/ocornut/imgui). Its headers are not included by default.
They _can_ be used by applications by including its header:
```cpp
#include <imgui.h>
```
The example applications make use of it extensively. ImGui-specific usage description can be found under:

- Section [Usage] in its [README.md](https://github.com/ocornut/imgui).
- Detailed documentation can be found in the [Dear ImGui Wiki](https://github.com/ocornut/imgui/wiki).

## imgui_manager and Establishing Callbacks

There is one class in particular which serves as a bridge between _Gears-Vk_ and ImGui which is the `gvk::invokee`-class `gvk::imgui_manager`. 
An instance of `gvk::imgui_manager` can be added to a composition. This also means that its callback methods are invoked constantly---in particular during the `update` and `render` stages.

The pattern to use `gvk::imgui_manager` is mainly through its `add_callback` methods which allows user code to be executed.
It can be set-up during the `initialize` stage of a `gvk::invokee` as follows:

```cpp
auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
if(nullptr != imguiManager) {
	imguiManager->add_callback([this](){	
		ImGui::Begin("Hello, world!");
		ImGui::SetWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
    		ImGui::End();
  	});
}
```

This code first tries to get a pointer to an instance of `gvk::imgui_manager` which is assumed to have been added to the composition and on success, a callback is added to the returned instance via `gvk::imgui_manager::add_callback`. Inside the callback, ImGui code can be invoked as requested by the user application. All of ImGui's functionality should be accessible. 

## Rendering Textures with get_or_create_texture_descriptor

Rendering custom images and textures with ImGui can be achieved via `ImGui::Image` and is described in the Wiki article [Image Loading and Displaying Examples](https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples). It describes that custom textures can be rendered via a generic [`ImTextureId`](https://github.com/ocornut/imgui/wiki/Image-Loading-and-Displaying-Examples#about-imtextureid) image handle. 

_Gears-Vk_ offers to get a suitable `ImTextureId` via `gvk::imgui_manager::get_or_create_texture_descriptor`. It can be used as follows in conjunction with `ImGui::Image`:

```cpp
auto imguiManager = gvk::current_composition()->element_by_type<gvk::imgui_manager>();
if(nullptr != imguiManager) {
	imguiManager->add_callback([this, imguiManager](){	
		ImGui::Begin("ImTextureID usage example");
		ImTextureID imTexId = imguiManager->get_or_create_texture_descriptor(avk::referenced(mMyImageSampler)); // mMyImageSampler is of type avk::image_sampler 
		ImGui::Image(imTexId, ImVec2(512.f, 512.f));
		ImGui::End();
	});
}
```

