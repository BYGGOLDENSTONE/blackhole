#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "SlashAbilityComponent.generated.h"

class UStaminaComponent;
class UIntegrityComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API USlashAbilityComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	USlashAbilityComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slash")
	float Damage;

	virtual void Execute() override;
	virtual bool CanExecute() const override;
	
	// Basic abilities don't have ultimate versions
	virtual void ExecuteUltimate() override { Execute(); }
	
	// Combo execution methods
	void ExecutePhantomStrike();  // Dash + Slash combo
	void ExecuteAerialRave();     // Jump + Slash combo
	void ExecuteTempestBlade();   // Jump + Dash + Slash combo
	void ExecuteBladeDance(int32 HitCount); // Slash chain combo

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	UStaminaComponent* StaminaComponent;
	
	// Cached owner reference
	UPROPERTY()
	class ABlackholePlayerCharacter* OwnerCharacter;
	
	// Prevent recursive combo execution
	bool bIsExecutingCombo = false;
};