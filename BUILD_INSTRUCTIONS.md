# Build Instructions for Blackhole Project

## Prerequisites
- Unreal Engine 5.5 installed at `D:\UnrealEngine\UE_5.5`
- Visual Studio 2022 with C++ development tools
- Windows SDK

## Build Commands

### From Windows Command Prompt
```cmd
cd "D:\Unreal Projects\blackhole"
"D:\UnrealEngine\UE_5.5\Engine\Build\BatchFiles\Build.bat" blackholeEditor Win64 Development -waitmutex
```

### From PowerShell
```powershell
cd "D:\Unreal Projects\blackhole"
& "D:\UnrealEngine\UE_5.5\Engine\Build\BatchFiles\Build.bat" blackholeEditor Win64 Development -waitmutex
```

### Clean Build
```cmd
"D:\UnrealEngine\UE_5.5\Engine\Build\BatchFiles\Clean.bat" blackholeEditor Win64 Development
"D:\UnrealEngine\UE_5.5\Engine\Build\BatchFiles\Build.bat" blackholeEditor Win64 Development -waitmutex
```

### Generate Project Files
```cmd
"D:\UnrealEngine\UE_5.5\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "D:\Unreal Projects\blackhole\blackhole.uproject" -game
```

## Common Build Errors

### "System cannot find the path specified"
- Ensure Unreal Engine is installed at the correct path
- Check that blackhole.uproject exists in the project directory

### Access Violations During Build
- Close Unreal Editor before building
- Clean and rebuild if errors persist

### Include Errors
- Run "Generate Project Files" to update IntelliSense

## Testing After Build
1. Open blackhole.uproject in Unreal Editor
2. Press Play to test in PIE (Play In Editor)
3. Check Output Log for any runtime errors
4. Test all abilities to ensure crash fixes are working

## Hot Reload
While the editor is open, you can compile changes with:
- Ctrl+Alt+F11 in Visual Studio
- Or click "Compile" button in Unreal Editor