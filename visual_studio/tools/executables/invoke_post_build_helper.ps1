$exeName = "post_build_helper.exe"
$postBuildExePath = Join-Path -Path $PSScriptRoot -ChildPath $exeName
$dlls = @("AssimpNet.dll", "Newtonsoft.Json.dll", "EnvDTE.dll", "EnvDTE80.dll", "Hardcodet.Wpf.TaskbarNotification.dll")

$anyDllMissing = $false
foreach ($dll in $dlls) {
	$dllPath = Join-Path -Path $PSScriptRoot -ChildPath $dll
	$anyDllMissing = $anyDllMissing -or (-Not (Test-Path $dllPath));
}

# Check if the tool is available:
if ((-Not (Test-Path $postBuildExePath)) -or $anyDllMissing)
{
	if ($args[0].ToLower() -eq "-msbuild" -and $args.Length -gt 2) 
	{
		try 
		{
			$msbuildPath = Join-Path -Path $args[1] -ChildPath "MSBuild.exe" 
			$nugetPath = Join-Path -Path $PSScriptRoot -ChildPath "nuget.exe"
			$postBuildSlnPath = Join-Path -Path $PSScriptRoot -ChildPath "..\sources\post_build_helper\post_build_helper.sln"
			Write-Output "$exeName or one of its required DLLs not found => going to build Post Build Helper from $postBuildSlnPath ..."
			Write-Output ""
			Write-Output "Note 1: If the build takes too long (more than 10 sec.), please cancel it (Build -> Cancel) and build again."
			Write-Output "Note 2: If the build fails, please open '$postBuildSlnPath' and build the C# project manually (just select the 'Release' configuration and build)."
			Write-Output ""
			Write-Output "First, restoring NuGet packages ... (Please allow nuget.exe to run if prompted)"
			Start-Process -Wait -WindowStyle Hidden -FilePath $nugetPath -ArgumentList "restore", $postBuildSlnPath
			Write-Output "Going to invoke: ""$msbuildPath"" ""$postBuildSlnPath"" /property:Configuration=Release"
			Start-Process -Wait -WindowStyle Hidden -FilePath $msbuildPath -ArgumentList $postBuildSlnPath, '/property:Configuration=Release'
		}
		catch 
		{
			Write-Output "Building the Post Build Helper caused an exception."
			$_
		}
	}
	else 
	{
		Write-Output "Incomplete parameters or wrong order of parameters! The '-msbuild' parameter followed by the path to MSBuild.exe must be passed first!"
	}

	$anyDllMissingFallback = $false
	foreach ($dll in $dlls) {
		$dllPath = Join-Path -Path $PSScriptRoot -ChildPath $dll
		$anyDllMissingFallback = $anyDllMissingFallback -or (-Not (Test-Path $dllPath));
	}

	if ((-Not (Test-Path $postBuildExePath)) -or $anyDllMissingFallback)
	{
		try 
		{
			$srcExe = Join-Path -Path $PSScriptRoot -ChildPath "fallback_post_build_helper.exe"
			Write-Output "Building $exeName failed => going to copy it from $srcExe ..."
			Copy-Item $srcExe -Destination $postBuildExePath

			foreach ($dll in $dlls) {
				$srcDll = Join-Path -Path $PSScriptRoot -ChildPath "fallback.$dll"
				$dstDll = Join-Path -Path $PSScriptRoot -ChildPath "$dll"
				Write-Output "Also going to copy $dll from '$srcDll' to '$dstDll' ..."
				Copy-Item $srcDll -Destination $dstDll
			}
		}
		catch 
		{
			Write-Output "Copying the files caused an exception"
			$_
		}
	}
}

# Put quotes around every arg:
$quotedArgs = '"{0}"' -f ($args -join '" "')
# Account for trailing backslashes in arguments (which would escape the quote, if unhandled):
$quotedArgs = $quotedArgs.Replace("\""", "\\""")

# Print args to console:
# $outPath = Join-Path -Path $PSScriptRoot -ChildPath "args.txt"
# Write-Output $outPath
# $args | Out-File -FilePath $outPath
# $quotedArgs | Out-File -FilePath $outPath

# Invoke the Post Build Helper tool:
Start-Process -FilePath $postBuildExePath -ArgumentList $quotedArgs -WorkingDirectory $PSScriptRoot

Write-Output "Post Build Helper has been invoked. Now waiting for the deployment of assets and shaders..."
$maxLoop = 10
$readSomethingAtLeastOnce = $false;
for ($i=0; $i -le $maxLoop; $i++)
{
	try 
	{
		$pipe = new-object System.IO.Pipes.NamedPipeClientStream '.','Gears-Vk Application Status Pipe','In'
		$pipe.Connect()
		$sr = new-object System.IO.StreamReader $pipe
		$messageReceived = $sr.ReadLine();
		$sr.Dispose()
		$pipe.Dispose()
	}
	catch 
	{
		Write-Output "Failed to connect to Post Build Helper"
		$_
	}

	# Wait until the Post Build Helper was active at least once (indicated by a 
	# message that actually contained some status), but don't wait longer than
	# a maximum of 5 seconds for the first actual status message to arrive.
	$readSomethingAtLeastOnce = $readSomethingAtLeastOnce -or ($null -ne $messageReceived -and $messageReceived.Length -gt 0) -or $i -ge 5
	if ($readSomethingAtLeastOnce -and $null -ne $messageReceived -and "" -eq $messageReceived)
	{
		break
	}

	Write-Output "After $i sec.: $messageReceived"
	Start-Sleep -s 1

	if ($i -eq $maxLoop) 
	{
		Write-Output "The Post Build Helper is still busy, but we're not going to wait any longer."
	}
}
Write-Output ""
