$imgui_impl_vulkan_h = "..\universal\include\imgui_impl_vulkan.h"
$imgui_impl_vulkan_cpp = "..\universal\src\imgui_impl_vulkan.cpp"

# Header for the beginning of files to communicate the modifications and prevent multiple executions of this script
$gearsVkHeader = @()
$gearsVkHeader += "/** Modified version for Gears-Vk"
$gearsVkHeader += " *  This backend was modified for Gears-Vk to support user texture binding via ImTextureID."
$gearsVkHeader += " *"
$gearsVkHeader += " *  Changes in imgui_impl_vulkan.cpp:"
$gearsVkHeader += " *  - ImGui_ImplVulkan_SetupRenderState:"
$gearsVkHeader += " *    - Remove unecessary descriptor set binding:"
$gearsVkHeader += " *        VkDescriptorSet desc_set[1] = { bd->DescriptorSet };"
$gearsVkHeader += " *        vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, NULL);"
$gearsVkHeader += " *  - ImGui_ImplVulkan_RenderDrawData:"
$gearsVkHeader += " *    - Add descriptor set binding before vkCmdDrawIndexed:"
$gearsVkHeader += " *	    VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->GetTexID() };"
$gearsVkHeader += " *	    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, NULL);"
$gearsVkHeader += " *  - ImGui_ImplVulkan_CreateFontsTexture:"
$gearsVkHeader += " *    - Change 'io.Fonts->SetTexID((ImTextureID)(intptr_t)bd->FontImage)' to 'io.Fonts->SetTexID((ImTextureID)bd->DescriptorSet)'"
$gearsVkHeader += " */"
$gearsVkHeader += ""

##
# Modify imgui vulkan impl header file (imgui_impl_vulkan.h)
##
"-> Modifying " + $imgui_impl_vulkan_h + ":"

# Read first line of imgui_impl_vulkan.h
$fileContent = Get-Content $imgui_impl_vulkan_h -First 1

# Skip file if it already contains our header, else add our header at the beginning of the file
if ($fileContent -eq $gearsVkHeader[0]) {
	Write-Host "   [X] Already modified" -ForegroundColor Green
} else {
	# Read the whole content of imgui_impl_vulkan.h
	$fileContent = Get-Content $imgui_impl_vulkan_h

	# Create an empty array and use it as the modified file
	$fileContentModified = @()
	# Add Gears-Vk header
	$fileContentModified += $gearsVkHeader
	# Add original filecontent
	$fileContentModified += $fileContent

	# Overwrite original imgui_impl_vulkan.h file with modified content
	Set-Content $imgui_impl_vulkan_h $fileContentModified
	Write-Host "   [X] Added Gears-Vk header" -ForegroundColor Green
}


##
# Modify imgui vulkan impl source file (imgui_impl_vulkan.cpp)
#
"-> Modifying " + $imgui_impl_vulkan_cpp + ":"

# Read imgui vulkan impl source
$fileContent = Get-Content $imgui_impl_vulkan_cpp

# Functions that will be modified
enum FunctionName {
	SetupRenderState;
	RenderDrawData;
	CreateFontsTexture;
	Undefined
}
$currentFunction = [FunctionName]::Undefined
$aboutToEnterFunction = [FunctionName]::Undefined

# For tracking changes that were performed
$appliedChanges = @{
	[FunctionName]::SetupRenderState = 0
	[FunctionName]::RenderDrawData = 0
	[FunctionName]::CreateFontsTexture = 0
}

# Create an empty array and use it as the modified file
$fileContentModified = @()

# Header for the beginning of files to communicate the modifications and prevent multiple executions of this script
$fileContentModified += $gearsVkHeader

$fileAlreadyModified = 0
$parentheseCounter = 0
Foreach ($line in $fileContent)
{
	# Skip file if it already contains our header
	if ($line -eq $gearsVkHeader[0]) {
		Write-Host "   [X] Already modified" -ForegroundColor Green
		$fileAlreadyModified = 1
		break
	}

	# Detect if about to enter ImGui_ImplVulkan_SetupRenderState
	if ($line -imatch "void\s+ImGui_ImplVulkan_SetupRenderState\(.+\)") {
		$aboutToEnterFunction = [FunctionName]::SetupRenderState
	}

	# Detect if about to enter ImGui_ImplVulkan_RenderDrawData
	if ($line -imatch "void\s+ImGui_ImplVulkan_RenderDrawData\(.+\)") {
		$aboutToEnterFunction = [FunctionName]::RenderDrawData
	}

	# Detect if about to enter ImGui_ImplVulkan_CreateFontsTexture
	if ($line -imatch "bool\s+ImGui_ImplVulkan_CreateFontsTexture\(.+\)") {
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
		if ($line -imatch "\/\/\s*Bind\s*pipeline\s*and\s*descriptor\s*sets") {
			# Get tabs/spaces up to the first '/' to nicely align the new comment
			$matched = $line -imatch "^[^/]*"
			$tabs = $Matches[0]
			$fileContentModified += $tabs + "// Bind pipeline"
			continue
		}
		# Remove unecessary descriptor binding
		if ($line -imatch "VkDescriptorSet\s*desc_set\[1\]\s*=\s*\{\s*bd->DescriptorSet\s*\};") {
			continue
		}
		if ($line -imatch "vkCmdBindDescriptorSets\(.+,\s*VK_PIPELINE_BIND_POINT_GRAPHICS\s*,.+,\s*0\s*,\s*1\s*,\s*desc_set\s*,\s*0\s*,.+\);") {
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
		if ($line -imatch "vkCmdSetScissor\(.+,\s*0\s*,\s*1\s*,.+\);") {
			# Add the scissor cmd line
			$fileContentModified += $line
			# Get tabs/spaces up to the 'v' of vkCmdSetScissor to nicely align the code
			$matched = $line -imatch "^[^v]*"
			$tabs = $Matches[0]
			# Add descriptor binding code after the vkCmdSetScissor cmd
			$fileContentModified += ''
			$fileContentModified += $tabs + '// Bind texture descriptor set stored as ImTextureID'
			$fileContentModified += $tabs + 'VkDescriptorSet desc_set[1] = { (VkDescriptorSet) pcmd->GetTexID() };'
			$fileContentModified += $tabs + 'vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, bd->PipelineLayout, 0, 1, desc_set, 0, NULL);'
			$appliedChanges[[FunctionName]::RenderDrawData] = 1
			continue
		}
	}

	# Change fonts texture id assignment to descriptor in ImGui_ImplVulkan_CreateFontsTexture
	if ($currentFunction -eq [FunctionName]::CreateFontsTexture) {
		if ($line -imatch "io.Fonts->SetTexID\(.+\);") {
			# Get tabs/spaces up to the 'i' of io.Fonts to nicely align the code
			$matched = $line -imatch "^[^i]*"
			$tabs = $Matches[0]
			# Asign descriptor set as ImTextureID
			$fileContentModified += $tabs + 'io.Fonts->SetTexID((ImTextureID)bd->DescriptorSet);'
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
