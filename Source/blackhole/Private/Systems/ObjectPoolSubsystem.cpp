#include "Systems/ObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Components/SceneComponent.h"

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    
    // Set up inactive object checking timer
    if (bAutoReturnInactiveObjects && InactiveCheckInterval > 0.0f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                InactiveCheckTimer,
                this,
                &UObjectPoolSubsystem::CheckInactiveObjects,
                InactiveCheckInterval,
                true
            );
        }
    }
}

void UObjectPoolSubsystem::Deinitialize()
{
    // Clear timer
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(InactiveCheckTimer);
    }
    
    // Destroy all pooled objects
    ClearAllPools();
    
    Super::Deinitialize();
}

void UObjectPoolSubsystem::RegisterPool(FName PoolName, TSubclassOf<AActor> ObjectClass, int32 InitialSize, int32 MaxSize, bool bCanExpand)
{
    if (PoolName.IsNone() || !ObjectClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectPool: Invalid pool registration - name or class is null"));
        return;
    }
    
    // Check if pool already exists
    if (ObjectPools.Contains(PoolName))
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectPool: Pool '%s' already exists"), *PoolName.ToString());
        return;
    }
    
    // Create new pool
    FObjectPool NewPool;
    NewPool.ObjectClass = ObjectClass;
    NewPool.InitialPoolSize = InitialSize;
    NewPool.MaxPoolSize = MaxSize > 0 ? MaxSize : INT32_MAX;
    NewPool.bExpandable = bCanExpand;
    
    ObjectPools.Add(PoolName, NewPool);
    
    // Pre-warm the pool
    PrewarmPool(PoolName, InitialSize);
}

AActor* UObjectPoolSubsystem::GetPooledObject(FName PoolName, const FTransform& SpawnTransform, bool bForceSpawn)
{
    FObjectPool* Pool = ObjectPools.Find(PoolName);
    if (!Pool)
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectPool: Pool '%s' not found"), *PoolName.ToString());
        return nullptr;
    }
    
    AActor* PooledObject = nullptr;
    
    // Try to get an available object
    if (Pool->AvailableObjects.Num() > 0)
    {
        PooledObject = Pool->AvailableObjects.Pop();
        Pool->ActiveObjects.Add(PooledObject);
        
        // Prepare for use
        PrepareObjectForUse(PooledObject, SpawnTransform);
        
        OnObjectSpawned.Broadcast(PooledObject, true);
    }
    else if (bForceSpawn && (Pool->bExpandable || Pool->ActiveObjects.Num() < Pool->MaxPoolSize))
    {
        // Spawn new object if allowed
        PooledObject = SpawnPooledObject(*Pool);
        if (PooledObject)
        {
            Pool->ActiveObjects.Add(PooledObject);
            ObjectToPoolMap.Add(PooledObject, PoolName);
            
            // Prepare for use
            PrepareObjectForUse(PooledObject, SpawnTransform);
            
            OnObjectSpawned.Broadcast(PooledObject, false);
        }
    }
    
    return PooledObject;
}

bool UObjectPoolSubsystem::ReturnToPool(AActor* PooledObject)
{
    if (!PooledObject)
    {
        return false;
    }
    
    // Find which pool this object belongs to
    FName* PoolNamePtr = ObjectToPoolMap.Find(PooledObject);
    if (!PoolNamePtr)
    {
        UE_LOG(LogTemp, Warning, TEXT("ObjectPool: Object not found in any pool"));
        return false;
    }
    
    FObjectPool* Pool = ObjectPools.Find(*PoolNamePtr);
    if (!Pool)
    {
        return false;
    }
    
    // Move from active to available
    if (Pool->ActiveObjects.Remove(PooledObject) > 0)
    {
        PrepareObjectForReturn(PooledObject);
        Pool->AvailableObjects.Add(PooledObject);
        
        OnObjectReturned.Broadcast(PooledObject);
        return true;
    }
    
    return false;
}

void UObjectPoolSubsystem::GetPoolStats(FName PoolName, int32& OutAvailable, int32& OutActive, int32& OutTotal) const
{
    const FObjectPool* Pool = ObjectPools.Find(PoolName);
    if (Pool)
    {
        OutAvailable = Pool->AvailableObjects.Num();
        OutActive = Pool->ActiveObjects.Num();
        OutTotal = OutAvailable + OutActive;
    }
    else
    {
        OutAvailable = 0;
        OutActive = 0;
        OutTotal = 0;
    }
}

void UObjectPoolSubsystem::PrewarmPool(FName PoolName, int32 Count)
{
    FObjectPool* Pool = ObjectPools.Find(PoolName);
    if (!Pool)
    {
        return;
    }
    
    int32 CurrentTotal = Pool->AvailableObjects.Num() + Pool->ActiveObjects.Num();
    int32 ToSpawn = FMath::Min(Count, Pool->MaxPoolSize - CurrentTotal);
    
    for (int32 i = 0; i < ToSpawn; i++)
    {
        AActor* NewObject = SpawnPooledObject(*Pool);
        if (NewObject)
        {
            PrepareObjectForReturn(NewObject);
            Pool->AvailableObjects.Add(NewObject);
            ObjectToPoolMap.Add(NewObject, PoolName);
        }
    }
}

void UObjectPoolSubsystem::ClearPool(FName PoolName)
{
    FObjectPool* Pool = ObjectPools.Find(PoolName);
    if (!Pool)
    {
        return;
    }
    
    // Destroy all objects in the pool
    for (AActor* Object : Pool->AvailableObjects)
    {
        if (IsValid(Object))
        {
            Object->Destroy();
        }
        ObjectToPoolMap.Remove(Object);
    }
    
    for (AActor* Object : Pool->ActiveObjects)
    {
        if (IsValid(Object))
        {
            Object->Destroy();
        }
        ObjectToPoolMap.Remove(Object);
    }
    
    Pool->AvailableObjects.Empty();
    Pool->ActiveObjects.Empty();
}

void UObjectPoolSubsystem::ClearAllPools()
{
    for (auto& PoolPair : ObjectPools)
    {
        ClearPool(PoolPair.Key);
    }
    
    ObjectPools.Empty();
    ObjectToPoolMap.Empty();
}

AActor* UObjectPoolSubsystem::SpawnPooledObject(FObjectPool& Pool)
{
    if (!Pool.ObjectClass)
    {
        return nullptr;
    }
    
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    
    return World->SpawnActor<AActor>(Pool.ObjectClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}

void UObjectPoolSubsystem::PrepareObjectForUse(AActor* PooledObject, const FTransform& SpawnTransform)
{
    if (!PooledObject)
    {
        return;
    }
    
    // Set transform
    PooledObject->SetActorTransform(SpawnTransform);
    
    // Enable the actor
    PooledObject->SetActorHiddenInGame(false);
    PooledObject->SetActorEnableCollision(true);
    PooledObject->SetActorTickEnabled(true);
    
    // Enable all components
    TArray<UActorComponent*> Components = PooledObject->GetComponents().Array();
    for (UActorComponent* Component : Components)
    {
        if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
        {
            SceneComp->SetVisibility(true, true);
        }
        Component->SetActive(true);
        Component->SetComponentTickEnabled(true);
    }
}

void UObjectPoolSubsystem::PrepareObjectForReturn(AActor* PooledObject)
{
    if (!PooledObject)
    {
        return;
    }
    
    // Reset transform
    PooledObject->SetActorLocation(FVector(0, 0, -10000)); // Move far below the world
    
    // Disable the actor
    PooledObject->SetActorHiddenInGame(true);
    PooledObject->SetActorEnableCollision(false);
    PooledObject->SetActorTickEnabled(false);
    
    // Disable all components
    TArray<UActorComponent*> Components = PooledObject->GetComponents().Array();
    for (UActorComponent* Component : Components)
    {
        if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
        {
            SceneComp->SetVisibility(false, true);
        }
        Component->SetActive(false);
        Component->SetComponentTickEnabled(false);
    }
}

void UObjectPoolSubsystem::CheckInactiveObjects()
{
    // This would check for objects that should be returned to the pool
    // For now, this is a placeholder for future implementation
    // Could check things like:
    // - Objects that have been inactive for too long
    // - Objects that are too far from the player
    // - Objects marked for return by other systems
}