#include "Debug/CrashLogger.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Engine/Engine.h"

FCrashLogger* FCrashLogger::Instance = nullptr;

FCrashLogger::FCrashLogger()
{
	// Initialize crash logger
	ClearLog();
	UE_LOG(LogTemp, Warning, TEXT("CrashLogger initialized"));
}

FCrashLogger::~FCrashLogger()
{
	// Cleanup
}

FCrashLogger& FCrashLogger::Get()
{
	if (!Instance)
	{
		Instance = new FCrashLogger();
	}
	return *Instance;
}

void FCrashLogger::LogCheckpoint(const FString& Function, const FString& File, int32 Line, const FString& Context)
{
	FString LogEntry = FString::Printf(TEXT("[%s] %s:%d - %s %s\n"), 
		*FDateTime::Now().ToString(), 
		*Function, 
		Line,
		*File,
		Context.IsEmpty() ? TEXT("") : *FString::Printf(TEXT("Context: %s"), *Context)
	);
	
	// Log to console with high verbosity
	UE_LOG(LogTemp, Warning, TEXT("CHECKPOINT: %s"), *LogEntry);
	
	// Also log to screen in development builds
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan, FString::Printf(TEXT("CP: %s:%d"), *Function, Line));
	}
#endif
	
	// Write to crash log file
#if !UE_BUILD_SHIPPING
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CrashCheckpoints.log");
	
	// Append to file
	FFileHelper::SaveStringToFile(LogEntry, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
#endif
}

void FCrashLogger::LogError(const FString& Function, const FString& File, int32 Line, const FString& Error)
{
	FString LogEntry = FString::Printf(TEXT("[ERROR] [%s] %s:%d - %s - %s\n"), 
		*FDateTime::Now().ToString(), 
		*Function, 
		Line,
		*File,
		*Error
	);
	
	// Log error with high priority
	UE_LOG(LogTemp, Error, TEXT("CRASH_LOGGER: %s"), *LogEntry);
	
	// Also log to screen in development builds
#if !UE_BUILD_SHIPPING
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 10.0f, FColor::Red, FString::Printf(TEXT("ERROR: %s - %s"), *Function, *Error));
	}
#endif
	
	// Write to crash log file
#if !UE_BUILD_SHIPPING
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CrashCheckpoints.log");
	FFileHelper::SaveStringToFile(LogEntry, *FilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
#endif
}

void FCrashLogger::ClearLog()
{
#if !UE_BUILD_SHIPPING
	FString FilePath = FPaths::ProjectSavedDir() / TEXT("CrashCheckpoints.log");
	FString Header = FString::Printf(TEXT("=== Crash Logger Started: %s ===\n"), *FDateTime::Now().ToString());
	FFileHelper::SaveStringToFile(Header, *FilePath);
#endif
}