$exeName = "cgb_post_build_helper.exe"
$postBuildExePath = Join-Path -Path $PSScriptRoot -ChildPath $exeName

# Check if the tool is available:
if (-Not (Test-Path $postBuildExePath))
{
	if ($args[0] -eq "-msbuild" -and $args.Length -gt 2) 
	{
		try 
		{
			$msbuildPath = Join-Path -Path $args[1] -ChildPath "MSBuild.exe" 
			$postBuildSlnPath = Join-Path -Path $PSScriptRoot -ChildPath "../sources/cgb_post_build_helper/cgb_post_build_helper.sln"
			Write-Output "$exeName not found => going to build it from $postBuildSlnPath ..."
			Start-Process -Wait -WindowStyle Hidden -FilePath $msbuildPath -ArgumentList $postBuildSlnPath, '/property:Configuration=Release'
		}
		catch 
		{
			Write-Output "Building post build helper failed."
			# $_
		}
	}
	if (-Not (Test-Path $postBuildExePath))
	{
		try 
		{
			$srcExe = Join-Path -Path $PSScriptRoot -ChildPath "fallback_cgb_post_build_helper.exe"
			Write-Output "Building $exeName failed => going to copy it from $srcExe ..."
			Copy-Item $srcExe -Destination $postBuildExePath
		}
		catch 
		{
			Write-Output "Copying fallback_cgb_post_build_helper.exe -> $exeName failed."
			# $_
		}
	}
}

# Print args to console:
#$outPath = Join-Path -Path $PSScriptRoot -ChildPath "args.txt"
#Write-Output $outPath
#$args | Out-File -FilePath $outPath

# Invoke the post build helper tool:
Start-Process -FilePath $postBuildExePath -ArgumentList $args -WorkingDirectory $PSScriptRoot

$maxLoop = 10
for ($i=1; $i -le $maxLoop; $i++)
{
	$pipe = new-object System.IO.Pipes.NamedPipeClientStream '.','Gears-Vk Application Status Pipe','In'
	$pipe.Connect()
	$sr = new-object System.IO.StreamReader $pipe
	$messageReceived = $sr.ReadLine();
	$sr.Dispose()
	$pipe.Dispose()

	if ([string]::IsNullOrEmpty($messageReceived)) 
	{
		break
	}

	Write-Output $messageReceived
	Start-Sleep -s 1

	if ($i -eq $maxLoop) 
	{
		Write-Output "The post build helper is still busy, but we're not going to wait any longer."
	}
}
