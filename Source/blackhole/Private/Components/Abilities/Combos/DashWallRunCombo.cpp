// DashWallRunCombo.cpp
#include "Components/Abilities/Combos/DashWallRunCombo.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Movement/WallRunComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Systems/ResourceManager.h"

UDashWallRunCombo::UDashWallRunCombo()
{
    // Basic movement combo - not considered an aggressive attack
    bIsBasicAbility = true;
    
    // No resource costs - this is a movement combo
    WPCost = 0.0f;
    
    // WP reward for successful execution (inherited from ComboAbilityComponent)
    WPRewardAmount = 10.0f; // Match the existing reward in WallRunComponent
    
    // Timing window for combo detection
    ComboWindowTime = 1.5f;
    
    // Visual feedback settings
    ComboTrailColor = FLinearColor(0.2f, 0.8f, 1.0f, 1.0f); // Light blue for movement combo
    bShowDebugVisuals = false;
}

void UDashWallRunCombo::ExecuteCombo()
{
    // Cache owner character from base class
    if (!OwnerCharacter)
    {
        OwnerCharacter = Cast<ABlackholePlayerCharacter>(GetOwner());
    }
    
    if (!OwnerCharacter)
    {
        return;
    }
    
    // Apply speed boost to wall run
    if (UWallRunComponent* WallRunComp = OwnerCharacter->GetWallRunComponent())
    {
        // This would require adding a speed multiplier method to WallRunComponent
        // For now, the combo detection and WP reward is the main feature
        UE_LOG(LogTemp, Warning, TEXT("Dash + Wall Run combo executed successfully!"));
    }
    
    // Grant WP reward
    if (UGameInstance* GameInstance = OwnerCharacter->GetGameInstance())
    {
        if (UResourceManager* ResourceMgr = GameInstance->GetSubsystem<UResourceManager>())
        {
            ResourceMgr->AddWillPower(WPRewardAmount);
            UE_LOG(LogTemp, Warning, TEXT("Dash + Wall Run combo: Granted %f WP"), WPRewardAmount);
        }
    }
    
    // Play combo feedback (visual and audio)
    PlayComboFeedback(OwnerCharacter->GetActorLocation());
    
    // Show debug visuals if enabled
    if (bShowDebugVisuals)
    {
        DrawComboVisuals(OwnerCharacter->GetActorLocation(), 
                        OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * 200.0f);
    }
}