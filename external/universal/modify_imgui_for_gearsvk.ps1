#
# Modify imgui vulkan impl header file
#

$imguiHeaderfile = ".\include\imgui_impl_vulkan.h"

"- Modifying " + $imguiHeaderfile + ":"

# Read imgui vulkan impl header
$imguiHeader = Get-Content $imguiHeaderfile

# Create an empty array and use it as the modified file
$imguiHeaderModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = "// Modified version for Gears-Vk"
$imguiHeaderModified += $gearsVkHeader
$imguiHeaderModified += '// Additions:'
$imguiHeaderModified += '// - User texture creation function'
$imguiHeaderModified += ''

$fileAlreadyModified = 0
Foreach ($line in $imguiHeader)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		$fileAlreadyModified = 1
		Write-Host " -> already modified" -ForegroundColor Green
		break
	}
	# Add user texture binding to implemented features list
	if ($line -match "// Missing features:") {
		$imguiHeaderModified += "//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset."
	}
	# Remove user texture binding comment from missing feature
	if ($line -match "//  \[ \] Renderer: User texture binding") {
		continue
	}

	# Add ImGui_ImplVulkan_CreateTexture and ImGui_ImplVulkan_Create_Or_GetTexture API functions
	if ($line -match "^// Called by user code") {
		"  - Adding new imgui API functions"
		$imguiHeaderModified += $line
		$imguiHeaderModified += 'IMGUI_IMPL_API ImTextureID  ImGui_ImplVulkan_Create_Or_GetTexture(VkSampler sampler, VkImageView image_view, VkImageLayout image_layout); '
		Write-Host "   -> added" -ForegroundColor Green
		continue
	}

	$imguiHeaderModified += $line
}

if (-Not $fileAlreadyModified) {
	Set-Content $imguiHeaderfile $imguiHeaderModified
}


#
# Modify imgui vulkan impl source file
#

$imguiSrcfile = ".\src\imgui_impl_vulkan.cpp"

"- Modifying " + $imguiSrcfile + ":"

$gearsVkSrcExt = ".\src\imgui_impl_vulkan_gears_vk_ext.cpp"

# Read imgui vulkan impl source
$imguiSrc = Get-Content $imguiSrcfile

# Helpers for function detection
$inSetupRenderState = 0
$inRenderDrawData = 0
$inCreateFontsTexture = 0

# Create an empty array and use it as the modified file
$imguiSrcModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = "// Modified version for Gears-Vk"
$imguiSrcModified += $gearsVkHeader
$imguiSrcModified += '// Additions:'
$imguiSrcModified += '// - User texture binding and descriptor set caching'
$imguiSrcModified += ''

$fileAlreadyModified = 0
Foreach ($line in $imguiSrc)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		Write-Host " -> already modified" -ForegroundColor Green
		$fileAlreadyModified = 1
		break
	}

	# Add user texture binding to implemented features list
	if ($line -match "// Missing features:") {
		$imguiSrcModified += "//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset."
	}
	# Remove user texture binding comment from missing feature
	if ($line -match "//  \[ \] Renderer: User texture binding") {
		continue
	}

	# Detect if in ImGui_ImplVulkan_SetupRenderState
	if ($line -match "^static void ImGui_ImplVulkan_SetupRenderState\(") {
		$inSetupRenderState = 1
		"  - Modifying ImGui_ImplVulkan_SetupRenderState"
	}

	# Detect if in ImGui_ImplVulkan_RenderDrawData
	if ($line -match "^void ImGui_ImplVulkan_RenderDrawData\(") {
		$inRenderDrawData = 1
		"  - Modifying ImGui_ImplVulkan_RenderDrawData"
	}

	# Detect if in ImGui_ImplVulkan_CreateFontsTexture
	if ($line -match "^bool ImGui_ImplVulkan_CreateFontsTexture\(") {
		$inCreateFontsTexture = 1
		"  - Modifying ImGui_ImplVulkan_CreateFontsTexture"
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
			$imguiSrcModified += $tabs + "// Bind pipeline"
			continue
		}
	}

	# Add descriptor set binding in ImGui_ImplVulkan_RenderDrawData
	if ($inRenderDrawData) {
		if ($line -match "vkCmdSetScissor\(command_buffer, 0, 1, &scissor\);") {
			# Add the scissor cmd line
			$imguiSrcModified += $line
			# Get tabs/spaces up to the 'v' of vkCmdSetScissor to nicely align the code
			$matched = $line -match "^[^v]*"
			$tabs = $Matches[0]
			# Add descriptor binding code after the vkCmdSetScissor cmd
			$imguiSrcModified += ''
			$imguiSrcModified += $tabs + '// Bind texture descriptor set stored as ImTextureID'
			$imguiSrcModified += $tabs + 'VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->TextureId };'
			$imguiSrcModified += $tabs + 'vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);'
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
			$imguiSrcModified += $tabs + 'ImTextureID texID = (ImTextureID)g_DescriptorSet;'
			$imguiSrcModified += $tabs + 'io.Fonts->SetTexID(texID);'
			continue
		}
	}

	$imguiSrcModified += $line
}

if (-Not $fileAlreadyModified) {
	# function definitions and internal descriptor set caching for # ImGui_ImplVulkan_Create_Or_GetTexture
	$fileGearsVkExt = Get-Content $gearsVkSrcExt

	$imguiSrcModified += ''
	# Add to imgui vulkan backend src file
	$imguiSrcModified += $fileGearsVkExt

	Set-Content $imguiSrcfile $imguiSrcModified
}


#
# Modify imgui header file
#

$imguiHeaderfile = ".\include\imgui.h"

"- Modifying " + $imguiHeaderfile + ":"

# Read imgui vulkan impl header
$imguiHeader = Get-Content $imguiHeaderfile

# Create an empty array and use it as the modified file
$imguiHeaderModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = '// Modified version for Gears-Vk'
$imguiHeaderModified += $gearsVkHeader
$imguiHeaderModified += '// Additions:'
$imguiHeaderModified += '// - Definition of IMGUI_INCLUDE_IMGUI_USER_H'
$imguiHeaderModified += ''

$fileAlreadyModified = 0
Foreach ($line in $imguiHeader)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		$fileAlreadyModified = 1
		Write-Host " -> already modified" -ForegroundColor Green
		break
	}
	# Add include imgui_user.h definition and comment why we need this
	if ($line -match "// Include imgui_user\.h") {
		$imguiHeaderModified += '// Define this here instead of in imconfig.h because IntelliSense of VisualStudio (at least version 16.9.4) is not able to determine that imconfig.h'
		$imguiHeaderModified += '// which is included at the top contains this definition, hence a list of errors is generated, because it does not see the function declarations in'
		$imguiHeaderModified += '// imgui_user.h. This is only a IntelliSense problem as compilation works flawless but the wrongly created error list is a inconvenient for users.'
		$imguiHeaderModified += '#define IMGUI_INCLUDE_IMGUI_USER_H'
	}

	$imguiHeaderModified += $line
}

if (-Not $fileAlreadyModified) {
	Set-Content $imguiHeaderfile $imguiHeaderModified
}
