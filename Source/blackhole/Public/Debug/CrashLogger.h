#pragma once

#include "CoreMinimal.h"

/**
 * Crash detection and logging system to help identify crash locations
 * Usage: Add CRASH_CHECKPOINT() at key points in your code
 */

class BLACKHOLE_API FCrashLogger
{
public:
	static FCrashLogger& Get();
	
	void LogCheckpoint(const FString& Function, const FString& File, int32 Line, const FString& Context = "");
	void LogError(const FString& Function, const FString& File, int32 Line, const FString& Error);
	void ClearLog();
	
private:
	FCrashLogger();
	~FCrashLogger();
	
	static FCrashLogger* Instance;
};

// Macro for easy checkpoint logging
#if !UE_BUILD_SHIPPING
	#define CRASH_CHECKPOINT() \
		FCrashLogger::Get().LogCheckpoint(TEXT(__FUNCTION__), TEXT(__FILE__), __LINE__)
		
	#define CRASH_CHECKPOINT_MSG(Context) \
		FCrashLogger::Get().LogCheckpoint(TEXT(__FUNCTION__), TEXT(__FILE__), __LINE__, Context)
		
	#define CRASH_LOG_ERROR(Error) \
		FCrashLogger::Get().LogError(TEXT(__FUNCTION__), TEXT(__FILE__), __LINE__, Error)
#else
	#define CRASH_CHECKPOINT()
	#define CRASH_CHECKPOINT_MSG(Context)
	#define CRASH_LOG_ERROR(Error)
#endif

// Macro for critical operations
#define SAFE_OPERATION(Operation, FailureLog) \
	CRASH_CHECKPOINT_MSG(TEXT("Before: ") TEXT(#Operation)); \
	Operation; \
	CRASH_CHECKPOINT_MSG(TEXT("After: ") TEXT(#Operation));

// Enhanced validation macros with crash tracking
#define VALIDATE_PTR(Ptr, Context) \
	CRASH_CHECKPOINT_MSG(FString::Printf(TEXT("Validating %s: %p"), TEXT(#Ptr), (void*)(Ptr))); \
	if (!IsValid(Ptr)) \
	{ \
		CRASH_LOG_ERROR(FString::Printf(TEXT("Invalid pointer %s (%s)"), TEXT(#Ptr), Context)); \
		return; \
	}

#define VALIDATE_PTR_RET(Ptr, Context, RetVal) \
	CRASH_CHECKPOINT_MSG(FString::Printf(TEXT("Validating %s: %p"), TEXT(#Ptr), (void*)(Ptr))); \
	if (!IsValid(Ptr)) \
	{ \
		CRASH_LOG_ERROR(FString::Printf(TEXT("Invalid pointer %s (%s)"), TEXT(#Ptr), Context)); \
		return RetVal; \
	}

// Safe pointer access
#define SAFE_ACCESS(Ptr, Member) \
	(IsValid(Ptr) ? (Ptr)->Member : (CRASH_LOG_ERROR(FString::Printf(TEXT("Null pointer access: %s"), TEXT(#Ptr))), nullptr))