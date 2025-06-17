msbuild /m /p:Configuration=Release /p:platform=x64 dmidecode.sln
msbuild /m /p:Configuration=Release /p:platform=x86 dmidecode.sln

$executables = "dmidecode.exe", "biosdecode.exe", "ownership.exe", "vpddecode.exe"
$platformMap = @{
	"x64/Release"   = "build/x64"
	"Win32/Release" = "build/x86"
}
$platformMap.Values | ForEach-Object { New-Item -Path $_ -ItemType Directory -Force }
foreach ($entry in $platformMap.GetEnumerator()) {
	foreach ($exe in $executables) {
		Move-Item -Path "$($entry.Name)/$exe" -Destination "$($entry.Value)/"
	}
}
Copy-Item -Path "LICENSE" -Destination "build/LICENSE"

Compress-Archive -Path build/* -DestinationPath "dmidecode.zip"
