// DashWallRunCombo.h
#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/ComboAbilityComponent.h"
#include "DashWallRunCombo.generated.h"

/**
 * Dash + Wall Run combo - A movement combo that rewards agile play
 * When player transitions from dash into wall run, they gain WP
 * This encourages fluid movement through the level
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UDashWallRunCombo : public UComboAbilityComponent
{
    GENERATED_BODY()

public:
    UDashWallRunCombo();

protected:
    // Override combo execution - required by base class
    virtual void ExecuteCombo() override;
    
    // This combo doesn't need special execution - it's detected automatically
    // The WP reward is handled when the combo is detected
    
private:
    // Movement boost during wall run after dash
    UPROPERTY(EditAnywhere, Category = "Combo Effects", meta = (ClampMin = "1.0", ClampMax = "2.0"))
    float WallRunSpeedMultiplier = 1.2f;
    
    // Duration of the speed boost
    UPROPERTY(EditAnywhere, Category = "Combo Effects", meta = (ClampMin = "1.0", ClampMax = "10.0"))
    float SpeedBoostDuration = 3.0f;
};