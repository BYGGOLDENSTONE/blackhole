#include "Components/Abilities/ComboAbilityComponent.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/HitStopManager.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"
#include "Config/GameplayConfig.h"
#include "Math/UnrealMathUtility.h"

UComboAbilityComponent::UComboAbilityComponent()
{
    // Enable ticking for time slow management
    PrimaryComponentTick.bCanEverTick = true;
    
    // Combos are special abilities with specific resource costs
    WPCost = 25.0f;
    Cooldown = 0.5f; // Shorter cooldown for combos
    
    // Default combo parameters
    ComboWindowTime = 0.5f;
    TimeSlowScale = 0.3f;
    TimeSlowDuration = 0.15f;
    DamageMultiplier = 1.5f;
    ComboRange = 300.0f;
    
    // Mark as not basic - combos are special
    bIsBasicAbility = false;
}

void UComboAbilityComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Cache references
    if (AActor* Owner = GetOwner())
    {
        OwnerCharacter = Cast<ABlackholePlayerCharacter>(Owner);
    }
    CachedWorld = GetWorld();
    
    // Ensure tick is enabled
    SetComponentTickEnabled(true);
}

void UComboAbilityComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Always reset time dilation when component is destroyed
    if (bIsTimeSlowActive && IsValid(CachedWorld))
    {
        UGameplayStatics::SetGlobalTimeDilation(CachedWorld, 1.0f);
        if (AWorldSettings* WorldSettings = CachedWorld->GetWorldSettings())
        {
            WorldSettings->SetTimeDilation(1.0f);
        }
        bIsTimeSlowActive = false;
        UE_LOG(LogTemp, Log, TEXT("ComboAbility: Reset time dilation in EndPlay"));
    }
    
    Super::EndPlay(EndPlayReason);
}

bool UComboAbilityComponent::CanExecute() const
{
    // Combos have special execution requirements
    // They're triggered by the combo system, not directly
    return Super::CanExecute();
}

void UComboAbilityComponent::Execute()
{
    // Reset any existing time slow before executing new combo
    if (bIsTimeSlowActive && IsValid(CachedWorld))
    {
        UGameplayStatics::SetGlobalTimeDilation(CachedWorld, 1.0f);
        bIsTimeSlowActive = false;
        UE_LOG(LogTemp, Log, TEXT("ComboAbility: Reset time before new combo execution"));
    }
    
    // Execute the combo
    ExecuteCombo();
}

void UComboAbilityComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Check if time slow should end (using real time)
    if (bIsTimeSlowActive && IsValid(CachedWorld))
    {
        // Don't reset if HitStop is active - let HitStop handle time dilation
        if (UHitStopManager* HitStopMgr = CachedWorld->GetSubsystem<UHitStopManager>())
        {
            if (HitStopMgr->IsHitStopActive())
            {
                // HitStop is active, delay our reset
                return;
            }
        }
        
        float CurrentRealTime = FPlatformTime::Seconds();
        if (CurrentRealTime >= TimeSlowEndTime)
        {
            // Get current dilation before reset
            float CurrentDilation = UGameplayStatics::GetGlobalTimeDilation(CachedWorld);
            
            // Reset time dilation using both methods
            UGameplayStatics::SetGlobalTimeDilation(CachedWorld, 1.0f);
            if (AWorldSettings* WorldSettings = CachedWorld->GetWorldSettings())
            {
                WorldSettings->SetTimeDilation(1.0f);
            }
            bIsTimeSlowActive = false;
            
            // Verify the reset worked
            float NewDilation = UGameplayStatics::GetGlobalTimeDilation(CachedWorld);
            UE_LOG(LogTemp, Warning, TEXT("ComboAbility: Time reset - Was: %f, Now: %f, TimeSlowEndTime: %f, CurrentTime: %f"), 
                CurrentDilation, NewDilation, TimeSlowEndTime, CurrentRealTime);
        }
    }
}

void UComboAbilityComponent::ApplyTimeSlow()
{
    if (!IsValid(CachedWorld) || TimeSlowDuration <= 0.0f) return;
    
    // If scale is 0 or negative, don't apply time slow
    if (TimeSlowScale <= 0.0f)
    {
        UE_LOG(LogTemp, Warning, TEXT("ComboAbility: TimeSlowScale is %f, skipping time slow"), TimeSlowScale);
        return;
    }
    
    // Check if HitStopManager is active - if so, skip time slow to avoid conflicts
    if (UHitStopManager* HitStopMgr = CachedWorld->GetSubsystem<UHitStopManager>())
    {
        if (HitStopMgr->IsHitStopActive())
        {
            UE_LOG(LogTemp, Warning, TEXT("ComboAbility: HitStop is active, skipping time slow to avoid conflicts"));
            return;
        }
    }
    
    // Get current dilation
    float CurrentDilation = UGameplayStatics::GetGlobalTimeDilation(CachedWorld);
    
    // Reset any existing time slow first
    if (bIsTimeSlowActive || CurrentDilation != 1.0f)
    {
        UGameplayStatics::SetGlobalTimeDilation(CachedWorld, 1.0f);
        bIsTimeSlowActive = false;
        UE_LOG(LogTemp, Warning, TEXT("ComboAbility: Reset existing time slow from %f to 1.0"), CurrentDilation);
    }
    
    // Apply time dilation using both methods to ensure consistency
    UGameplayStatics::SetGlobalTimeDilation(CachedWorld, TimeSlowScale);
    if (AWorldSettings* WorldSettings = CachedWorld->GetWorldSettings())
    {
        WorldSettings->SetTimeDilation(TimeSlowScale);
    }
    
    // Track when time slow should end using real time
    TimeSlowEndTime = FPlatformTime::Seconds() + TimeSlowDuration;
    bIsTimeSlowActive = true;
    
    // Verify it was applied
    float NewDilation = UGameplayStatics::GetGlobalTimeDilation(CachedWorld);
    UE_LOG(LogTemp, Warning, TEXT("ComboAbility: Applied time slow - Requested: %f, Actual: %f, Duration: %f, EndTime: %f"), 
        TimeSlowScale, NewDilation, TimeSlowDuration, TimeSlowEndTime);
}

void UComboAbilityComponent::ApplyHitStop(UHitStopManager* HitStopMgr, float DamageAmount)
{
    if (!HitStopMgr) return;
    
    // Apply hit stop based on damage
    if (DamageAmount >= 100.0f)
    {
        HitStopMgr->RequestCriticalHitStop();
    }
    else if (DamageAmount >= 50.0f)
    {
        HitStopMgr->RequestHeavyHitStop();
    }
    else if (DamageAmount >= 25.0f)
    {
        HitStopMgr->RequestMediumHitStop();
    }
    else
    {
        HitStopMgr->RequestLightHitStop();
    }
}

void UComboAbilityComponent::DrawComboVisuals(const FVector& Start, const FVector& End)
{
    if (!bShowDebugVisuals || !IsValid(CachedWorld)) return;
    
    #if WITH_EDITOR
    // Draw combo trail
    DrawDebugLine(CachedWorld, Start, End, ComboTrailColor.ToFColor(true), false, 2.0f, 0, 5.0f);
    
    // Draw impact sphere
    DrawDebugSphere(CachedWorld, End, 50.0f, 16, ComboTrailColor.ToFColor(true), false, 1.0f);
    #endif
}

void UComboAbilityComponent::PlayComboFeedback(const FVector& Location)
{
    if (!IsValid(CachedWorld)) return;
    
    // Play sound
    if (ComboSound)
    {
        UGameplayStatics::PlaySoundAtLocation(CachedWorld, ComboSound, Location);
    }
    
    // Spawn particle effect
    if (ComboParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(CachedWorld, ComboParticle, Location);
    }
    
    // Camera shake
    if (CameraShakeScale > 0.0f && OwnerCharacter)
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            PC->ClientStartCameraShake(nullptr, CameraShakeScale);
        }
    }
}