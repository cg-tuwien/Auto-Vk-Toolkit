$imgui_h = "..\universal\include\imgui.h"
$imgui_impl_vulkan_h = "..\universal\include\imgui_impl_vulkan.h"
$imgui_impl_vulkan_cpp = "..\universal\src\imgui_impl_vulkan.cpp"
$imgui_impl_vulkan_gears_vk_ext = ".\imgui_impl_vulkan_gears_vk_ext.cpp"

##
# Modify imgui vulkan impl header file (imgui_impl_vulkan.h)
#
# This adds:
# - User texture comment to the implemented features list at the top:
#	//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset.
#
# - API call to get or create a texture
#	IMGUI_IMPL_API ImTextureID  ImGui_ImplVulkan_Create_Or_GetTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
#
# This removes:
# - User texture binding as missing feature comment at the top:
#	//  [ ] Renderer: User texture binding
##

"- Modifying " + $imgui_impl_vulkan_h + ":"

# Read imgui vulkan impl header
$fileContent = Get-Content $imgui_impl_vulkan_h

# Create an empty array and use it as the modified file
$fileContentModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = "// Modified version for Gears-Vk"
$fileContentModified += $gearsVkHeader
$fileContentModified += '// Additions:'
$fileContentModified += '// - User texture creation function'
$fileContentModified += ''

$fileAlreadyModified = 0
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		$fileAlreadyModified = 1
		Write-Host " -> already modified" -ForegroundColor Green
		break
	}
	# Add user texture binding to implemented features list
	if ($line -match "// Missing features:") {
		$fileContentModified += "//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset."
	}
	# Remove user texture binding comment from missing feature
	if ($line -match "//  \[ \] Renderer: User texture binding") {
		continue
	}

	# Add ImGui_ImplVulkan_CreateTexture and ImGui_ImplVulkan_Create_Or_GetTexture API functions
	if ($line -match "^// Called by user code") {
		Write-Host "  - Adding new imgui API functions"
		$fileContentModified += $line
		$fileContentModified += 'IMGUI_IMPL_API ImTextureID  ImGui_ImplVulkan_Create_Or_GetTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);'
		Write-Host "   -> added" -ForegroundColor Green
		continue
	}

	$fileContentModified += $line
}

if (-Not $fileAlreadyModified) {
	Set-Content $imgui_impl_vulkan_h $fileContentModified
}


##
# Modify imgui vulkan impl source file (imgui_impl_vulkan.cpp)
#
# This adds:
# - User texture comment to the implemented features list at the top:
#	//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset.
#
# - API call to get or create a texture
#	IMGUI_IMPL_API ImTextureID  ImGui_ImplVulkan_Create_Or_GetTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout);
#
# - Add code for texture descriptor set binding after "vkCmdSetScissor\(command_buffer, 0, 1, &scissor\);" in ImGui_ImplVulkan_RenderDrawData:
# 	// Bind texture descriptor set stored as ImTextureID'
# 	VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->TextureId };'
# 	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);'
#
# This changes:
# - The comment "// Bind pipeline and descriptor sets:" in ImGui_ImplVulkan_SetupRenderState to:
#	// Bind pipeline
#
# - The line "io.Fonts->SetTexID((ImTextureID)(intptr_t)g_FontImage);" in ImGui_ImplVulkan_CreateFontsTexture for setting the font texture id to:
#	ImTextureID texID = (ImTextureID)g_DescriptorSet;
#	io.Fonts->SetTexID(texID);
#
# This removes:
# - User texture binding as missing feature comment at the top:
#	//  [ ] Renderer: User texture binding
#
# - The unecessary font texture descriptor binding:
#	VkDescriptorSet desc_set[1] = { g_DescriptorSet };
#	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);
##

"- Modifying " + $imgui_impl_vulkan_cpp + ":"

# Read imgui vulkan impl source
$fileContent = Get-Content $imgui_impl_vulkan_cpp

# Helpers for function detection
$inSetupRenderState = 0
$inRenderDrawData = 0
$inCreateFontsTexture = 0

# Create an empty array and use it as the modified file
$fileContentModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = "// Modified version for Gears-Vk"
$fileContentModified += $gearsVkHeader
$fileContentModified += '// Additions:'
$fileContentModified += '// - User texture binding and descriptor set caching'
$fileContentModified += ''

$fileAlreadyModified = 0
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		Write-Host " -> already modified" -ForegroundColor Green
		$fileAlreadyModified = 1
		break
	}

	# Add user texture binding to implemented features list
	if ($line -match "// Missing features:") {
		$fileContentModified += "//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset."
	}
	# Remove user texture binding comment from missing feature
	if ($line -match "//  \[ \] Renderer: User texture binding") {
		continue
	}

	# Detect if in ImGui_ImplVulkan_SetupRenderState
	if ($line -match "^static void ImGui_ImplVulkan_SetupRenderState\(") {
		$inSetupRenderState = 1
		Write-Host "  - Modifying ImGui_ImplVulkan_SetupRenderState"
	}

	# Detect if in ImGui_ImplVulkan_RenderDrawData
	if ($line -match "^void ImGui_ImplVulkan_RenderDrawData\(") {
		$inRenderDrawData = 1
		Write-Host "  - Modifying ImGui_ImplVulkan_RenderDrawData"
	}

	# Detect if in ImGui_ImplVulkan_CreateFontsTexture
	if ($line -match "^bool ImGui_ImplVulkan_CreateFontsTexture\(") {
		$inCreateFontsTexture = 1
		Write-Host "  - Modifying ImGui_ImplVulkan_CreateFontsTexture"
	}

	# Detect if the function we have modified is left by relying on consistent code
	# formatting, i.e. the closing '}' is at the beginning of the line
	if (($inRenderDrawData -Or $inSetupRenderState -Or $inCreateFontsTexture) -And $line -match "^}") {
		if ($inSetupRenderState) {
			Write-Host "   -> modified" -ForegroundColor Green
			$inSetupRenderState = 0
		}
		if ($inRenderDrawData) {
			Write-Host "   -> modified" -ForegroundColor Green
			$inRenderDrawData = 0
		}
		if ($inCreateFontsTexture) {
			Write-Host "   -> modified" -ForegroundColor Green
			$inCreateFontsTexture = 0
		}
	}

	# Remove font descriptor set binding in ImGui_ImplVulkan_SetupRenderState
	if ($inSetupRenderState) {
		# Remove unecessary descriptor binding
		if (($line -match "VkDescriptorSet desc_set\[1\] = \{ g_DescriptorSet \};") -Or
			($line -match "vkCmdBindDescriptorSets\(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL\);")) {
			continue
		}
		# Change comment, stating that descriptor sets are bound
		if ($line -match "// Bind pipeline and descriptor sets:") {
			# Get tabs/spaces up to the first '/' to nicely align the new comment
			$matched = $line -match "^[^/]*"
			$tabs = $Matches[0]
			$fileContentModified += $tabs + "// Bind pipeline"
			continue
		}
	}

	# Add descriptor set binding in ImGui_ImplVulkan_RenderDrawData
	if ($inRenderDrawData) {
		if ($line -match "vkCmdSetScissor\(command_buffer, 0, 1, &scissor\);") {
			# Add the scissor cmd line
			$fileContentModified += $line
			# Get tabs/spaces up to the 'v' of vkCmdSetScissor to nicely align the code
			$matched = $line -match "^[^v]*"
			$tabs = $Matches[0]
			# Add descriptor binding code after the vkCmdSetScissor cmd
			$fileContentModified += ''
			$fileContentModified += $tabs + '// Bind texture descriptor set stored as ImTextureID'
			$fileContentModified += $tabs + 'VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->TextureId };'
			$fileContentModified += $tabs + 'vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);'
			continue
		}
	}

	# Change fonts texture id assignment to descriptor in ImGui_ImplVulkan_CreateFontsTexture
	if ($inCreateFontsTexture) {
		if ($line -match "io.Fonts->SetTexID\(\(ImTextureID\)\(intptr_t\)g_FontImage\);") {
			# Get tabs/spaces up to the 'i' of io.Fonts to nicely align the code
			$matched = $line -match "^[^i]*"
			$tabs = $Matches[0]
			# Asign descriptor set as ImTextureID
			$fileContentModified += $tabs + 'ImTextureID texID = (ImTextureID)g_DescriptorSet;'
			$fileContentModified += $tabs + 'io.Fonts->SetTexID(texID);'
			continue
		}
	}

	$fileContentModified += $line
}

if (-Not $fileAlreadyModified) {
	# function definitions and internal descriptor set caching for # ImGui_ImplVulkan_Create_Or_GetTexture
	$fileContentGearsVkExtension = Get-Content $imgui_impl_vulkan_gears_vk_ext

	$fileContentModified += ''
	# Add to imgui vulkan backend src file
	$fileContentModified += $fileContentGearsVkExtension

	Set-Content $imgui_impl_vulkan_cpp $fileContentModified
}


##
# Modify imgui header file (imgui.h)
#
# This adds:
# - Define and comment for imgui user header inclusion at the end of the file above the line "#ifdef IMGUI_INCLUDE_IMGUI_USER_H":
#	// Include imgui_user.h
# 	// Define this here instead of in imconfig.h because IntelliSense of VisualStudio (at least version 16.9.4) is not able to determine that imconfig.h
# 	// which is included at the top contains this definition, hence a list of errors is generated, because it does not see the function declarations in
# 	// imgui_user.h. This is only a IntelliSense problem as compilation works flawless but the wrongly created error list is a inconvenient for users.
# 	#define IMGUI_INCLUDE_IMGUI_USER_H'
##

"- Modifying " + $imgui_h + ":"

# Read imgui vulkan impl header
$fileContent = Get-Content $imgui_h

# Create an empty array and use it as the modified file
$fileContentModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = '// Modified version for Gears-Vk'
$fileContentModified += $gearsVkHeader
$fileContentModified += '// Additions:'
$fileContentModified += '// - Definition of IMGUI_INCLUDE_IMGUI_USER_H'
$fileContentModified += ''

$fileAlreadyModified = 0
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		$fileAlreadyModified = 1
		Write-Host " -> already modified" -ForegroundColor Green
		break
	}
	# Add include imgui_user.h definition and comment why we need this
	if ($line -match "// Include imgui_user\.h") {
		$fileContentModified += '// Define this here instead of in imconfig.h because IntelliSense of VisualStudio (at least version 16.9.4) is not able to determine that imconfig.h'
		$fileContentModified += '// which is included at the top contains this definition, hence a list of errors is generated, because it does not see the function declarations in'
		$fileContentModified += '// imgui_user.h. This is only a IntelliSense problem as compilation works flawless but the wrongly created error list is a inconvenient for users.'
		$fileContentModified += '#define IMGUI_INCLUDE_IMGUI_USER_H'
		Write-Host "   -> modified" -ForegroundColor Green
	}

	$fileContentModified += $line
}

if (-Not $fileAlreadyModified) {
	Set-Content $imgui_h $fileContentModified
}
