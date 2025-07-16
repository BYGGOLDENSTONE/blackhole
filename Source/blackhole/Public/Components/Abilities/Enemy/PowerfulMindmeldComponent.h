#pragma once

#include "CoreMinimal.h"
#include "Components/Abilities/AbilityComponent.h"
#include "PowerfulMindmeldComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMindmeldStarted, float, CastTime);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMindmeldComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMindmeldInterrupted);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UPowerfulMindmeldComponent : public UAbilityComponent
{
	GENERATED_BODY()

public:
	UPowerfulMindmeldComponent();

	// Mindmeld stats
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Powerful Mindmeld", meta = (DisplayName = "Cast Time"))
	float CastTime = 30.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Powerful Mindmeld", meta = (DisplayName = "Channel Range"))
	float ChannelRange = 3000.0f; // Long range
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Powerful Mindmeld", meta = (DisplayName = "Requires Line of Sight"))
	bool bRequiresLineOfSight = true;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Powerful Mindmeld", meta = (DisplayName = "Interrupt Range"))
	float InterruptRange = 300.0f; // Player must get close to interrupt
	
	// Events
	UPROPERTY(BlueprintAssignable, Category = "Powerful Mindmeld")
	FOnMindmeldStarted OnMindmeldStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "Powerful Mindmeld")
	FOnMindmeldComplete OnMindmeldComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Powerful Mindmeld")
	FOnMindmeldInterrupted OnMindmeldInterrupted;

	virtual void Execute() override;
	virtual void Deactivate() override;
	
	UFUNCTION(BlueprintPure, Category = "Powerful Mindmeld")
	bool IsChanneling() const { return bIsChanneling; }
	
	UFUNCTION(BlueprintPure, Category = "Powerful Mindmeld")
	float GetChannelProgress() const;
	
	UFUNCTION(BlueprintPure, Category = "Powerful Mindmeld")
	float GetTimeRemaining() const;
	
	UFUNCTION(BlueprintCallable, Category = "Powerful Mindmeld")
	void InterruptChannel();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bIsChanneling;
	float ChannelStartTime;
	FTimerHandle ChannelCompleteTimer;
	
	UPROPERTY()
	AActor* ChannelTarget;
	
	void StartChannel();
	void UpdateChannel();
	void CompleteChannel();
	bool CheckInterruptConditions();
	void NotifyPlayerOfChannel();
};