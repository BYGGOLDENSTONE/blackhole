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
✅WP_energy|11_abilities|Enemy_drain|Kill/combo_restore|5s_critical|2_combos|Wall_run|8_enemy_types|State_machines
✅Agile_assassin:StabAttack(0.5s_stagger)+AssassinApproach|600_engage→dash_behind→backstab_2x→retreat_6s→maintain_450-550
✅Tank_enemy:HeatAura(5WP/s)+Charge(300-1500)+GroundSlam|Heavy_slow|Heat_affects_all
✅Standard_enemy:SwordAttack+Builder|Build_after_4s_chase|Psi-disruptor_construction(20s)|Disables_movement_abilities
✅MindMelder_enemy:PowerfulMindmeld(30s_cast→0WP)|3000_range|Interrupt_only_by_death|Location_warning
✅StatusEffectComponent:Centralized_states|Stagger/Stun/Slow/etc|All_actors|Event_system
✅Wall_run_height:150|Camera_freedom|Critical_state_limit(3)|Slash_trace+sphere
❌AIController|Multiplayer|GameplayTags_unused

## LATEST_SESSION_CHANGES(2025-07-17)
- ✅StatusEffectComponent_improvements:Source_tracking|Priority_system|Stacking_rules
- ✅AbilityComponent:Added_CanAct()_check_before_execution
- ✅Movement_system:Added_CanMove()_checks_to_Move/Jump/Dash
- ✅Effect_priorities:Higher_priority_effects_override_lower
- ✅Stacking_rules:Per-effect_type(Refresh/Replace/Stack/Extend/Ignore)
- ✅Source_cleanup:RemoveEffectsFromSource()_for_proper_cleanup
- ✅StandardEnemy_building:Build_PsiDisruptor_after_6s_without_hitting_player
- ✅PsiDisruptor_invulnerable:Only_destroyable_by_GravityPull_ultimate(Singularity)
- ✅GravityPull_ultimate:Detects_and_destroys_PsiDisruptors|Added_object_types_for_detection
- ✅Builder_interruption:Building_cancels_if_not_enough_builders_alive
- ✅MindMelder_fix:PowerfulMindmeld_executes_properly|Reduced_cooldown_45s|Fixed_LoS_check
- ✅PowerfulMindmeld_completion:Uses_SetCurrentValue(0)_to_drop_WP|Only_interrupted_by_death
- ✅PowerfulMindmeld_endplay:Added_EndPlay_to_interrupt_on_MindMelder_death
- ✅HUD_timer_positions:Moved_from_center_to_top_right_corner
- ✅HUD_MindMeld_text:Updated_to_"Kill_MindMelder"_not_"Get_close"
- ✅Builder_hit_tracking:OnSwordHit_delegate_tracks_actual_hits_not_attempts
- ✅Builder_spawn_debug:Added_logging_for_PsiDisruptor_spawn_failures
- ✅StandardChaseState:Created_new_state_for_4s_chase_timeout_building
- ✅StandardEnemy_building_moved:From_combat_to_chase_state|4s_timeout
- ✅PowerfulMindmeld_debug:Added_timer_verification|Progress_logging
- ✅PowerfulMindmeld_WP_fix:Use_ResourceManager|Triggers_critical_timer_properly
- ✅PowerfulMindmeld_compile_fix:Renamed_local_vars_to_avoid_shadowing_members

## LATEST_SESSION_CHANGES(2025-07-17)
- ✅AutomaticDoor_system:Proximity_detection|Look_at_requirement|Auto_close_on_exit
- ✅Door_opens_up:When_player_close_AND_looking|Smooth_interpolation
- ✅Inside_detection:Separate_trigger_volume|Immediate_close_on_exit
- ✅Visual_feedback:Debug_boxes|Player_look_direction|Door_states
- ✅PsiDisruptor_building:Visual_sphere|Timer_doubling_on_death|Pause_not_cancel
- ✅Builder_behavior:Stay_in_sphere|No_chase/attack|Building_state
- ✅StandardEnemy_fixes:GetController_cast|Building_state_integration
- ✅Walk_Run_System:W_key_walk_300|Double_tap_W_run_600|Toggle_mode
- ✅Double_tap_detection:1_second_window|Toggle_between_walk_run
- ✅Movement_speed_states:Walk_300_default|Run_600_on_double_tap

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
- ✅New_enemies:Tank+HeatAura+Charge|Standard+Builder|MindMelder+30s_kill
- ✅Agile_updates:6s_retreat|0.5s_stab_stagger|Assassin_pattern_refined
- ✅Psi-disruptor:Builder_coordination|20s_build|Disables_dash/jump/wallrun
- ✅Tank_abilities:5WP/s_heat_aura|Charge_1200speed|Knockback_on_impact
- ✅Enemy_death_fix:Clear_timers|Stop_line_of_sight|Proper_cleanup
- ✅New_state_machines:StandardEnemy+MindMelder|Combat_states|Proper_AI

## MOVEMENT_SETTINGS
Friction:4.0|Braking:800|MaxAccel:1200|BrakingFriction:0.5|No_air_friction
Dash:3000_instant|Stop:50%_velocity|WallRun:Cap_1000_from_dash|Normal:600|Exit:70%_at_high_speed
WallRun_camera:Free_look|Movement_input_ignored|W_key_only|Orient_rotation_disabled

## UPCOMING_TASKS
1. Visual_effects_for_abilities+status_effects
2. UI_notifications_for_mindmeld+psi-disruptor
3. Boss_encounters
4. Additional_combos(Tempest_Blade|Blade_Dance)

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
Enemies:Agile/Tank/Standard/MindMelder|BuilderComp:Psi-disruptor|HeatAura+Charge:Tank
PsiDisruptor:Actors/PsiDisruptor|PowerfulMindmeld:30s_instant_kill
AutomaticDoor:Actors/AutomaticDoor|Proximity+Look_detection|Auto_close_on_exit

## FIXED_ISSUES
Menu_input|Memory_UPROPERTY|GameMode_TSubclassOf|Include_paths|Enemy_duplication→utility
HUD_cache|WP_sync|Slash_free|Critical_timer|WP_protection|Cheat_removed|Cost_field_unified
Combo_classification|Enemy_ability_filtering|Timer_conflicts|Wall_run|State_machines|Combat_enhanced
Debug_cleanup|Multi_height_detection|WP_energy_transform|Jump_state_preserve|Velocity_HUD
Wall_run_200|Critical_limit_system|Slash_dual_detection|Agile_chase_spam|Player_stagger_system|Enemy_config_stats
StatusEffect_component|Agile_ability_components|Stats_organization|Component_inheritance_fix
Enemy_death_cleanup|New_enemy_state_machines|Compilation_errors|API_method_updates

## BUG_PREVENTION_SUMMARY
Check_methods|Check_members|No_duplicates|Memory_mgmt|Cast_safety|Data_sync|Timer_mgmt
State_flow|Combo_classify|Event_ownership|Timer_interference|Resource_protection|Death_logic

IMPROVEMENT_REPORT.md=full_history|GDD.md=design_doc