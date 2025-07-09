#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputActionValue.h"
#include "ComboComponent.generated.h"

USTRUCT(BlueprintType)
struct FComboInputEntry
{
    GENERATED_BODY()

    UPROPERTY()
    FName InputName;
    
    UPROPERTY()
    float Timestamp;
    
    FComboInputEntry()
    {
        InputName = NAME_None;
        Timestamp = 0.0f;
    }
    
    FComboInputEntry(const FName& Name)
    {
        InputName = Name;
        Timestamp = FPlatformTime::Seconds();
    }
};

USTRUCT(BlueprintType)
struct FComboSequence
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName ComboName;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FName> InputSequence;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float InputWindow = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float CooldownTime = 1.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FString Description;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnComboPerformed, const FComboSequence&, Combo);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UComboComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UComboComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    // Register an input for combo detection
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void RegisterInput(const FName& InputName);
    
    // Clear input history
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void ClearInputHistory();
    
    // Add a combo sequence to track
    UFUNCTION(BlueprintCallable, Category = "Combo")
    void AddComboSequence(const FComboSequence& Sequence);
    
    // Get current combo status
    UFUNCTION(BlueprintPure, Category = "Combo")
    FString GetCurrentComboStatus() const;
    
    // Get last performed combo
    UFUNCTION(BlueprintPure, Category = "Combo")
    FName GetLastPerformedCombo() const { return LastPerformedCombo; }
    
    // Events
    UPROPERTY(BlueprintAssignable, Category = "Combo")
    FOnComboPerformed OnComboPerformed;

protected:
    // Input history
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combo|Debug")
    TArray<FComboInputEntry> InputHistory;
    
    // Registered combo sequences
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    TArray<FComboSequence> ComboSequences;
    
    // Configuration
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    float InputHistoryTimeout = 2.0f;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combo")
    int32 MaxInputHistory = 10;

private:
    // Check if current input history matches any combo
    void CheckForCombos();
    
    // Check if a specific combo matches current inputs
    bool DoesComboMatch(const FComboSequence& Combo) const;
    
    // Clean old inputs from history
    void CleanInputHistory();
    
    // Combo cooldowns
    TMap<FName, float> ComboCooldowns;
    
    // Last performed combo
    FName LastPerformedCombo;
    
    // For HUD display
    float LastComboTime = 0.0f;
};