# Auto-Vk-Toolkit Versions

Notable _Auto-Vk-Toolkit_ versions in date-wise decending order:

### v0.99

**Date:** 03.06.2023

Major changes:
- Gone is the overly huge precompiled header (PCH) configuration.
    - A much smaller PCH project configuration is now used, mostly containing the externals.
	- This should get rid of some bad compile/build experiences.
- Ported `static_meshlets` and `skinned_meshlets` examples to using both `VK_EXT_mesh_shader` and `VK_NV_mesh_shader`.
- Added a new camera: `orbit_camera`
    - It is used in several example applications: `model_loader`, `orca_loader`, `static_meshlets`, and `skinned_meshlets`.

### v0.98

**Date:** 27.07.2022          
First version number.   
  
Renamed repository from _Gears-Vk_ to _Auto-Vk-Toolkit_.

Major changes:
- Ported all examples to new synchronization and commands usage provided by _Auto-Vk_
- Added new example: `multiple_queues`
- Added new example: `present_from_compute`
