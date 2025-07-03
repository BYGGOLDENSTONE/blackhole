#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "BlockComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UBlockComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UBlockComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block")
	float BlockDuration;

	UPROPERTY(BlueprintReadOnly, Category = "Block")
	bool bIsBlocking;

	virtual void Execute() override;

	UFUNCTION(BlueprintCallable, Category = "Block")
	bool IsBlocking() const { return bIsBlocking; }

protected:
	virtual void BeginPlay() override;

private:
	FTimerHandle BlockTimerHandle;
	void StopBlocking();
};