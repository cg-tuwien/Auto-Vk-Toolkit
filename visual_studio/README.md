# Project Management with Visual Studio

The Visual Studio projects can be used for resource management and there is a post build helper tool (cgb_post_build_helper) which mainly handles deployment of the resource files to the target folder. Its executable is located under `tools/cgb_post_build_helper.exe`

In general, the main repository contains one solution file: [`cg_base.sln`](./cg_base.sln) which references multiple Visual Studio project files (`*.vcxproj`). Out of the box, they are configured for Visual Studio 2019 and require C++ with the latest language features, i.e. `/std:c++latest`.

The examples' `*.vcxproj` files are located in the [`examples` subfolder of the `visual_studio` folder](./examples), but their source code is located in the [`examples` folder in the repository's root](../examples). All examples reference the [`cg_base` library](./cg_base/) which contains the framework's code. A lot of the Visual Studio project configuration is handled via property files which are located under [`props`](./props).

## Creating a New Project

In order to create a new project that uses the **cg_base** framework, you have to reference the framework and reference the correct property files, e.g. `rendering_api_vulkan.props` for Vulkan builds or `linked_libs_debug.props` for Debug builds. The example configurations are fully configures for proper usage.

A more convenient way to create a new project could be to use the `create_new_project.exe` tool, located under [`tools/executables`](./tools/executables). It allows to copy the settings from an existing project (like one of the examples) and effectively duplicates and renames a selected project.

## Asset Management

Assets required for an application are managed directly via Visual Studio project settings, more specifically, via Visual Studio's filters. The data is stored in `*.vcxproj.filters` files. There are two special filters which are deployed by the cgb_post_build_helper to the target directory. These two are: 

* `assets`, and
* `shaders`.

The following figure shows an example of a project configuration where shaders, a 3D model file, and an ORCA scene file are assigned to these two special filters: 

<img src="./docs/images/orca_loader_filters.png" width="292"/>

When building the project, all these resources get deployed _according to their **filter path**_ to the target directory. Sub-folders in filter paths are preserved. The filter-path is the path where to load resources from, like shown in the following examples:

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

### Dependent Assets

Some assets have dependent assets. This mostly applies to 3D models which reference textures. All the dependent assets are deployed to the target directory as well. You don't need to add them manually to the filters file, but you have to make sure that the asset paths in the "root asset" **point to valid paths to the dependent assets**. Also, make sure that dependent assets are **in the same folder or in a subfolder** w.r.t. to the root asset. If they aren't, it might be impossible to deploy them properly, because they have to remain in the same relative directory to the root assets (this might create a mess on your file system if the dependent assets weren't be in the same or in a sub-directory of the root asset).

**Example:**     

You add a `model.obj` file directly to your `assets` filter in your Visual Studio project. Let's assume that the `model.obj` file has an associated `model.mat` file, containing the materials, and let's further assume that the `.mat` file references the textures `texture01.jpg` and `normal_maps/texture02.png`. The following files will be deployed to the target directory:

* `assets/model.obj`
* `assets/model.mat`
* `assets/texture01.jpg`
* `assets/normal_maps/texture02.png`

**Dependent assets not present/not at the right path:**

If the cgb_post_build_helper notices that a dependent asset is not present or located at a path which is not a the same path or a sub-path w.r.t. the root asset, it will issue a warning. You might still be able to compile a working project configuration by assigning all your dependent asset to the right filters in Visual Studio and just ignore cgb_post_build_helper's warnings. 

For the example above, you'd have to create the following filters structure in your Visual Studio project:

* `assets/`
  * `model.obj`
  * `model.mat`
  * `texture01.jpg`
  * `normal_maps/`
    * `texture02.png`

### Known Issues and Troubleshooting w.r.t. Asset Handling

#### Build errors when adding assets

In many cases, assets can just be dragged into the appropriate filters in Visual Studio, but for some file types, Visual Studio assumes that it should build them. This happens, for instance, for 3D models in the OBJ-format. To prevent that, please set the assets' **Item Type** to `"Does not participate in build"` which can be done from the file's properties in Visual Studio (select file, and press `Alt + Enter`, navigate to the tab `"General"`, and set the `"Item Type"` to that value). Here is a screenshot of the property pages with the appropriate setting:

<img src="./docs/images/vs_does_not_participate_in_build.png" width="825"/>

#### Asset is not deployed because it is not saved in the Visual Studio's filters-file

Sometimes, Visual Studio won't store the exact filter path immediately in the `*.vcxproj.filters` file, which results in the affected file not being deployed to the target directory. In order to ensure that a asset has been stored in the `*.vcxproj.filters` file, please try the following steps:

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

`cgb_post_build_helper.exe` is a helper tool which is executed upon successful building of a project. It deploys all the referenced assets (i.e. everything assigned to `shaders` or `assets` filters in Visual Studio) to the target directory. It also deploys all the dependent assets. These could be all the textures which are referenced by a 3D model file.

cgb_post_build_helper will provide a tray icon, informing about the deployment process and providing lists of deployed files. The tray icon provides several actions that can be accessed by left-clicking or right-clicking it.

As an example: The [`orca_loader` example](./examples/orca_loader), should deploy 72 assets to the target directory, although only four assets and shaders are referenced. The sponza 3D model references many textures, all of which are deployed to the target folder as well. In addition to that, `.dll` files of [external dependencies](../external) are deployed to the target folder.

<img src="./docs/images/deployed_72_files.png" width="422"/>

If the `orca_loader` example does not deploy 72 assets, please check the hints in section about troubleshooting below.

### Known Issues and Troubleshooting w.r.t. CGB Post Build Helper

#### Application could not start at first try (maybe due to missing DLLs)

cgb_post_build_helper runs asynchronously to not block Visual Studio. This has the side effect that deployment can still be in progress when the application is already starting. Please build your project and wait until cgb_post_build_helper's tray icon shows no more animated dots, before starting your application!

#### Error message about denied access to DLL files (DLLs are not re-deployed)

This is usually not a problem. Some process still accesses the `.dll` files in the target directory and prevents cgb_post_build_helper to replace them. However, in most cases the files would just be exactly the same `.dll` files as in the previous build (except you've updated them). If it turns out to be a real issue, please submit an [Issue](https://github.com/cg-tuwien/cg_base/issues) and as a first-aid measure, try to kill all processes which might access the affected DLL files (closing console windows, closing Windows Explorer, exiting the CGB Post Build Helper).

#### Too few resources are being deployed

Sometimes, cgb_post_build_helper deploys too few resources which is probably due to a lack of rights from Windows, or maybe some anti virus tool is preventing some type of access. The exact reasons for this are, unfortunately, unknown at the moment, but there are workarounds. 

For example, you might be subject to this problem if you build the `orca_loader` example and only 6 (or fewer) files are deployed to the target directory instead of 72 files.

Please try one or all of the following approaches to solve that problem:

1. Restart cgb_post_build_helper:
    1. Close cgb_post_build_helper by right-clicking the tray icon and executing `Exit`. 
	2. Navigate to the [`tools/executables`](./tools/executables) directory and start `cgb_post_build_helper.exe` manually.
	3. If Windows warns you about the unknown source of this tool, please select to run anyways and permanently trust this executable.
2. Start cgb_post_build_helper in with administrator rights:
    1. Close cgb_post_build_helper by right-clicking the tray icon and executing `Exit`. 
	2. Navigate to the [`tools/executables`](./tools/executables) directory, right-click on `cgb_post_build_helper.exe`, and select `"Run as administrator"`
3. Build cgb_post_build_helper on your PC. (If the tool is built on your PC, Windows assesses it to be more trustworthy.)
    1. Close cgb_post_build_helper by right-clicking the tray icon and executing `Exit` (otherwise it can't be overwritten).
	2. Open the `cgb_post_build_helper.sln` Visual Studio solution, which can be found under [`tools/sources/cgb_post_build_helper`](./tools/sources/cgb_post_build_helper)
	3. Select the `Release` build and build the project.
	4. A custom build event will automatically overwrite the `cgb_post_build_helper.exe` in the [`tools/executables`](./tools/executables) directory, which is the location that is used for executing cgb_post_build_helper as a post build step from the examples.
	
At least the third approach should solve the problem. (If not, please create an [Issue](https://github.com/cg-tuwien/cg_base/issues) and describe your situation in detail.)

#### Slow performance when showing lists within CGB Post Build Helper

Restart the tool to clear the internal lists. Right click on the tray icon, and select `Exit`. A new instance of CGB Post Build Helper will start at the next build. Please note, however, that all the existing lists will be gone after exiting CGB Post Build Helper. They are not persisted.

### Settings

cgb_post_build_helper has several settings that might be helpful during the development process. They can be accessed by right-clicking on the tray icon and executing `Open Settings`.

<img src="./docs/images/settings_post_build_helper.png" width="399"/>

The setting "Always deploy release DLLs" can lead to significantly increased performance when loading textures or 3D models from Debug builds.

The settings which open popup windows if shader errors are detected can be helpful during shader development.
	
	
