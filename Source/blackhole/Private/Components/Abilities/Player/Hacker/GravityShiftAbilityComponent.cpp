#include "Components/Abilities/Player/Hacker/GravityShiftAbilityComponent.h"
#include "Components/GravityDirectionComponent.h"
#include "GameFramework/Character.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Enemy/BaseEnemy.h"

UGravityShiftAbilityComponent::UGravityShiftAbilityComponent()
{
    // Set default values - NO COST OR COOLDOWN for testing
    WPCost = 0.0f;
    Cooldown = 0.0f;
    Range = 0.0f; // Not used for this ability
    
    // Mark as basic ability to bypass ultimate system
    bIsBasicAbility = true;
}

void UGravityShiftAbilityComponent::BeginPlay()
{
    Super::BeginPlay();
    
    // Get or create gravity component
    if (AActor* Owner = GetOwner())
    {
        GravityComponent = Owner->FindComponentByClass<UGravityDirectionComponent>();
        if (!GravityComponent)
        {
            GravityComponent = NewObject<UGravityDirectionComponent>(Owner, TEXT("GravityDirectionComponent"));
            GravityComponent->RegisterComponent();
        }
    }
}

void UGravityShiftAbilityComponent::Execute()
{
    if (!CanExecute()) return;
    
    // Skip base class Execute to avoid any ultimate system
    
    if (!GravityComponent) return;
    
    FVector NewGravityDirection;
    
    if (bUseLookDirection)
    {
        // Use player's look direction to determine gravity
        NewGravityDirection = GetGravityFromLookDirection();
    }
    else
    {
        // Cycle through predefined axes
        EGravityAxis NextAxis = GetNextGravityAxis();
        GravityComponent->SetGravityAxis(NextAxis);
        return;
    }
    
    // Apply gravity shift
    ApplyAreaGravityShift(NewGravityDirection, ShiftRadius, TransitionDuration);
    
    UE_LOG(LogTemp, Log, TEXT("Gravity Shift: Initiating smooth transition to gravity direction %s over %.1f seconds"), 
        *NewGravityDirection.ToString(), TransitionDuration);
    
    // Visual feedback for gravity shift
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        FVector CharLocation = Character->GetActorLocation();
        
        #if WITH_EDITOR
        // Draw a sphere that expands during the transition
        for (int32 i = 0; i < 5; i++)
        {
            float Delay = i * 0.5f;
            float Radius = 100.0f + (i * 200.0f);
            DrawDebugSphere(GetWorld(), CharLocation, Radius, 24, FColor::Cyan, false, TransitionDuration - Delay, 0, 2.0f);
        }
        
        // Draw gravity direction indicator
        DrawDebugLine(GetWorld(), CharLocation, CharLocation + (NewGravityDirection * 1000.0f), 
            FColor::Magenta, false, TransitionDuration, 0, 10.0f);
        #endif
    }
}

bool UGravityShiftAbilityComponent::CanExecute() const
{
    // Always allow execution for testing
    return true;
}


EGravityAxis UGravityShiftAbilityComponent::GetNextGravityAxis() const
{
    // Cycle through axes
    TArray<EGravityAxis> AxisOrder = {
        EGravityAxis::Default,
        EGravityAxis::Left,
        EGravityAxis::Forward,
        EGravityAxis::Right,
        EGravityAxis::Backward,
        EGravityAxis::Up
    };
    
    CurrentAxisIndex = (CurrentAxisIndex + 1) % AxisOrder.Num();
    return AxisOrder[CurrentAxisIndex];
}

FVector UGravityShiftAbilityComponent::GetGravityFromLookDirection() const
{
    if (!GetOwner()) return FVector(0, 0, -1);
    
    // Get camera direction
    FVector CameraDirection;
    
    if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
    {
        if (UCameraComponent* Camera = Character->FindComponentByClass<UCameraComponent>())
        {
            CameraDirection = Camera->GetForwardVector();
        }
        else
        {
            CameraDirection = Character->GetActorForwardVector();
        }
    }
    
    // Find which surface we're looking at
    FHitResult HitResult;
    FVector TraceStart = GetOwner()->GetActorLocation();
    FVector TraceEnd = TraceStart + (CameraDirection * 5000.0f);
    
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(GetOwner());
    
    if (GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
    {
        // Use the surface normal as the new "up" direction
        // So gravity points opposite to the surface normal
        FVector NewGravity = -HitResult.ImpactNormal;
        
        #if WITH_EDITOR
        // Debug visualization
        DrawDebugLine(GetWorld(), TraceStart, HitResult.ImpactPoint, FColor::Green, false, 2.0f);
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 50.0f, 12, FColor::Green, false, 2.0f);
        DrawDebugLine(GetWorld(), HitResult.ImpactPoint, HitResult.ImpactPoint + (NewGravity * 200.0f), 
            FColor::Blue, false, 2.0f, 0, 5.0f);
        #endif
        
        return NewGravity;
    }
    
    // Default to current gravity if no surface hit
    if (GravityComponent)
    {
        return GravityComponent->GetGravityDirection();
    }
    
    return FVector(0, 0, -1);
}

void UGravityShiftAbilityComponent::ApplyAreaGravityShift(FVector NewGravityDirection, float Radius, float Duration)
{
    if (!GravityComponent) return;
    
    // Apply to player
    GravityComponent->TransitionToGravityDirection(NewGravityDirection, Duration);
    
    // Apply to all characters in radius
    FVector Center = GetOwner()->GetActorLocation();
    
    TArray<AActor*> FoundActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), FoundActors);
    
    int32 AffectedEnemies = 0;
    int32 AffectedTotal = 0;
    
    for (AActor* Actor : FoundActors)
    {
        if (!Actor || Actor == GetOwner()) continue;
        
        float Distance = FVector::Dist(Center, Actor->GetActorLocation());
        if (Distance <= Radius)
        {
            // Create gravity component if needed
            UGravityDirectionComponent* CharGravity = Actor->FindComponentByClass<UGravityDirectionComponent>();
            if (!CharGravity)
            {
                CharGravity = NewObject<UGravityDirectionComponent>(Actor, UGravityDirectionComponent::StaticClass(), TEXT("GravityDirectionComponent"));
                CharGravity->RegisterComponent();
            }
            
            // Apply the gravity transition
            CharGravity->TransitionToGravityDirection(NewGravityDirection, Duration);
            AffectedTotal++;
            
            // Check if it's an enemy
            if (Actor->ActorHasTag(FName("Enemy")))
            {
                AffectedEnemies++;
                UE_LOG(LogTemp, Log, TEXT("Applied gravity shift to enemy: %s at distance %.1f"), *Actor->GetName(), Distance);
            }
        }
    }
    
    UE_LOG(LogTemp, Log, TEXT("Gravity shift affected %d total characters (%d enemies) within radius %.1f"), 
        AffectedTotal, AffectedEnemies, Radius);
    
    #if WITH_EDITOR
    // Debug visualization - pulsing sphere
    for (int32 i = 0; i < 3; i++)
    {
        float Delay = i * (Duration / 3.0f);
        DrawDebugSphere(GetWorld(), Center, Radius - (i * 100.0f), 32, FColor::Blue, false, Duration - Delay, 0, 3.0f);
    }
    #endif
}