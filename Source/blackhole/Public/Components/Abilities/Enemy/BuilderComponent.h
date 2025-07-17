#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuilderComponent.generated.h"

class APsiDisruptor;
class AStandardEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBuildingStarted, const FVector&, BuildLocation);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuildingComplete);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnBuildingCancelled);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class BLACKHOLE_API UBuilderComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBuilderComponent();

	// Building parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder", meta = (DisplayName = "Build Time"))
	float BuildTime = 20.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder", meta = (DisplayName = "Build Radius"))
	float BuildRadius = 500.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder", meta = (DisplayName = "Min Builders Required"))
	int32 MinBuildersRequired = 2;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder", meta = (DisplayName = "Psi-Disruptor Class"))
	TSubclassOf<AActor> PsiDisruptorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Builder", meta = (DisplayName = "Build Sphere Material"))
	class UMaterialInterface* BuildSphereMaterial;
	
	// Events
	UPROPERTY(BlueprintAssignable, Category = "Builder")
	FOnBuildingStarted OnBuildingStarted;
	
	UPROPERTY(BlueprintAssignable, Category = "Builder")
	FOnBuildingComplete OnBuildingComplete;
	
	UPROPERTY(BlueprintAssignable, Category = "Builder")
	FOnBuildingCancelled OnBuildingCancelled;
	
	// Building coordination
	UFUNCTION(BlueprintCallable, Category = "Builder")
	void InitiateBuild(const FVector& BuildLocation);
	
	UFUNCTION(BlueprintCallable, Category = "Builder")
	void JoinBuild(UBuilderComponent* LeaderBuilder);
	
	UFUNCTION(BlueprintCallable, Category = "Builder")
	void CancelBuild();
	
	UFUNCTION(BlueprintCallable, Category = "Builder")
	void PauseBuild();
	
	UFUNCTION(BlueprintCallable, Category = "Builder")
	void ResumeBuild();
	
	UFUNCTION(BlueprintPure, Category = "Builder")
	bool IsBuilding() const { return bIsBuilding; }
	
	UFUNCTION(BlueprintPure, Category = "Builder")
	bool IsBuildPaused() const { return bBuildPaused; }
	
	UFUNCTION(BlueprintPure, Category = "Builder")
	bool IsLeader() const { return bIsBuildLeader; }
	
	UFUNCTION(BlueprintPure, Category = "Builder")
	float GetBuildProgress() const;
	
	UFUNCTION(BlueprintPure, Category = "Builder")
	FVector GetBuildLocation() const { return CurrentBuildLocation; }

	// Static build coordination
	static UBuilderComponent* FindNearestBuildLeader(AActor* SearchOrigin, float MaxRange);
	static TArray<UBuilderComponent*> GetAllActiveBuilders();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	bool bIsBuilding;
	bool bIsBuildLeader;
	bool bBuildPaused;
	float BuildProgress;
	float CurrentBuildTime; // Actual time remaining
	float TimeSpentBuilding; // Time already spent
	FVector CurrentBuildLocation;
	
	UPROPERTY()
	UBuilderComponent* LeaderComponent;
	
	UPROPERTY()
	TArray<UBuilderComponent*> ParticipatingBuilders;
	
	UPROPERTY()
	APsiDisruptor* SpawnedDisruptor;
	
	// Visual build sphere
	UPROPERTY()
	class UStaticMeshComponent* BuildSphere;
	
	UPROPERTY()
	class AActor* BuildSphereActor;
	
	FTimerHandle BuildTimerHandle;
	int32 InitialBuilderCount; // Track initial builders for timer calculation
	
	void UpdateBuildProgress();
	void CompleteBuild();
	void SpawnPsiDisruptor();
	void CreateBuildSphere();
	void DestroyBuildSphere();
	
	// Static tracking of all active builders
	static TArray<UBuilderComponent*> AllActiveBuilders;
};