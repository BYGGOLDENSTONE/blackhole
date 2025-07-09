#include "Components/ComboComponent.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

UComboComponent::UComboComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.1f; // Check combos 10 times per second
}

void UComboComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Register default combos
    // Dash + Slash (Shift + LMB)
    FComboSequence PhantomStrike;
    PhantomStrike.ComboName = "PhantomStrike";
    PhantomStrike.InputSequence = { "Dash", "Slash" };
    PhantomStrike.InputWindow = 0.5f;
    PhantomStrike.Description = "Dash + Slash = Teleport backstab";
    AddComboSequence(PhantomStrike);
    
    // Jump + Slash (Space + LMB)
    FComboSequence AerialRave;
    AerialRave.ComboName = "AerialRave";
    AerialRave.InputSequence = { "Jump", "Slash" };
    AerialRave.InputWindow = 0.3f;
    AerialRave.Description = "Jump + Slash = Aerial shockwave";
    AddComboSequence(AerialRave);
    
    // Jump + Dash + Slash
    FComboSequence TempestBlade;
    TempestBlade.ComboName = "TempestBlade";
    TempestBlade.InputSequence = { "Jump", "Dash", "Slash" };
    TempestBlade.InputWindow = 0.3f;
    TempestBlade.Description = "Jump + Dash + Slash = Multi-teleport";
    AddComboSequence(TempestBlade);
    
    // Slash + Slash
    FComboSequence BladeDance;
    BladeDance.ComboName = "BladeDance";
    BladeDance.InputSequence = { "Slash", "Slash" };
    BladeDance.InputWindow = 0.8f;
    BladeDance.Description = "Slash x2+ = Progressive combo";
    AddComboSequence(BladeDance);
}

void UComboComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear all data to prevent crashes
    InputHistory.Empty();
    ComboSequences.Empty();
    ComboCooldowns.Empty();
    LastPerformedCombo = NAME_None;
    
    // Clear delegate bindings
    OnComboPerformed.Clear();
    
    Super::EndPlay(EndPlayReason);
}

void UComboComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    // Safety check - don't process if owner is invalid
    if (!IsValid(GetOwner()) || !IsValid(GetWorld()))
    {
        return;
    }
    
    // Clean old inputs
    CleanInputHistory();
    
    // Update combo cooldowns
    for (auto& Pair : ComboCooldowns)
    {
        if (Pair.Value > 0.0f)
        {
            Pair.Value -= DeltaTime;
        }
    }
}

void UComboComponent::RegisterInput(const FName& InputName)
{
    // Safety check - don't process if owner is invalid
    if (!IsValid(GetOwner()) || !IsValid(GetWorld()))
    {
        UE_LOG(LogTemp, Warning, TEXT("ComboComponent: Cannot register input - invalid owner or world"));
        return;
    }
    
    // Additional safety check - don't register empty input names
    if (InputName.IsNone() || InputName.ToString().IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("ComboComponent: Cannot register empty input name"));
        return;
    }
    
    // Add to history
    InputHistory.Add(FComboInputEntry(InputName));
    
    // Keep history size limited
    if (InputHistory.Num() > MaxInputHistory)
    {
        InputHistory.RemoveAt(0);
    }
    
    // Check for combos
    CheckForCombos();
    
    // Debug log
    UE_LOG(LogTemp, Log, TEXT("ComboComponent: Registered input %s"), *InputName.ToString());
}

void UComboComponent::ClearInputHistory()
{
    InputHistory.Empty();
    LastPerformedCombo = NAME_None;
}

void UComboComponent::AddComboSequence(const FComboSequence& Sequence)
{
    ComboSequences.Add(Sequence);
    ComboCooldowns.Add(Sequence.ComboName, 0.0f);
}

FString UComboComponent::GetCurrentComboStatus() const
{
    if (LastPerformedCombo != NAME_None)
    {
        float TimeSinceCombo = FPlatformTime::Seconds() - LastComboTime;
        if (TimeSinceCombo < 2.0f)
        {
            return FString::Printf(TEXT("COMBO: %s!"), *LastPerformedCombo.ToString());
        }
    }
    
    // Show current input sequence
    if (InputHistory.Num() > 0)
    {
        FString InputString = "Inputs: ";
        for (int32 i = FMath::Max(0, InputHistory.Num() - 3); i < InputHistory.Num(); i++)
        {
            InputString += InputHistory[i].InputName.ToString() + " ";
        }
        return InputString;
    }
    
    return "";
}

void UComboComponent::CheckForCombos()
{
    // Safety check - don't process if owner is invalid
    if (!IsValid(GetOwner()) || !IsValid(GetWorld()))
    {
        return;
    }
    
    // Check each registered combo
    for (const FComboSequence& Combo : ComboSequences)
    {
        // Skip if on cooldown
        if (ComboCooldowns[Combo.ComboName] > 0.0f)
        {
            continue;
        }
        
        if (DoesComboMatch(Combo))
        {
            // Combo matched!
            LastPerformedCombo = Combo.ComboName;
            LastComboTime = FPlatformTime::Seconds();
            
            // Set cooldown
            ComboCooldowns[Combo.ComboName] = Combo.CooldownTime;
            
            // Broadcast event - check if delegate is valid
            if (OnComboPerformed.IsBound())
            {
                OnComboPerformed.Broadcast(Combo);
            }
            
            // Clear input history after successful combo
            InputHistory.Empty();
            
            UE_LOG(LogTemp, Warning, TEXT("COMBO PERFORMED: %s"), *Combo.ComboName.ToString());
            
            break; // Only one combo at a time
        }
    }
}

bool UComboComponent::DoesComboMatch(const FComboSequence& Combo) const
{
    // Safety checks
    if (Combo.InputSequence.Num() == 0 || InputHistory.Num() == 0)
    {
        return false;
    }
    
    // Need at least as many inputs as the combo requires
    if (InputHistory.Num() < Combo.InputSequence.Num())
    {
        return false;
    }
    
    // Get the starting index to check from
    int32 StartIndex = InputHistory.Num() - Combo.InputSequence.Num();
    
    // Validate StartIndex
    if (StartIndex < 0 || StartIndex >= InputHistory.Num())
    {
        return false;
    }
    
    // Check if inputs match in sequence
    for (int32 i = 0; i < Combo.InputSequence.Num(); i++)
    {
        int32 InputIndex = StartIndex + i;
        
        // Bounds check
        if (InputIndex < 0 || InputIndex >= InputHistory.Num())
        {
            return false;
        }
        
        const FComboInputEntry& Input = InputHistory[InputIndex];
        
        // Check if input name matches
        if (Input.InputName != Combo.InputSequence[i])
        {
            return false;
        }
        
        // Check timing window (except for first input)
        if (i > 0)
        {
            int32 PrevInputIndex = StartIndex + i - 1;
            
            // Bounds check for previous input
            if (PrevInputIndex < 0 || PrevInputIndex >= InputHistory.Num())
            {
                return false;
            }
            
            const FComboInputEntry& PrevInput = InputHistory[PrevInputIndex];
            float TimeDiff = Input.Timestamp - PrevInput.Timestamp;
            
            if (TimeDiff > Combo.InputWindow)
            {
                return false; // Too slow
            }
        }
    }
    
    return true;
}

void UComboComponent::CleanInputHistory()
{
    // Safety check - don't clean if we're being destroyed
    if (!IsValid(GetOwner()) || !IsValid(GetWorld()))
    {
        return;
    }
    
    float CurrentTime = FPlatformTime::Seconds();
    
    // Remove inputs older than timeout
    InputHistory.RemoveAll([this, CurrentTime](const FComboInputEntry& Input)
    {
        return (CurrentTime - Input.Timestamp) > InputHistoryTimeout;
    });
}