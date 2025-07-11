#include "Systems/ComboDetectionSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Player/BlackholePlayerCharacter.h"
#include "Components/Abilities/ComboAbilityComponent.h"

void UComboDetectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Set up periodic update timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(UpdateTimerHandle, this, &UComboDetectionSubsystem::UpdateComboStates, 0.1f, true);
    }
}

void UComboDetectionSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
    
    ActiveCombos.Empty();
    InputHistories.Empty();
    
    Super::Deinitialize();
}

void UComboDetectionSubsystem::RegisterInput(AActor* InputOwner, EComboInput Input, FVector InputLocation)
{
    if (!InputOwner || Input == EComboInput::None)
    {
        return;
    }
    
    // Record input
    FComboInputRecord NewInput;
    NewInput.Input = Input;
    NewInput.Timestamp = GetWorld()->GetTimeSeconds();
    NewInput.InputLocation = InputLocation;
    
    TArray<FComboInputRecord>& History = InputHistories.FindOrAdd(InputOwner);
    History.Add(NewInput);
    
    // Check if this continues an existing combo
    if (FActiveComboState* ActiveCombo = ActiveCombos.Find(InputOwner))
    {
        if (ActiveCombo->bIsActive)
        {
            AdvanceCombo(InputOwner, *ActiveCombo, Input);
            return;
        }
    }
    
    // Check for new combo starts
    CheckForNewCombos(InputOwner, Input);
    
    // Cleanup old inputs
    CleanupInputHistory(InputOwner);
}

void UComboDetectionSubsystem::UpdateComboStates()
{
    float CurrentTime = GetWorld()->GetTimeSeconds();
    
    // Check all active combos for timeout
    for (auto It = ActiveCombos.CreateIterator(); It; ++It)
    {
        AActor* Owner = It->Key;
        FActiveComboState& ComboState = It->Value;
        
        if (ComboState.bIsActive && CurrentTime > ComboState.WindowEndTime)
        {
            // Combo window expired
            BreakCombo(Owner);
        }
    }
}

void UComboDetectionSubsystem::CheckForNewCombos(AActor* Owner, EComboInput NewInput)
{
    if (!ComboData)
    {
        return;
    }
    
    // Get all combos that could start with this input
    TArray<FComboDefinition> PotentialCombos = ComboData->GetCombosStartingWith(NewInput);
    
    if (PotentialCombos.Num() > 0)
    {
        // Start tracking the first matching combo (could be improved to track multiple)
        const FComboDefinition& FirstCombo = PotentialCombos[0];
        FActiveComboState& NewComboState = ActiveCombos.FindOrAdd(Owner);
        NewComboState.ComboName = FirstCombo.ComboName;
        NewComboState.CurrentStep = 0;
        NewComboState.TotalSteps = FirstCombo.Steps.Num();
        NewComboState.WindowEndTime = GetWorld()->GetTimeSeconds() + FirstCombo.Steps[0].TimeWindow;
        NewComboState.bIsActive = true;
        
        OnComboStarted.Broadcast(FirstCombo);
        OnComboProgress.Broadcast(1, FirstCombo.Steps.Num());
    }
}

void UComboDetectionSubsystem::AdvanceCombo(AActor* Owner, FActiveComboState& ComboState, EComboInput NewInput)
{
    if (!ComboState.bIsActive || ComboState.ComboName.IsNone() || !ComboData)
    {
        return;
    }
    
    // Look up the combo definition
    FComboDefinition ComboDefinition;
    if (!ComboData->FindComboByName(ComboState.ComboName, ComboDefinition))
    {
        // Combo no longer exists?
        BreakCombo(Owner);
        return;
    }
    
    int32 NextStep = ComboState.CurrentStep + 1;
    
    // Check if we've completed all steps
    if (NextStep >= ComboDefinition.Steps.Num())
    {
        CompleteCombo(Owner, ComboState);
        return;
    }
    
    // Check if input matches next step
    if (ComboDefinition.Steps[NextStep].RequiredInput == NewInput)
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        
        // Check if within time window
        if (CurrentTime <= ComboState.WindowEndTime)
        {
            // Advance to next step
            ComboState.CurrentStep = NextStep;
            ComboState.WindowEndTime = CurrentTime + ComboDefinition.Steps[NextStep].TimeWindow;
            
            OnComboProgress.Broadcast(NextStep + 1, ComboState.TotalSteps);
            
            // Check if this was the final step
            if (NextStep == ComboDefinition.Steps.Num() - 1)
            {
                CompleteCombo(Owner, ComboState);
            }
        }
        else
        {
            // Too slow
            BreakCombo(Owner);
        }
    }
    else
    {
        // Wrong input
        BreakCombo(Owner);
    }
}

void UComboDetectionSubsystem::CompleteCombo(AActor* Owner, const FActiveComboState& ComboState)
{
    if (!ComboState.bIsActive || ComboState.ComboName.IsNone() || !ComboData)
    {
        return;
    }
    
    // Look up the combo definition
    FComboDefinition ComboDefinition;
    if (!ComboData->FindComboByName(ComboState.ComboName, ComboDefinition))
    {
        return;
    }
    
    // Calculate timing score
    float TimingScore = 1.0f; // Simplified for now
    bool bPerfectTiming = TimingScore >= 0.9f;
    
    OnComboCompleted.Broadcast(ComboDefinition, bPerfectTiming);
    
    // Execute the combo ability
    if (ABlackholePlayerCharacter* PlayerCharacter = Cast<ABlackholePlayerCharacter>(Owner))
    {
        if (ComboDefinition.ComboAbilityClass)
        {
            // Find the combo component on the player
            if (UComboAbilityComponent* ComboComponent = PlayerCharacter->FindComponentByClass<UComboAbilityComponent>())
            {
                if (ComboComponent->IsA(ComboDefinition.ComboAbilityClass))
                {
                    ComboComponent->Execute();
                }
            }
        }
    }
    
    // Reset combo state
    ActiveCombos.Remove(Owner);
}

void UComboDetectionSubsystem::BreakCombo(AActor* Owner)
{
    if (FActiveComboState* ComboState = ActiveCombos.Find(Owner))
    {
        if (ComboState->bIsActive)
        {
            OnComboBroken.Broadcast();
            ComboState->Reset();
        }
    }
}

void UComboDetectionSubsystem::CleanupInputHistory(AActor* Owner)
{
    if (TArray<FComboInputRecord>* History = InputHistories.Find(Owner))
    {
        float CurrentTime = GetWorld()->GetTimeSeconds();
        float CutoffTime = CurrentTime - InputHistoryDuration;
        
        // Remove old inputs
        History->RemoveAll([CutoffTime](const FComboInputRecord& Input)
        {
            return Input.Timestamp < CutoffTime;
        });
    }
}

bool UComboDetectionSubsystem::IsInCombo(AActor* Owner) const
{
    if (const FActiveComboState* ComboState = ActiveCombos.Find(Owner))
    {
        return ComboState->bIsActive;
    }
    return false;
}

float UComboDetectionSubsystem::GetComboWindowRemaining(AActor* Owner) const
{
    if (const FActiveComboState* ComboState = ActiveCombos.Find(Owner))
    {
        if (ComboState->bIsActive)
        {
            return FMath::Max(0.0f, ComboState->WindowEndTime - GetWorld()->GetTimeSeconds());
        }
    }
    return 0.0f;
}

int32 UComboDetectionSubsystem::GetComboProgress(AActor* Owner) const
{
    if (const FActiveComboState* ComboState = ActiveCombos.Find(Owner))
    {
        if (ComboState->bIsActive)
        {
            return ComboState->CurrentStep + 1;
        }
    }
    return 0;
}

void UComboDetectionSubsystem::SetComboDataAsset(UComboDataAsset* DataAsset)
{
    ComboData = DataAsset;
}

bool UComboDetectionSubsystem::IsInputWithinWindow(float InputTime, float WindowStart, float WindowEnd) const
{
    return InputTime >= WindowStart && InputTime <= WindowEnd;
}

float UComboDetectionSubsystem::CalculateTimingScore(float InputTime, float IdealTime) const
{
    float Difference = FMath::Abs(InputTime - IdealTime);
    if (Difference <= PerfectTimingWindow)
    {
        return 1.0f;
    }
    return FMath::Max(0.0f, 1.0f - (Difference / PerfectTimingWindow));
}