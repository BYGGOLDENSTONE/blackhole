#pragma once

#include "CoreMinimal.h"
#include "AbilityComponent.h"
#include "MindmeldComponent.generated.h"

class UWillPowerComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UMindmeldComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UMindmeldComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mindmeld")
	float DrainRate;

	UPROPERTY(BlueprintReadOnly, Category = "Mindmeld")
	bool bIsMindmeldActive;

	virtual void Execute() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Mindmeld")
	void SetTarget(AActor* NewTarget);

	UFUNCTION(BlueprintCallable, Category = "Mindmeld")
	void StopMindmeld();

protected:
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	AActor* TargetActor;

	bool HasLineOfSight() const;
};