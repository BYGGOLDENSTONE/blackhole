@echo off
setlocal enabledelayedexpansion

:: Build script for Blackhole UE5.5 project with Rider
:: This script builds the project using UnrealBuildTool

echo ========================================
echo Building Blackhole Project (UE5.5)
echo ========================================

:: Set UE5.5 installation path
set UE_PATH=D:\UE_5.5

:: Set project variables
set PROJECT_NAME=blackhole
set PROJECT_PATH=D:\Unreal Projects\blackhole
set UPROJECT_FILE=%PROJECT_PATH%\%PROJECT_NAME%.uproject

:: Check if UE5.5 exists
if not exist "%UE_PATH%" (
    echo ERROR: Unreal Engine 5.5 not found at %UE_PATH%
    echo Please update the UE_PATH variable in this script
    exit /b 1
)

:: Check if project file exists
if not exist "%UPROJECT_FILE%" (
    echo ERROR: Project file not found at %UPROJECT_FILE%
    exit /b 1
)

:: Parse command line arguments
set BUILD_CONFIG=Development
set BUILD_TARGET=Editor
set CLEAN_BUILD=0

:parse_args
if "%1"=="" goto :done_parsing
if /i "%1"=="debug" set BUILD_CONFIG=Debug
if /i "%1"=="development" set BUILD_CONFIG=Development
if /i "%1"=="shipping" set BUILD_CONFIG=Shipping
if /i "%1"=="game" set BUILD_TARGET=
if /i "%1"=="editor" set BUILD_TARGET=Editor
if /i "%1"=="clean" set CLEAN_BUILD=1
if /i "%1"=="help" goto :show_help
shift
goto :parse_args
:done_parsing

:: Set build target name
if "%BUILD_TARGET%"=="Editor" (
    set TARGET_NAME=%PROJECT_NAME%Editor
) else (
    set TARGET_NAME=%PROJECT_NAME%
)

:: Set UnrealBuildTool path
set UBT_PATH=%UE_PATH%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe

:: Check if UnrealBuildTool exists
if not exist "%UBT_PATH%" (
    echo ERROR: UnrealBuildTool not found at %UBT_PATH%
    exit /b 1
)

:: Clean if requested
if %CLEAN_BUILD%==1 (
    echo.
    echo Cleaning previous build...
    "%UBT_PATH%" -Target="%TARGET_NAME% Win64 %BUILD_CONFIG%" -Project="%UPROJECT_FILE%" -Clean
)

:: Build the project
echo.
echo Building %TARGET_NAME% - %BUILD_CONFIG% configuration...
echo.

"%UBT_PATH%" -Target="%TARGET_NAME% Win64 %BUILD_CONFIG%" -Project="%UPROJECT_FILE%" -Progress -NoHotReloadFromIDE

:: Check build result
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo BUILD FAILED!
    echo ========================================
    exit /b %ERRORLEVEL%
) else (
    echo.
    echo ========================================
    echo BUILD SUCCESSFUL!
    echo ========================================
    
    :: Show output location
    echo.
    echo Build output location:
    echo %PROJECT_PATH%\Binaries\Win64\
    
    :: If this was an Editor build, remind about hot reload
    if "%BUILD_TARGET%"=="Editor" (
        echo.
        echo Note: If the editor is running, use Hot Reload (Ctrl+Alt+F11)
        echo instead of restarting the editor.
    )
)

exit /b 0

:show_help
echo.
echo Usage: build.bat [options]
echo.
echo Options:
echo   debug         - Build Debug configuration
echo   development   - Build Development configuration (default)
echo   shipping      - Build Shipping configuration
echo   editor        - Build Editor target (default)
echo   game          - Build Game target
echo   clean         - Clean before building
echo   help          - Show this help message
echo.
echo Examples:
echo   build.bat                    - Build Development Editor
echo   build.bat debug              - Build Debug Editor
echo   build.bat shipping game      - Build Shipping Game
echo   build.bat clean development  - Clean and build Development Editor
echo.
exit /b 0