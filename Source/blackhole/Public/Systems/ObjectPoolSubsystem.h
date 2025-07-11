#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolSubsystem.generated.h"

USTRUCT()
struct FObjectPool
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<AActor*> AvailableObjects;

    UPROPERTY()
    TArray<AActor*> ActiveObjects;

    UPROPERTY()
    TSubclassOf<AActor> ObjectClass;

    int32 MaxPoolSize = 50;
    int32 InitialPoolSize = 10;
    bool bExpandable = true;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectSpawned, AActor*, SpawnedObject, bool, bFromPool);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnObjectReturned, AActor*, ReturnedObject);

/**
 * Object pooling subsystem for efficient spawning of frequently used actors
 * Particularly useful for projectiles, particles, and temporary effects
 */
UCLASS()
class BLACKHOLE_API UObjectPoolSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Subsystem implementation
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

    /**
     * Register a new object pool
     * @param PoolName Unique identifier for this pool
     * @param ObjectClass Class of objects to pool
     * @param InitialSize Number of objects to pre-spawn
     * @param MaxSize Maximum pool size (0 = unlimited)
     * @param bCanExpand Whether pool can grow beyond initial size
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (CallInEditor = "true"))
    void RegisterPool(FName PoolName, TSubclassOf<AActor> ObjectClass, int32 InitialSize = 10, int32 MaxSize = 50, bool bCanExpand = true);

    /**
     * Get an object from the pool
     * @param PoolName Pool to get object from
     * @param SpawnTransform Transform to apply to the object
     * @param bForceSpawn If true, spawn new object if pool is empty
     * @return Pooled object or nullptr if unavailable
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool", meta = (CallInEditor = "true"))
    AActor* GetPooledObject(FName PoolName, const FTransform& SpawnTransform, bool bForceSpawn = true);

    /**
     * Return an object to the pool
     * @param PooledObject Object to return
     * @return True if object was successfully returned
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    bool ReturnToPool(AActor* PooledObject);

    /**
     * Get pool statistics
     */
    UFUNCTION(BlueprintPure, Category = "Object Pool")
    void GetPoolStats(FName PoolName, int32& OutAvailable, int32& OutActive, int32& OutTotal) const;

    /**
     * Pre-warm a pool by spawning objects
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void PrewarmPool(FName PoolName, int32 Count);

    /**
     * Clear a specific pool
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ClearPool(FName PoolName);

    /**
     * Clear all pools
     */
    UFUNCTION(BlueprintCallable, Category = "Object Pool")
    void ClearAllPools();

    // Events
    UPROPERTY(BlueprintAssignable, Category = "Object Pool")
    FOnObjectSpawned OnObjectSpawned;

    UPROPERTY(BlueprintAssignable, Category = "Object Pool")
    FOnObjectReturned OnObjectReturned;

protected:
    // Pool storage
    UPROPERTY()
    TMap<FName, FObjectPool> ObjectPools;

    // Track which pool each object belongs to
    UPROPERTY()
    TMap<AActor*, FName> ObjectToPoolMap;

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Object Pool")
    bool bAutoReturnInactiveObjects = true;

    UPROPERTY(EditAnywhere, Category = "Object Pool")
    float InactiveCheckInterval = 5.0f;

private:
    // Helper functions
    AActor* SpawnPooledObject(FObjectPool& Pool);
    void PrepareObjectForUse(AActor* PooledObject, const FTransform& SpawnTransform);
    void PrepareObjectForReturn(AActor* PooledObject);
    void CheckInactiveObjects();

    // Timer for inactive object checks
    FTimerHandle InactiveCheckTimer;
};