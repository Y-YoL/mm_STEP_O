
$data =  Get-Content .\libogg_static.vcxproj |
	%{ $_ -replace "MultiThreadedDebugDLL", "MultiThreadedDebug" } |
	%{ $_ -replace "MultiThreadedDLL", "MultiThreaded" }

$data | Out-File .\libogg_static.vcxproj
