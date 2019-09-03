# Project Management with Visual Studio

The Visual Studio projects can be used for resource management and there is a post build helper tool (cgb_post_build_helper) which mainly handles deployment of the resource files to the target folder. Its executable is located under `tools/cgb_post_build_helper.exe`

In general, the main repository contains one solution file: [`cg_base.sln`](./cg_base.sln) which references multiple Visual Studio project files (`*.vcxproj`). Out of the box, they are configured for Visual Studio 2019 and require C++ with the latest language features, i.e. `/std:c++latest`.

The examples' `*.vcxproj` files are located in the [`examples` subfolder of the `visual_studio` folder](./examples), but their source code is located in the [`examples` folder in the repository's root](../examples). All examples reference the [`cg_base` library](./cg_base/) which contains the framework's code. A lot of the Visual Studio project configuration is handled via property files which are located under [`props`](./props).

## Creating a New Project

In order to create a new project that uses the **cg_base** framework, you have to reference the framework and reference the correct property files, e.g. `rendering_api_vulkan.props` for Vulkan builds or `linked_libs_debug.props` for Debug builds. The example configurations are fully configures for proper usage.

A more convenient way to create a new project could be to use the `create_new_project.exe` tool, located under [`tools/executables`](./tools/executables). It allows to copy the settings from an existing project (like one of the examples) and effectively duplicates and renames a selected project.

## Resource Management

Dependent resources are managed directly via Visual Studio project settings, more specifically, via Visual Studio's filters (stored in `*.vcxproj.filters` files). There are two special filters which are deployed by the cgb_post_build_helper to the target directory. These two are: 

* `assets`, and
* `shaders`.

The following figure shows an example of a project configuration where shaders, a 3D model file, and an ORCA scene file are assigned to these two special filters: 

![filters configuration in the orca_loader example](./docs/images/orca_loader_filters.png | width=583)

When building the project, all these resources get deployed according to their filter path to the target directory. Sub-folders in filter paths are preserved. The filter-path is the path where to load resources from, like shown in the following examples:

Load a 3D model:
```
auto sponza = cgb::model_t::load_from_file("assets/sponza_structure.obj", aiProcess_Triangulate | aiProcess_PreTransformVertices);
```

Load an ORCA scene:
```
auto orca = cgb::orca_scene_t::load_from_file("assets/sponza.fscene");
```

Load shaders and use them to create graphics pipeline:
```
auto pipeline = cgb::graphics_pipeline_for(
	cgb::vertex_shader("shaders/transform_and_pass_pos_nrm_uv.vert"),
	cgb::fragment_shader("shaders/diffuse_shading_fixed_lightsource.frag"),
	cgb::cfg::front_face::define_front_faces_to_be_counter_clockwise(),
	cgb::vertex_input_location(0, glm::vec3{}).from_buffer_at_binding(0),
	cgb::vertex_input_location(1, glm::vec2{}).from_buffer_at_binding(1),
	cgb::vertex_input_location(2, glm::vec3{}).from_buffer_at_binding(2)
);
```

### Troubleshooting

In many cases, resources can just be dragged into the appropriate filters in Visual Studio, but for some file types, Visual Studio assumes that it should build them. This happens, for instance, for 3D models in the OBJ-format. To prevent that, please set the resource's **Item Type** to `"Does not participate in build"` which can be done from the file's properties in Visual Studio (select file, and press `Alt + Enter`, navigate to the tab `"General"`, and set the `"Item Type"` to that value). Here is a screenshot of the property pages with the appropriate setting:

!["Does not participate in build" setting](./docs/images/vs_does_not_participate_in_build.png "Does not participate in build")

Sometimes, Visual Studio won't store the exact filter path immediately in the `*.vcxproj.filters` file, which results in the affected file not being deployed to the target directory. In order to ensure that a resource has been stored in the `*.vcxproj.filters` file, please try the following steps:

1. Execute `Save All` from Visual Studio's `File` menu.
2. Close and re-open Visual Studio 
3. Ensure that the `*.vcxproj.filters` file contains the correct value, which should look something like follows:

```
    <None Include="..\..\..\assets\3rd_party\models\sponza\sponza_structure.obj">
      <Filter>assets</Filter>
    </None>
```

Make sure that the `<Filter>` element is present and set to the correct value. In this case, the element's name is `<None>` because the file has been configured to `"Does not participate in build"`.

## CGB Post Build Helper 

`cgb_post_build_helper.exe` is a helper tool which is executed upon successful building of a project. It deploys all the referenced resources (i.e. everything assigned to `shaders` or `assets` filters in Visual Studio) to the target directory. It also deploys all the dependent resources. These could be all the textures which are referenced by a 3D model file.

cgb_post_build_helper will provide a tray icon, informing about the deployment process and providing lists of deployed files. The tray icon provides several actions that can be accessed by left-clicking or right-clicking it.

As an example: The [`orca_loader` example](./examples/orca_loader), should deploy 72 assets to the target directory, although only four assets and shaders are referenced. The sponza 3D model references many textures, all of which are deployed to the target folder as well. In addition to that, `.dll` files of [external dependencies](../external) are deployed to the target folder.

![72 files deployed](./docs/images/deployed_72_files.png | width=843)

If the `orca_loader` example does not deploy 72 assets, please check the hints in section about troubleshooting below.

### Troubleshooting

**Some resources can't be loaded on the first start**

cgb_post_build_helper runs asynchronously to not block Visual Studio. This has the side effect that deployment can still be in progress when the application is already starting. Please build your project and wait until cgb_post_build_helper's tray icon shows no more animated dots, before starting your application!

**DLL files are not deployed**

This is usually not a problem. Some process still accesses the `.dll` files in the target directory and prevents cgb_post_build_helper to replace them. However, in most cases the files would just be exactly the same `.dll` files as in the previous build (except you've updated them).

**Too few resources are being deployed** 

Sometimes, cgb_post_build_helper deploys too few resources which is probably due to a lack of rights from Windows, or maybe some anti virus tool is preventing something. The exact reasons for this are, unfortunately, unknown at the moment, but there are workarounds. 

You might be subject to this problem if you build the `orca_loader` example and only 6 (or fewer) files are deployed to the target directory instead of 72 files.

Please try the following steps to solve that problem:

1. Restart cgb_post_build_helper:
    * Close cgb_post_build_helper by right-clicking the tray icon and executing `Exit`. 
	* Navigate to the [`tools/executables`](./tools/executables) directory and start `cgb_post_build_helper.exe` manually.
	* If Windows warns you about the unknown source of this tool, please select to run anyways and permanently trust this executable.
2. Start cgb_post_build_helper in with administrator rights:
    * Close cgb_post_build_helper by right-clicking the tray icon and executing `Exit`. 
	* Navigate to the [`tools/executables`](./tools/executables) directory, right-click on `cgb_post_build_helper.exe`, and select `"Run as administrator"`
3. Build cgb_post_build_helper on your PC. (If the tool is built on your PC, Windows assesses it to be more trustworthy.)
    * Close cgb_post_build_helper by right-clicking the tray icon and executing `Exit` (otherwise it can't be overwritten).
	* Open the `cgb_post_build_helper.sln` Visual Studio solution, which can be found under [`tools/sources/cgb_post_build_helper`](./tools/sources/cgb_post_build_helper)
	* Select the `Release` build and build the project.
	* A custom build event will automatically overwrite the `cgb_post_build_helper.exe` in the [`tools/executables`](./tools/executables) directory, which is the location that is used for executing cgb_post_build_helper as a post build step from the examples.
    
### Settings

cgb_post_build_helper has several settings that might be helpful during the development process. They can be accessed by right-clicking on the tray icon and executing `Open Settings`.

![cgb_post_build_helper settings](./docs/images/settings_post_build_helper.png | width=798)

The setting "Always deploy release DLLs" can lead to significantly increased performance when loading textures or 3D models from Debug builds.

The settings which open popup windows if shader errors are detected can be helpful during shader development.
	
	