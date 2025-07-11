#include "Data/ComboDataAsset.h"
#include "Components/Abilities/ComboAbilityComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UComboDataAsset::FindComboByName(FName ComboName, FComboDefinition& OutCombo) const
{
    for (const FComboDefinition& Combo : ComboDefinitions)
    {
        if (Combo.ComboName == ComboName)
        {
            OutCombo = Combo;
            return true;
        }
    }
    return false;
}

TArray<FComboDefinition> UComboDataAsset::GetCombosStartingWith(EComboInput Input) const
{
    TArray<FComboDefinition> Result;
    
    for (const FComboDefinition& Combo : ComboDefinitions)
    {
        if (Combo.Steps.Num() > 0 && Combo.Steps[0].RequiredInput == Input)
        {
            Result.Add(Combo);
        }
    }
    
    return Result;
}

void UComboDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    
    // Validate combo definitions when edited
    for (FComboDefinition& Combo : ComboDefinitions)
    {
        // Ensure combo has a valid name
        if (Combo.ComboName.IsNone())
        {
            Combo.ComboName = FName(*FString::Printf(TEXT("Combo_%d"), ComboDefinitions.Num()));
        }
        
        // Ensure display name is set
        if (Combo.DisplayName.IsEmpty())
        {
            Combo.DisplayName = FText::FromName(Combo.ComboName);
        }
        
        // Validate steps
        for (int32 i = Combo.Steps.Num() - 1; i >= 0; --i)
        {
            if (Combo.Steps[i].RequiredInput == EComboInput::None)
            {
                Combo.Steps.RemoveAt(i);
            }
        }
    }
}

#if WITH_EDITOR
EDataValidationResult UComboDataAsset::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = Super::IsDataValid(Context);
    
    TSet<FName> UsedNames;
    
    for (const FComboDefinition& Combo : ComboDefinitions)
    {
        // Check for duplicate names
        if (UsedNames.Contains(Combo.ComboName))
        {
            Context.AddError(FText::Format(
                NSLOCTEXT("ComboDataAsset", "DuplicateName", "Duplicate combo name: {0}"),
                FText::FromName(Combo.ComboName)
            ));
            Result = EDataValidationResult::Invalid;
        }
        UsedNames.Add(Combo.ComboName);
        
        // Validate combo has steps
        if (Combo.Steps.Num() == 0)
        {
            Context.AddError(FText::Format(
                NSLOCTEXT("ComboDataAsset", "NoSteps", "Combo '{0}' has no steps defined"),
                FText::FromName(Combo.ComboName)
            ));
            Result = EDataValidationResult::Invalid;
        }
        
        // Validate combo has ability class
        if (!Combo.ComboAbilityClass)
        {
            Context.AddError(FText::Format(
                NSLOCTEXT("ComboDataAsset", "NoAbilityClass", "Combo '{0}' has no ability class assigned"),
                FText::FromName(Combo.ComboName)
            ));
            Result = EDataValidationResult::Invalid;
        }
    }
    
    return Result;
}
#endif