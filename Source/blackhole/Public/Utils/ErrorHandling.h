#pragma once

#include "CoreMinimal.h"

/**
 * Comprehensive error handling macros for the Blackhole project.
 * These provide consistent error checking and logging throughout the codebase.
 */

// Check if a pointer is valid and log error if not
#define CHECK_VALID(Ptr, ReturnValue) \
	if (!IsValid(Ptr)) \
	{ \
		UE_LOG(LogTemp, Error, TEXT("%s: Invalid pointer %s at %s:%d"), \
			*FString(__FUNCTION__), TEXT(#Ptr), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

// Check if a pointer is valid (for void functions)
#define CHECK_VALID_VOID(Ptr) \
	if (!IsValid(Ptr)) \
	{ \
		UE_LOG(LogTemp, Error, TEXT("%s: Invalid pointer %s at %s:%d"), \
			*FString(__FUNCTION__), TEXT(#Ptr), TEXT(__FILE__), __LINE__); \
		return; \
	}

// Check if a cast succeeded and log error if not
#define CHECK_CAST(CastResult, ReturnValue) \
	if (!(CastResult)) \
	{ \
		UE_LOG(LogTemp, Error, TEXT("%s: Cast failed for %s at %s:%d"), \
			*FString(__FUNCTION__), TEXT(#CastResult), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

// Check if a cast succeeded (for void functions)
#define CHECK_CAST_VOID(CastResult) \
	if (!(CastResult)) \
	{ \
		UE_LOG(LogTemp, Error, TEXT("%s: Cast failed for %s at %s:%d"), \
			*FString(__FUNCTION__), TEXT(#CastResult), TEXT(__FILE__), __LINE__); \
		return; \
	}

// Check array bounds
#define CHECK_ARRAY_BOUNDS(Array, Index, ReturnValue) \
	if ((Index) < 0 || (Index) >= (Array).Num()) \
	{ \
		UE_LOG(LogTemp, Error, TEXT("%s: Array index out of bounds %s[%d] (Size: %d) at %s:%d"), \
			*FString(__FUNCTION__), TEXT(#Array), (Index), (Array).Num(), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

// Check if a component exists
#define CHECK_COMPONENT(Component, ReturnValue) \
	if (!(Component)) \
	{ \
		UE_LOG(LogTemp, Warning, TEXT("%s: Component %s is null at %s:%d"), \
			*FString(__FUNCTION__), TEXT(#Component), TEXT(__FILE__), __LINE__); \
		return ReturnValue; \
	}

// Log and return on condition
#define LOG_RETURN_IF(Condition, LogLevel, Message, ReturnValue) \
	if (Condition) \
	{ \
		UE_LOG(LogTemp, LogLevel, TEXT("%s: %s"), *FString(__FUNCTION__), Message); \
		return ReturnValue; \
	}

// Validate range parameters
#define VALIDATE_POSITIVE(Value, ParamName) \
	if ((Value) <= 0.0f) \
	{ \
		UE_LOG(LogTemp, Warning, TEXT("%s: Invalid %s value: %.2f"), \
			*FString(__FUNCTION__), TEXT(ParamName), (Value)); \
		return false; \
	}

// Safe array access with default
template<typename T>
T SafeArrayAccess(const TArray<T>& Array, int32 Index, const T& DefaultValue)
{
	if (Index >= 0 && Index < Array.Num())
	{
		return Array[Index];
	}
	
	UE_LOG(LogTemp, Warning, TEXT("SafeArrayAccess: Index %d out of bounds (Array size: %d)"), 
		Index, Array.Num());
	return DefaultValue;
}

// Safe division to prevent divide by zero
FORCEINLINE float SafeDivide(float Numerator, float Denominator, float DefaultValue = 0.0f)
{
	if (FMath::IsNearlyZero(Denominator))
	{
		UE_LOG(LogTemp, Warning, TEXT("SafeDivide: Division by zero prevented"));
		return DefaultValue;
	}
	return Numerator / Denominator;
}

// Validate actor before operations
FORCEINLINE bool IsActorValidForOperation(const AActor* Actor)
{
	return IsValid(Actor) && !Actor->IsPendingKillPending() && Actor->GetWorld() != nullptr;
}

// Log execution context for debugging
#define LOG_EXECUTION_CONTEXT() \
	UE_LOG(LogTemp, VeryVerbose, TEXT("%s called from %s:%d"), \
		*FString(__FUNCTION__), TEXT(__FILE__), __LINE__)

// Assert with custom message in debug builds
#if !UE_BUILD_SHIPPING
	#define BLACKHOLE_ASSERT(Condition, Message) \
		if (!(Condition)) \
		{ \
			UE_LOG(LogTemp, Fatal, TEXT("ASSERTION FAILED: %s at %s:%d - %s"), \
				TEXT(#Condition), TEXT(__FILE__), __LINE__, Message); \
		}
#else
	#define BLACKHOLE_ASSERT(Condition, Message)
#endif