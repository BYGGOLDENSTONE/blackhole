#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "SlashAbilityComponent.generated.h"

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

protected:
	virtual void BeginPlay() override;

private:
	// Cached owner reference
	UPROPERTY()
	class ABlackholePlayerCharacter* OwnerCharacter;
};