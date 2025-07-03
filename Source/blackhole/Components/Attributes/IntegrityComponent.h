#pragma once

#include "CoreMinimal.h"
#include "AttributeComponent.h"
#include "IntegrityComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UIntegrityComponent : public UAttributeComponent
{
	GENERATED_BODY()

public:
	UIntegrityComponent();

	UFUNCTION(BlueprintCallable, Category = "Integrity")
	void TakeDamage(float DamageAmount);

	UFUNCTION(BlueprintCallable, Category = "Integrity")
	bool IsAlive() const { return CurrentValue > 0.0f; }

protected:
	virtual void BeginPlay() override;
};