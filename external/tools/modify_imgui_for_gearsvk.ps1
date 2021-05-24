$imgui_h = "..\universal\include\imgui.h"
$imgui_impl_vulkan_h = "..\universal\include\imgui_impl_vulkan.h"
$imgui_impl_vulkan_cpp = "..\universal\src\imgui_impl_vulkan.cpp"

##
# Modify imgui vulkan impl header file (imgui_impl_vulkan.h)
#
# This adds:
# - User texture comment to the implemented features list at the top:
#	//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset.
#
# This removes:
# - User texture binding as missing feature comment at the top:
#	//  [ ] Renderer: User texture binding
##

"-> Modifying " + $imgui_impl_vulkan_h + ":"

# Read imgui vulkan impl header
$fileContent = Get-Content $imgui_impl_vulkan_h

# Create an empty array and use it as the modified file
$fileContentModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = "// Modified version for Gears-Vk"
$fileContentModified += $gearsVkHeader
$fileContentModified += '// Changes:'
$fileContentModified += '// - User texture binding comment via ImTextureID'
$fileContentModified += ''

$fileAlreadyModified = 0
$appliedChanges = @{
	AddedFeature = 0
	RemovedFeature = 0
}
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		$fileAlreadyModified = 1
		Write-Host "   -> Already modified" -ForegroundColor Green
		break
	}
	# Add user texture binding to implemented features list
	if ($line -match "//\s*Missing\s*features:") {
		$fileContentModified += "//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset."
		$appliedChanges["AddedFeature"] = 1
	}
	# Remove user texture binding comment from missing feature
	if ($line -match "//\s*\[ \]\s*Renderer:\s*User\s*texture\s*binding") {
		$appliedChanges["RemovedFeature"] = 1
		continue
	}

	$fileContentModified += $line
}

if (-Not $fileAlreadyModified) {
	# Write modified file content to original file
	Set-Content $imgui_impl_vulkan_h $fileContentModified
	# Output success of performed actions
	$msg = "Comments about user texture support"
	if ($appliedChanges["AddedFeature"] -eq 1 -And $appliedChanges["RemovedFeature"] -eq 1) {
		$msg = "   [X] " + $msg
		Write-Host $msg -ForegroundColor Green
	}
	else {
		$msg = "   [ ] " + $msg
		Write-Host $msg -ForegroundColor Red
	}
}


##
# Modify imgui vulkan impl source file (imgui_impl_vulkan.cpp)
#
# This adds:
# - User texture comment to the implemented features list at the top:
#	//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset.
#
# - Add code for texture descriptor set binding after "vkCmdSetScissor\(command_buffer, 0, 1, &scissor\);" in ImGui_ImplVulkan_RenderDrawData:
# 	// Bind texture descriptor set stored as ImTextureID'
# 	VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->GetTexID() };'
# 	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);'
#
# This changes:
# - The comment "// Bind pipeline and descriptor sets:" in ImGui_ImplVulkan_SetupRenderState to:
#	// Bind pipeline
#
# - The line "io.Fonts->SetTexID((ImTextureID)(intptr_t)g_FontImage);" in ImGui_ImplVulkan_CreateFontsTexture for setting the font texture id to:
#	io.Fonts->SetTexID((ImTextureID)g_DescriptorSet);
#
# This removes:
# - User texture binding as missing feature comment at the top:
#	//  [ ] Renderer: User texture binding
#
# - The unecessary font texture descriptor binding:
#	VkDescriptorSet desc_set[1] = { g_DescriptorSet };
#	vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);
##

"-> Modifying " + $imgui_impl_vulkan_cpp + ":"

# Read imgui vulkan impl source
$fileContent = Get-Content $imgui_impl_vulkan_cpp

enum FunctionName {
	SetupRenderState;
	RenderDrawData;
	CreateFontsTexture;
	Undefined
}
$currentFunction = [FunctionName]::Undefined
$aboutToEnterFunction = [FunctionName]::Undefined

$appliedChanges = @{
	[FunctionName]::SetupRenderState = 0
	[FunctionName]::RenderDrawData = 0
	[FunctionName]::CreateFontsTexture = 0
}

# Create an empty array and use it as the modified file
$fileContentModified = @()

# Header in file to communicate modification and prevent
# multiple executions of this script
$gearsVkHeader = "// Modified version for Gears-Vk"
$fileContentModified += $gearsVkHeader
$fileContentModified += '// Additions:'
$fileContentModified += '// - User texture binding via ImTextureID'
$fileContentModified += ''

$fileAlreadyModified = 0
$parentheseCounter = 0
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		Write-Host "   -> Already modified" -ForegroundColor Green
		$fileAlreadyModified = 1
		break
	}

	# Add user texture binding to implemented features list
	if ($line -match "//\s*Missing\s*features:") {
		$fileContentModified += "//  [x] Renderer: User texture binding. ImTextureID is used to store a handle to a Descriptorset."
	}
	# Remove user texture binding comment from missing feature
	if ($line -match "//\s*\[ \]\s*Renderer:\s*User\s*texture\s*binding") {
		continue
	}

	# Detect if about to enter ImGui_ImplVulkan_SetupRenderState
	if ($line -match "void\s+ImGui_ImplVulkan_SetupRenderState\(.+\)") {
		$aboutToEnterFunction = [FunctionName]::SetupRenderState
	}

	# Detect if about to enter ImGui_ImplVulkan_RenderDrawData
	if ($line -match "void\s+ImGui_ImplVulkan_RenderDrawData\(.+\)") {
		$aboutToEnterFunction = [FunctionName]::RenderDrawData
	}

	# Detect if about to enter ImGui_ImplVulkan_CreateFontsTexture
	if ($line -match "bool\s+ImGui_ImplVulkan_CreateFontsTexture\(.+\)") {
		$aboutToEnterFunction = [FunctionName]::CreateFontsTexture
	}

	# If we are about to enter a function to change, we need one '{' as indication, that we are
	# actually in the function
	if ($aboutToEnterFunction -ne [FunctionName]::Undefined) {
		$openParentheseCount = ($line.Split('{')).count - 1
		if ($openParentheseCount -gt 0) {
			$currentFunction = $aboutToEnterFunction
			$aboutToEnterFunction = [FunctionName]::Undefined
		}
	}

	# If we are in a function to change, add occurences of '{' and subtract occurences of'}' from
	# parentheseCounter. If parentheseCounter is equals to zero, we are leaving the function
	if ($currentFunction -ne [FunctionName]::Undefined) {
		$aboutToEnterFunction = [FunctionName]::Undefined
		$openParentheseCount = ($line.Split('{')).count - 1
		$closeParentheseCount = ($line.Split('}')).count - 1
		$parentheseCounter = $parentheseCounter + $openParentheseCount - $closeParentheseCount
		if ($parentheseCounter -eq 0) {
			$currentFunction = [FunctionName]::Undefined
		}
	}

	# Remove font descriptor set binding in ImGui_ImplVulkan_SetupRenderState
	if ($currentFunction -eq [FunctionName]::SetupRenderState) {
		# Change comment, stating that descriptor sets are bound
		if ($line -match "//\s*Bind\s*pipeline\s*and\s*descriptor\s*sets") {
			# Get tabs/spaces up to the first '/' to nicely align the new comment
			$matched = $line -match "^[^/]*"
			$tabs = $Matches[0]
			$fileContentModified += $tabs + "// Bind pipeline"
			continue
		}
		# Remove unecessary descriptor binding
		if ($line -match "VkDescriptorSet\s*desc_set\[1\]\s*=\s*\{\s*g_DescriptorSet\s*\};") {
			continue
		}
		if ($line -match "vkCmdBindDescriptorSets\(.+,\s*VK_PIPELINE_BIND_POINT_GRAPHICS\s*,.+,\s*0\s*,\s*1\s*,\s*desc_set\s*,\s*0\s*,.+\);") {
			# Accept the change here since this is where the actual unecessary bind happens. It is
			# not 'that' important to change the comment above or the VkDescriptorSet declaration
			# since the optimizing compiler will probably get rid of it anyways if we remove
			# vkCmdBindDescriptorSets() because desc_set is then an unused variable.
			$appliedChanges[[FunctionName]::SetupRenderState] = 1
			continue
		}
	}

	# Add descriptor set binding in ImGui_ImplVulkan_RenderDrawData
	if ($currentFunction -eq [FunctionName]::RenderDrawData) {
		if ($line -match "vkCmdSetScissor\(.+,\s*0\s*,\s*1\s*,.+\);") {
			# Add the scissor cmd line
			$fileContentModified += $line
			# Get tabs/spaces up to the 'v' of vkCmdSetScissor to nicely align the code
			$matched = $line -match "^[^v]*"
			$tabs = $Matches[0]
			# Add descriptor binding code after the vkCmdSetScissor cmd
			$fileContentModified += ''
			$fileContentModified += $tabs + '// Bind texture descriptor set stored as ImTextureID'
			$fileContentModified += $tabs + 'VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->GetTexID() };'
			$fileContentModified += $tabs + 'vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_PipelineLayout, 0, 1, desc_set, 0, NULL);'
			$appliedChanges[[FunctionName]::RenderDrawData] = 1
			continue
		}
	}

	# Change fonts texture id assignment to descriptor in ImGui_ImplVulkan_CreateFontsTexture
	if ($currentFunction -eq [FunctionName]::CreateFontsTexture) {
		if ($line -match "io.Fonts->SetTexID\(.+\);") {
			# Get tabs/spaces up to the 'i' of io.Fonts to nicely align the code
			$matched = $line -match "^[^i]*"
			$tabs = $Matches[0]
			# Asign descriptor set as ImTextureID
			$fileContentModified += $tabs + 'io.Fonts->SetTexID((ImTextureID)g_DescriptorSet);'
			$appliedChanges[[FunctionName]::CreateFontsTexture] = 1
			continue
		}
	}

	$fileContentModified += $line
}

if (-Not $fileAlreadyModified) {
	# Write modified file content to original file
	Set-Content $imgui_impl_vulkan_cpp $fileContentModified
	# Output success of performed actions
	foreach($key in $appliedChanges.keys) {
		$func = 'ImGui_ImplVulkan_' + $key
		if ($appliedChanges[$key] -eq 1) {
			$msg = "   [X] " + $func
			Write-Host $msg -ForegroundColor Green
		}
		else {
			$msg = "   [ ] " + $func
			Write-Host $msg -ForegroundColor Red
		}
	}
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

"-> Modifying " + $imgui_h + ":"

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
$appliedChange = 0
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -match $gearsVkHeader) {
		$fileAlreadyModified = 1
		Write-Host "   -> Already modified" -ForegroundColor Green
		break
	}
	# Add include imgui_user.h definition and comment why we need this
	if ($line -match "#ifdef\s*IMGUI_INCLUDE_IMGUI_USER_H") {
		$fileContentModified += '// Define this here instead of in imconfig.h because IntelliSense of VisualStudio (at least version 16.9.4) is not able to determine that imconfig.h'
		$fileContentModified += '// which is included at the top contains this definition, hence a list of errors is generated, because it does not see the function declarations in'
		$fileContentModified += '// imgui_user.h. This is only a IntelliSense problem as compilation works flawless but the wrongly created error list is a inconvenient for users.'
		$fileContentModified += '#define IMGUI_INCLUDE_IMGUI_USER_H'
		$appliedChange = 1;
	}

	$fileContentModified += $line
}

if (-Not $fileAlreadyModified) {
	# Write modified file content to original file
	Set-Content $imgui_h $fileContentModified
	# Output success of performed actions
	$msg = "Defined IMGUI_INCLUDE_IMGUI_USER_H"
	if ($appliedChange -eq 1) {
		$msg = "   [X] " + $msg
		Write-Host $msg -ForegroundColor Green
	}
	else {
		$msg = "   [ ] " + $msg
		Write-Host $msg -ForegroundColor Red
	}
}
