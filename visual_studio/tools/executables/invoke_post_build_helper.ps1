$outPath = Join-Path -Path $PSScriptRoot -ChildPath "args.txt"
#Write-Output $outPath
#$args | Out-File -FilePath $outPath
$postBuildHelperPath = Join-Path -Path $PSScriptRoot -ChildPath "cgb_post_build_helper.exe"
$result = Start-Process -FilePath $postBuildHelperPath -ArgumentList $args -WorkingDirectory $PSScriptRoot
$result | Out-File -FilePath $outPath