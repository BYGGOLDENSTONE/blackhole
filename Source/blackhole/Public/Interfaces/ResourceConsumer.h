#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ResourceConsumer.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UResourceConsumer : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for objects that consume resources (willpower, etc.)
 * Provides a clean abstraction for resource checks and consumption
 * Note: Stamina has been removed from the game, but parameters kept for interface compatibility
 */
class BLACKHOLE_API IResourceConsumer
{
    GENERATED_BODY()

public:
    /**
     * Check if the consumer has enough resources for an action
     * @param StaminaCost DEPRECATED - No longer used, kept for interface compatibility
     * @param WPCost Required willpower amount (can be negative for WP reduction)
     * @return True if resources are available
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
    bool HasResources(float StaminaCost, float WPCost) const;

    /**
     * Consume resources for an action
     * @param StaminaCost DEPRECATED - No longer used, kept for interface compatibility
     * @param WPCost Willpower to add (positive) or reduce (negative)
     * @return True if resources were successfully consumed
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
    bool ConsumeResources(float StaminaCost, float WPCost);

    /**
     * Get current resource percentages for UI display
     * @param OutStaminaPercent DEPRECATED - Always returns 0, kept for interface compatibility
     * @param OutWPPercent Current willpower percentage (0-1)
     */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Resources")
    void GetResourcePercentages(float& OutStaminaPercent, float& OutWPPercent) const;
};