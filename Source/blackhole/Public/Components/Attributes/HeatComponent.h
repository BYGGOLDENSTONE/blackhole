#pragma once

#include "CoreMinimal.h"
#include "AttributeComponent.h"
#include "HeatComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UHeatComponent : public UAttributeComponent
{
	GENERATED_BODY()

public:
	UHeatComponent();

	// Override tick to handle heat dissipation
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UFUNCTION(BlueprintCallable, Category = "Heat")
	void AddHeat(float Amount);

	UFUNCTION(BlueprintCallable, Category = "Heat")
	bool IsOverheated() const;

	UFUNCTION(BlueprintCallable, Category = "Heat")
	float GetHeatPercentage() const { return GetPercentage(); }

	// Heat dissipation rate (per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat")
	float DissipationRate;

	// Threshold for overheat warning (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WarningThreshold;

	// Threshold for overheat (0.0 - 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Heat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float OverheatThreshold;

protected:
	virtual void BeginPlay() override;
};