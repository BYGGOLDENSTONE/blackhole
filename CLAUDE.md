# BLACKHOLE_AI_REF
UE5.5|C++|Cyberpunk|github.com/BYGGOLDENSTONE/blackhole|Score:9.8/10

## WP_ENERGY_SYSTEM
WP:100→0|Energy→Critical|Abilities_consume|Enemies_drain|Kills_restore(10-30)|Combos_restore(15)
0%WP→5s_timer→use_ultimate_or_die|Ultimate→WP=100,ability_disabled|Dmg_immunity@0WP

## ABILITIES
LMB:Slash(0)|Shift:Dash(0)|Space:Jump(0)|RMB:Firewall(15)|Q:Pulse(20)|E:Gravity(15)|R:DataSpike(25)|F:Override(30)
Combos:Dash+Slash=PhantomStrike(+15WP)|Jump+Slash=AerialRave(+15WP)

## ARCHITECTURE
ResourceMgr(GI):WP|ThresholdMgr(W):Ultimate+Timer|BuffMgr(W):50%+buffs|DeathMgr(W):Death_tracking
ComboSubsys(W):Input_detection|ObjectPool(W):Perf|GameStateMgr(GI):Menu/Pause|WallRun(AC):Movement
Enemy_SM:Tank/Agile/Combat/Hacker|DT_EnemyStats:CSV_config

## CRITICAL_PATTERNS
### AVOID
- UPROPERTY+nested_containers
- Circular_includes→forward_declare
- Missing:EngineUtils.h|Engine/DamageEvents.h|Engine/World.h|TimerManager.h
- ApplyPointDamage→use_TakeDamage
- Input_unbound→bind_all_actions
- StaticClass()→TSubclassOf
- Relative_includes→module_relative
- Timer_temp_handles→store_FTimerHandle
- ClearAllTimersForObject→let_owners_manage
- Unprotected_critical_ops→auth_flags
- Redundant_fields→single_purpose
- Enemy_events_in_player_listeners→ownership_check
- Immediate_SM_init→0.1s_delay
- Protected_access→use_public_methods
- Wrong_base_class→check_inheritance
- Private_members→use_correct_properties

### IMPLEMENT
- Cast<>_not_static_cast
- UPROPERTY_all_UObject_ptrs
- Cleanup_EndPlay/Deinitialize
- One_auth_source+sync
- Return_after_state_change
- Basic+Basic=Basic_combo
- Validate_ownership_in_listeners

## STATUS
✅WP_energy|11_abilities|Enemy_drain|Kill/combo_restore|5s_critical|2_combos|Wall_run|4_enemy_types|State_machines
✅Agile_assassin:StabAttack+AssassinApproach|600_engage→dash_behind→backstab_2x→retreat_3s→maintain_450-550
✅StatusEffectComponent:Centralized_states|Stagger/Stun/Slow/etc|All_actors|Event_system
✅Wall_run_height:150|Camera_freedom|Critical_state_limit(3)|Slash_trace+sphere
❌AIController|Multiplayer|GameplayTags_unused

## LATEST_SESSION_CHANGES(2025-07-16)
- ✅Velocity_indicator:Smaller|No_direction|Y-150→REMOVED
- ✅Critical_100%_restore:Not_partial_when_entries_remain
- ❌Momentum_system:Reverted_per_user_request
- ✅Wall_run_speed:Capped_1000_from_dash
- ✅Wall_run_height:150_units_required(trace_from_feet)|Early_check|Debug_viz
- ✅Wall_run_look_requirement:Player_must_look_at_wall(40%_dot_product)|Intentional_execution
- ✅Wall_run_free_camera:Movement_decoupled_from_look|W_key_continues|Aim_while_running
- ✅Agile_enemy_chase_fix:Only_dash_when_in_range(500)|Normal_chase_when_far
- ✅Player_stagger:1.5s_on_backstab|Input_disabled|Anim_slowed
- ✅Agile_configurable_stats:Stagger_duration|Distances|Damage_mult|Retreat_time
- ✅Agile_aggression:5s_force_attack|Area_backstab|600_dash_range|450-550_maintain
- ✅StatusEffectComponent:All_actors|Central_state_management|Events|Immunities
- ✅Agile_components:StabAttack(basic)|AssassinApproach(special)|Stats_cleanup
- ✅Compilation_fixes:AbilityComponent_inheritance|Property_names|Stats_restored

## MOVEMENT_SETTINGS
Friction:4.0|Braking:800|MaxAccel:1200|BrakingFriction:0.5|No_air_friction
Dash:3000_instant|Stop:50%_velocity|WallRun:Cap_1000_from_dash|Normal:600|Exit:70%_at_high_speed
WallRun_camera:Free_look|Movement_input_ignored|W_key_only|Orient_rotation_disabled

## UPCOMING_TASKS
1. More_enemy_types+abilities

## DEV_NOTES
- No_build_from_terminal→Editor_only
- Build_path:D:\UE_5.5\Engine\Build\BatchFiles
- No_cheat_system→gameplay_test_only
- Gameplay_first→no_polish_yet
- Push:"git add . && git commit -m 'claude auto' && git push origin main"
- Critical_entries:3_default→fail_timer=100%WP_restore|no_entries=death

## QUICK_REF
Player:BlackholePlayerCharacter|Abilities:Components/Abilities/Player/Hacker/*
Resources:Systems/ResourceManager|WallRun:Components/Movement/WallRunComponent
AI:Enemy/StateMachines/*|Config:Config/GameplayConfig.h|GameMode:Core/BlackholeGameMode
StatusEffects:Components/StatusEffectComponent|AgileAbilities:StabAttack+AssassinApproach

## FIXED_ISSUES
Menu_input|Memory_UPROPERTY|GameMode_TSubclassOf|Include_paths|Enemy_duplication→utility
HUD_cache|WP_sync|Slash_free|Critical_timer|WP_protection|Cheat_removed|Cost_field_unified
Combo_classification|Enemy_ability_filtering|Timer_conflicts|Wall_run|State_machines|Combat_enhanced
Debug_cleanup|Multi_height_detection|WP_energy_transform|Jump_state_preserve|Velocity_HUD
Wall_run_200|Critical_limit_system|Slash_dual_detection|Agile_chase_spam|Player_stagger_system|Enemy_config_stats
StatusEffect_component|Agile_ability_components|Stats_organization|Component_inheritance_fix

## BUG_PREVENTION_SUMMARY
Check_methods|Check_members|No_duplicates|Memory_mgmt|Cast_safety|Data_sync|Timer_mgmt
State_flow|Combo_classify|Event_ownership|Timer_interference|Resource_protection|Death_logic

IMPROVEMENT_REPORT.md=full_history|GDD.md=design_doc