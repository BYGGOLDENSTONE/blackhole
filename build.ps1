# Build script for Blackhole UE5.5 project with Rider
# This script builds the project using UnrealBuildTool

param(
    [ValidateSet("Debug", "Development", "Shipping")]
    [string]$Configuration = "Development",
    
    [ValidateSet("Editor", "Game")]
    [string]$Target = "Editor",
    
    [switch]$Clean,
    [switch]$Help
)

# Show help if requested
if ($Help) {
    Write-Host @"

Usage: build.ps1 [options]

Options:
  -Configuration   Build configuration (Debug, Development, Shipping) [Default: Development]
  -Target         Build target (Editor, Game) [Default: Editor]
  -Clean          Clean before building
  -Help           Show this help message

Examples:
  .\build.ps1                                    # Build Development Editor
  .\build.ps1 -Configuration Debug              # Build Debug Editor
  .\build.ps1 -Configuration Shipping -Target Game  # Build Shipping Game
  .\build.ps1 -Clean -Configuration Development    # Clean and build Development Editor

"@
    exit 0
}

Write-Host "========================================"
Write-Host "Building Blackhole Project (UE5.5)"
Write-Host "========================================"

# Set UE5.5 installation path
$UEPath = "D:\UE_5.5"

# Set project variables
$ProjectName = "blackhole"
$ProjectPath = "D:\Unreal Projects\blackhole"
$UProjectFile = "$ProjectPath\$ProjectName.uproject"

# Check if UE5.5 exists
if (-not (Test-Path $UEPath)) {
    Write-Error "Unreal Engine 5.5 not found at $UEPath"
    Write-Host "Please update the UEPath variable in this script"
    exit 1
}

# Check if project file exists
if (-not (Test-Path $UProjectFile)) {
    Write-Error "Project file not found at $UProjectFile"
    exit 1
}

# Set build target name
$TargetName = if ($Target -eq "Editor") { "${ProjectName}Editor" } else { $ProjectName }

# Set UnrealBuildTool path
$UBTPath = "$UEPath\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

# Check if UnrealBuildTool exists
if (-not (Test-Path $UBTPath)) {
    Write-Error "UnrealBuildTool not found at $UBTPath"
    exit 1
}

# Clean if requested
if ($Clean) {
    Write-Host ""
    Write-Host "Cleaning previous build..."
    & "$UBTPath" -Target="$TargetName Win64 $Configuration" -Project="$UProjectFile" -Clean
}

# Build the project
Write-Host ""
Write-Host "Building $TargetName - $Configuration configuration..."
Write-Host ""

& "$UBTPath" -Target="$TargetName Win64 $Configuration" -Project="$UProjectFile" -Progress -NoHotReloadFromIDE

# Check build result
if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "BUILD FAILED!" -ForegroundColor Red
    Write-Host "========================================"
    exit $LASTEXITCODE
} else {
    Write-Host ""
    Write-Host "========================================"
    Write-Host "BUILD SUCCESSFUL!" -ForegroundColor Green
    Write-Host "========================================"
    
    # Show output location
    Write-Host ""
    Write-Host "Build output location:"
    Write-Host "$ProjectPath\Binaries\Win64\"
    
    # If this was an Editor build, remind about hot reload
    if ($Target -eq "Editor") {
        Write-Host ""
        Write-Host "Note: If the editor is running, use Hot Reload (Ctrl+Alt+F11)"
        Write-Host "instead of restarting the editor."
    }
}

exit 0