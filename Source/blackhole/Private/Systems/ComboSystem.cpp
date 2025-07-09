#include "Systems/ComboSystem.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Systems/ResourceManager.h"
#include "Components/Abilities/AbilityComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Kismet/GameplayStatics.h"
#include "Config/GameplayConfig.h"
#include "Utils/ErrorHandling.h"

UComboSystem::UComboSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f; // 20Hz is enough for combo tracking
}

void UComboSystem::BeginPlay()
{
    Super::BeginPlay();

    // Cache owner
    OwnerCharacter = Cast<ABlackholePlayerCharacter>(GetOwner());
    CHECK_CAST_VOID(OwnerCharacter);

    // Get resource manager
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            ResourceManager = GameInstance->GetSubsystem<UResourceManager>();
        }
    }

    // Register default combos
    RegisterDefaultCombos();
}

void UComboSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Update input history
    UpdateInputHistory(DeltaTime);

    // Update active combo window
    if (ActiveCombo.WindowTimeRemaining > 0.0f)
    {
        ActiveCombo.WindowTimeRemaining -= DeltaTime;
        
        if (ActiveCombo.WindowTimeRemaining <= 0.0f)
        {
            CloseComboWindow();
        }
    }

    // Update combo reset timer
    if (ComboResetTimer > 0.0f)
    {
        ComboResetTimer -= DeltaTime;
        if (ComboResetTimer <= 0.0f && ActiveCombo.CurrentInputIndex > 0)
        {
            ResetCombo();
        }
    }

    // Debug drawing
    if (bDebugDrawComboWindow && ActiveCombo.WindowTimeRemaining > 0.0f)
    {
        float WindowPercent = ActiveCombo.WindowTimeRemaining / 
            (ActiveCombo.Pattern.TimingWindows.IsValidIndex(ActiveCombo.CurrentInputIndex) ? 
             ActiveCombo.Pattern.TimingWindows[ActiveCombo.CurrentInputIndex] : 1.0f);
        
        DrawDebugSphere(
            GetWorld(),
            OwnerCharacter->GetActorLocation() + FVector(0, 0, 100),
            50.0f * WindowPercent,
            16,
            FColor::Cyan,
            false,
            0.1f
        );
    }
}

void UComboSystem::RegisterInput(EComboInputType InputType, FVector InputLocation)
{
    // Create input record
    FComboInput NewInput(InputType);
    NewInput.InputLocation = InputLocation.IsZero() ? OwnerCharacter->GetActorLocation() : InputLocation;
    
    // Add to history
    InputHistory.Add(NewInput);
    LastInputTime = FPlatformTime::Seconds();

    // Check if this continues an active combo
    if (ActiveCombo.CurrentInputIndex > 0)
    {
        const FComboPattern& Pattern = ActiveCombo.Pattern;
        
        // Check if input matches expected
        if (Pattern.RequiredInputs.IsValidIndex(ActiveCombo.CurrentInputIndex) &&
            Pattern.RequiredInputs[ActiveCombo.CurrentInputIndex] == InputType)
        {
            // Check timing
            float ExpectedWindow = Pattern.TimingWindows.IsValidIndex(ActiveCombo.CurrentInputIndex) ? 
                                   Pattern.TimingWindows[ActiveCombo.CurrentInputIndex] : 0.5f;
            
            if (ActiveCombo.WindowTimeRemaining > 0.0f)
            {
                // Success! Advance combo
                ActiveCombo.CurrentInputIndex++;
                
                // Check if combo is complete
                if (ActiveCombo.CurrentInputIndex >= Pattern.RequiredInputs.Num())
                {
                    // Execute combo!
                    bool bPerfect = ActiveCombo.bPerfectTiming && 
                                    (ExpectedWindow - ActiveCombo.WindowTimeRemaining) < PerfectTimingWindow;
                    
                    ExecuteComboEffects(Pattern, bPerfect);
                    OnComboExecuted.Broadcast(Pattern, bPerfect);
                    ResetCombo();
                }
                else
                {
                    // Continue combo - open next window
                    float NextWindow = Pattern.TimingWindows.IsValidIndex(ActiveCombo.CurrentInputIndex) ? 
                                       Pattern.TimingWindows[ActiveCombo.CurrentInputIndex] : 0.5f;
                    StartComboWindow(NextWindow);
                    
                    // Update progress
                    OnComboProgress.Broadcast(ActiveCombo.CurrentInputIndex, Pattern.RequiredInputs.Num());
                }
            }
            else
            {
                // Too late - reset combo
                ResetCombo();
            }
        }
        else
        {
            // Wrong input - reset combo
            ResetCombo();
        }
    }
    else
    {
        // Not in combo - check if this starts one
        CheckForComboMatch();
    }

    // Reset combo timer
    ComboResetTimer = ComboResetDelay;
}

bool UComboSystem::CheckForComboMatch()
{
    // Check each registered combo
    for (const FComboPattern& Pattern : RegisteredCombos)
    {
        if (Pattern.RequiredInputs.Num() == 0) continue;
        
        // Check if most recent input matches first input of pattern
        if (InputHistory.Num() > 0 && 
            InputHistory.Last().InputType == Pattern.RequiredInputs[0])
        {
            // Start this combo!
            ActiveCombo.Pattern = Pattern;
            ActiveCombo.CurrentInputIndex = 1;
            ActiveCombo.bPerfectTiming = true;
            
            // If combo has more inputs, open window for next
            if (Pattern.RequiredInputs.Num() > 1)
            {
                float WindowDuration = Pattern.TimingWindows.IsValidIndex(1) ? 
                                       Pattern.TimingWindows[1] : 0.5f;
                StartComboWindow(WindowDuration);
            }
            else
            {
                // Single input combo - execute immediately
                ExecuteComboEffects(Pattern, true);
                OnComboExecuted.Broadcast(Pattern, true);
                ResetCombo();
            }
            
            return true;
        }
    }
    
    return false;
}

void UComboSystem::ResetCombo()
{
    ActiveCombo = FActiveCombo();
    ComboResetTimer = 0.0f;
}

void UComboSystem::StartComboWindow(float Duration)
{
    ActiveCombo.WindowTimeRemaining = Duration;
    OnComboWindowOpen.Broadcast(Duration);
}

void UComboSystem::CloseComboWindow()
{
    ActiveCombo.WindowTimeRemaining = 0.0f;
    OnComboWindowClosed.Broadcast();
    
    // If we were in a combo, reset it
    if (ActiveCombo.CurrentInputIndex > 0)
    {
        ResetCombo();
    }
}

void UComboSystem::UpdateInputHistory(float DeltaTime)
{
    // Remove old inputs
    float CurrentTime = FPlatformTime::Seconds();
    InputHistory.RemoveAll([this, CurrentTime](const FComboInput& Input)
    {
        return (CurrentTime - Input.Timestamp) > InputHistoryDuration;
    });
}

void UComboSystem::ExecuteComboEffects(const FComboPattern& Combo, bool bPerfectTiming)
{
    if (!OwnerCharacter) return;

    // Apply resource discount
    ApplyResourceDiscount(Combo);

    // Apply damage bonus
    ApplyDamageBonus(Combo);

    // Visual feedback
    PlayComboFeedback(Combo);

    // UI feedback
    ShowComboUI(Combo, bPerfectTiming);

    // Log for debug
    UE_LOG(LogTemp, Log, TEXT("Combo Executed: %s (Perfect: %s)"), 
           *Combo.ComboName.ToString(), 
           bPerfectTiming ? TEXT("Yes") : TEXT("No"));
}

void UComboSystem::ApplyResourceDiscount(const FComboPattern& Combo)
{
    if (!ResourceManager) return;

    // Apply discount to next ability
    float Discount = Combo.ResourceDiscount;
    if (ActiveCombo.bPerfectTiming)
    {
        Discount += 0.1f; // Extra 10% for perfect timing
    }

    // Resource discount is handled by abilities themselves when they check CanExecute()
    // The discount is applied through the combo pattern's ResourceDiscount field
}

void UComboSystem::ApplyDamageBonus(const FComboPattern& Combo)
{
    // This would integrate with damage system
    // For now, log the multiplier
    float FinalMultiplier = Combo.DamageMultiplier;
    if (ActiveCombo.bPerfectTiming)
    {
        FinalMultiplier *= 1.2f; // 20% bonus for perfect
    }
}

void UComboSystem::PlayComboFeedback(const FComboPattern& Combo)
{
    if (!OwnerCharacter) return;

    // Spawn particle effect
    if (Combo.ComboEffect)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            Combo.ComboEffect,
            OwnerCharacter->GetActorLocation(),
            FRotator::ZeroRotator,
            FVector(1.5f)
        );
    }

    // Play sound
    if (Combo.ComboSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            GetWorld(),
            Combo.ComboSound,
            OwnerCharacter->GetActorLocation()
        );
    }

    // Camera shake
    if (Combo.CameraShake)
    {
        if (APlayerController* PC = Cast<APlayerController>(OwnerCharacter->GetController()))
        {
            PC->ClientStartCameraShake(Combo.CameraShake);
        }
    }
}

void UComboSystem::ShowComboUI(const FComboPattern& Combo, bool bPerfectTiming)
{
    // This would integrate with HUD
    // For now, just broadcast the event
}

void UComboSystem::RegisterComboPattern(const FComboPattern& Pattern)
{
    RegisteredCombos.Add(Pattern);
}

void UComboSystem::ClearComboPatterns()
{
    RegisteredCombos.Empty();
    ResetCombo();
}

bool UComboSystem::TryExecuteCombo(const FComboPattern& ComboPattern)
{
    // Manual combo execution (for special cases)
    ExecuteComboEffects(ComboPattern, false);
    return true;
}

void UComboSystem::RegisterDefaultCombos()
{
    // Dash + Slash = Phantom Strike
    FComboPattern PhantomStrike;
    PhantomStrike.ComboName = "PhantomStrike";
    PhantomStrike.RequiredInputs = { EComboInputType::Dash, EComboInputType::Slash };
    PhantomStrike.TimingWindows = { 0.0f, 0.5f }; // 0.5s to slash after dash
    PhantomStrike.ResourceDiscount = 0.5f;
    PhantomStrike.DamageMultiplier = 1.5f;
    RegisterComboPattern(PhantomStrike);

    // Jump + Slash = Aerial Rave
    FComboPattern AerialRave;
    AerialRave.ComboName = "AerialRave";
    AerialRave.RequiredInputs = { EComboInputType::Jump, EComboInputType::Slash };
    AerialRave.TimingWindows = { 0.0f, 0.3f }; // 0.3s window while airborne
    AerialRave.ResourceDiscount = 0.25f;
    AerialRave.DamageMultiplier = 1.25f;
    RegisterComboPattern(AerialRave);

    // Jump + Dash + Slash = Tempest Blade
    FComboPattern TempestBlade;
    TempestBlade.ComboName = "TempestBlade";
    TempestBlade.RequiredInputs = { EComboInputType::Jump, EComboInputType::Dash, EComboInputType::Slash };
    TempestBlade.TimingWindows = { 0.0f, 0.3f, 0.3f }; // Tight windows
    TempestBlade.ResourceDiscount = 0.4f;
    TempestBlade.DamageMultiplier = 2.0f;
    RegisterComboPattern(TempestBlade);

    // Slash + Slash = Blade Dance (start)
    FComboPattern BladeDance;
    BladeDance.ComboName = "BladeDance";
    BladeDance.RequiredInputs = { EComboInputType::Slash, EComboInputType::Slash };
    BladeDance.TimingWindows = { 0.0f, 0.8f }; // Rhythm window
    BladeDance.ResourceDiscount = 0.2f;
    BladeDance.DamageMultiplier = 1.0f; // Increases per hit
    RegisterComboPattern(BladeDance);
}