// Copyright ProjectLogos

#include "Tag/SyncCombatTags.h"

// Combat abilities.
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Blocking, "Ability.Blocking");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Knockback, "Ability.Knockback");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Knockdown, "Ability.Knockdown");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Pushback, "Ability.Pushback");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Stagger, "Ability.Stagger");
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Stagger_Cosmetic, "Ability.Stagger.Cosmetic");

// Damage types.
UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Physical, "Damage.Type.Physical");
UE_DEFINE_GAMEPLAY_TAG(TAG_Damage_Type_Magical, "Damage.Type.Magical");

// Combat states.
UE_DEFINE_GAMEPLAY_TAG(TAG_State, "State");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action, "State.Action");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action_Blocking, "State.Action.Blocking");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action_Dodging, "State.Action.Dodging");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action_Escaping, "State.Action.Escaping");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action_GettingUp, "State.Action.GettingUp");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Action_Parrying, "State.Action.Parrying");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_CC, "State.CC");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_CC_Rooted, "State.CC.Rooted");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_CC_Stunned, "State.CC.Stunned");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat, "State.Combat");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Combat_Attacking, "State.Combat.Attacking");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Collision, "State.Collision");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Collision_IgnoreBody, "State.Collision.IgnoreBody");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement, "State.Movement");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Movement_Airborne, "State.Movement.Airborne");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction, "State.Reaction");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_Flinch, "State.Reaction.Flinch");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_Knockdown, "State.Reaction.Knockdown");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_Parried, "State.Reaction.Parried");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_Pushback, "State.Reaction.Pushback");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_Stagger, "State.Reaction.Stagger");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_SuperArmor1, "State.Reaction.SuperArmor1");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_SuperArmor2, "State.Reaction.SuperArmor2");
UE_DEFINE_GAMEPLAY_TAG(TAG_State_Reaction_SuperArmor3, "State.Reaction.SuperArmor3");

// Combat reaction triggers.
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger, "Trigger");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Flinch, "Trigger.Flinch");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit, "Trigger.Hit");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_Block_Success, "Trigger.Hit.Block.Success");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_Blocked, "Trigger.Hit.Blocked");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_Dodge_Success, "Trigger.Hit.Dodge.Success");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_Dodged, "Trigger.Hit.Dodged");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_Parried, "Trigger.Hit.Parried");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_Parry_Success, "Trigger.Hit.Parry.Success");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_SuperArmor_Success, "Trigger.Hit.SuperArmor.Success");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Hit_SuperArmored, "Trigger.Hit.SuperArmored");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Knockback, "Trigger.Knockback");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Knockdown, "Trigger.Knockdown");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Pushback, "Trigger.Pushback");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Stagger, "Trigger.Stagger");
UE_DEFINE_GAMEPLAY_TAG(TAG_Trigger_Stun, "Trigger.Stun");

// Combat gameplay cues.
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Hit_CameraShake, "GameplayCue.Hit.CameraShake");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Hit_CameraShake_Heavy, "GameplayCue.Hit.CameraShake.Heavy");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Hit_CameraShake_Medium, "GameplayCue.Hit.CameraShake.Medium");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Hit_CameraShake_Small, "GameplayCue.Hit.CameraShake.Small");
UE_DEFINE_GAMEPLAY_TAG(TAG_GameplayCue_Hit_VFX_Melee, "GameplayCue.Hit.VFX.Melee");

// Hit metadata.
UE_DEFINE_GAMEPLAY_TAG(TAG_Hit_Critical, "Hit.Critical");
