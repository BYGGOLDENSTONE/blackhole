#pragma once

#include "CoreMinimal.h"

/**
 * Central configuration for all gameplay-related magic numbers.
 * This keeps all tweakable values in one place for easier balancing.
 */
namespace GameplayConfig
{
	// Enemy AI Configuration
	namespace Enemy
	{
		constexpr float DETECTION_RANGE = 2500.0f;				// Units
		constexpr float AI_UPDATE_RATE = 0.2f;					// Seconds
		constexpr float SIGHT_HEIGHT_OFFSET = 50.0f;			// Units
		constexpr float DEATH_IMPULSE_Z = 0.5f;				// Normalized
		constexpr float DEATH_IMPULSE_MAGNITUDE = 5000.0f;		// Force units
		constexpr float CORPSE_LIFESPAN = 10.0f;				// Seconds
		constexpr float COMBAT_MESSAGE_DURATION = 3.0f;		// Seconds
	}

	// Resource System Configuration
	namespace Resources
	{
		constexpr float MAX_WILLPOWER = 100.0f;				// WP units
		constexpr float WP_WARNING_PERCENT = 0.9f;				// 90%
		constexpr float WP_BUFFED_THRESHOLD = 0.5f;			// 50%
	}

	// Player Movement Configuration
	namespace Movement
	{
		constexpr float CAPSULE_RADIUS = 42.0f;				// Units
		constexpr float CAPSULE_HEIGHT = 96.0f;				// Units
		constexpr float ROTATION_RATE = 540.0f;				// Degrees/second
		constexpr float BASE_JUMP_VELOCITY = 600.0f;			// Units/second
		constexpr float AIR_CONTROL = 0.2f;					// 0-1 range
		constexpr float CAMERA_ARM_LENGTH = 400.0f;			// Units
		constexpr float FIRST_PERSON_OFFSET = 70.0f;			// Units
		
		// First person camera adjustments
		constexpr float FP_CAMERA_FORWARD = 15.0f;				// Units
		constexpr float FP_CAMERA_UP = 5.0f;					// Units
		constexpr float FP_CAMERA_PITCH = -10.0f;				// Degrees
		constexpr float FP_CAMERA_YAW = 90.0f;					// Degrees
		constexpr float FP_FALLBACK_HEIGHT = 160.0f;			// Units
	}

	// General Ability Defaults
	namespace Abilities
	{
		namespace Defaults
		{
			constexpr float COOLDOWN = 1.0f;					// Seconds
			constexpr float RANGE = 1000.0f;					// Units
		}

		// Pulse Hack (Hacker)
		namespace PulseHack
		{
			constexpr float WP_COST = 10.0f;					// WP units
			constexpr float STAMINA_COST = 5.0f;				// Stamina units
			constexpr float COOLDOWN = 8.0f;					// Seconds
			constexpr float RADIUS = 500.0f;					// Units
			constexpr float SLOW_DURATION = 3.0f;				// Seconds
			constexpr float SLOW_AMOUNT = 0.3f;				// 70% slow (1.0 - 0.3)
			constexpr float WP_REFUND_PER_ENEMY = 3.0f;		// WP units
			constexpr float MAX_WP_REFUND = 15.0f;				// WP units
			
			// Ultimate mode
			constexpr float ULTIMATE_RADIUS_MULT = 4.0f;		// Multiplier
			constexpr float ULTIMATE_STUN_DURATION = 3.0f;		// Seconds
			constexpr float ULTIMATE_WP_CLEANSE = 50.0f;		// WP units
		}

		// Hacker Jump
		namespace HackerJump
		{
			constexpr float STAMINA_COST = 10.0f;				// Stamina units
			constexpr float JUMP_VELOCITY = 1200.0f;			// Units/second
			constexpr float AIR_CONTROL_BOOST = 2.0f;			// Multiplier
			constexpr int32 MAX_JUMPS = 2;						// Count (double jump)
			constexpr float JUMP_COOLDOWN = 0.5f;				// Seconds between jumps
		}

		// Data Spike (Hacker)
		namespace DataSpike
		{
			constexpr float STAMINA_COST = 12.0f;				// Stamina units
			constexpr float WP_COST = 18.0f;					// WP units
			constexpr float COOLDOWN = 6.0f;					// Seconds
			constexpr float DAMAGE = 30.0f;						// HP damage
			constexpr float RANGE = 1500.0f;					// Units
			constexpr float PROJECTILE_SPEED = 2000.0f;		// Units/second
			constexpr float DOT_DAMAGE = 5.0f;					// HP damage per tick
			constexpr float DOT_DURATION = 4.0f;				// Seconds
			constexpr float DOT_TICK_RATE = 0.5f;				// Seconds between ticks
			constexpr int32 PIERCE_COUNT = 3;					// Number of enemies to pierce
			
			// Ultimate mode
			constexpr float ULTIMATE_DAMAGE_MULT = 2.0f;		// Multiplier
			constexpr float ULTIMATE_DOT_MULT = 3.0f;			// Multiplier
			constexpr int32 ULTIMATE_PIERCE_COUNT = 10;		// Pierce all enemies
			constexpr float ULTIMATE_WP_CLEANSE = 25.0f;		// WP units
		}

		// System Override (Hacker)
		namespace SystemOverride
		{
			constexpr float STAMINA_COST = 30.0f;				// Stamina units
			constexpr float WP_COST = 40.0f;					// WP units
			constexpr float COOLDOWN = 15.0f;					// Seconds
			constexpr float DISABLE_DURATION = 3.0f;			// Seconds
			constexpr float RADIUS = 2000.0f;					// Units
			constexpr float WP_CLEANSE = 30.0f;				// WP units
			constexpr float DAMAGE = 50.0f;					// HP damage to all enemies
		}
	}

	// Attribute System Configuration
	namespace Attributes
	{
		constexpr float DEFAULT_MAX_VALUE = 100.0f;			// Default for all attributes
		constexpr float DEFAULT_REGEN_RATE = 0.0f;				// Per second
		
		// Stamina specific
		namespace Stamina
		{
			constexpr float MAX_VALUE = 100.0f;				// Stamina units
			constexpr float REGEN_RATE = 10.0f;				// Per second
		}
		
		// Integrity (Health) specific
		namespace Integrity
		{
			constexpr float MAX_VALUE = 100.0f;				// HP units
			constexpr float BLOCK_DAMAGE_REDUCTION = 0.5f;		// 50% reduction when blocking
		}
	}

	// Threshold System Configuration
	namespace Thresholds
	{
		constexpr int32 MAX_DISABLED_ABILITIES = 3;			// Before death
		constexpr int32 MAX_WP_REACHES = 4;					// Times WP can hit 100%
		
		// Buffs at 50%+ WP
		constexpr float BUFFED_DAMAGE_MULT = 1.2f;				// 20% increase
		constexpr float BUFFED_COOLDOWN_REDUCTION = 0.15f;		// 15% faster
		constexpr float BUFFED_ATTACK_SPEED = 1.25f;			// 25% faster
		
		// Per lost ability scaling
		constexpr float DAMAGE_PER_LOST_ABILITY = 0.1f;		// 10% per ability
		constexpr float CDR_PER_LOST_ABILITY = 0.05f;			// 5% per ability
		constexpr float SPEED_PER_LOST_ABILITY = 0.1f;			// 10% per ability
	}

	// HUD and UI Configuration
	namespace UI
	{
		constexpr float DEFAULT_BAR_WIDTH = 200.0f;			// Pixels
		constexpr float DEFAULT_BAR_HEIGHT = 20.0f;			// Pixels
		constexpr float COOLDOWN_ICON_SIZE = 64.0f;			// Pixels
	}
}