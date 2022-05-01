/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
//=========================================================
// Zombie
//=========================================================

// UNDONE: Don't flinch every time you get hit

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"weapons.h"

namespace ZecuBodygroup
{
	enum ZecuBodygroup
	{
		Body = 0,
		Crab,
		Grenade
	};
}

namespace Body
{
	enum Body
	{
		grunt = 0,
		saw,
		rpg,
		engi,
		medic,
		massn
	};
}

namespace Crab
{
	enum Crab
	{
		gasmask_crab = 0,
		gasmask_exposed,
		soldier_crab,
		soldier_exposed,
		massn_crab,
		massn_exposed,
	};
}

namespace Grenade
{
	enum Grenade
	{
		none = 0,
		armed
	};
}



//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_SOLDIER_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_SOLDIER_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_SOLDIER_AE_ATTACK_BOTH		0x03
#define	ZOMBIE_SOLDIER_AE_PULL_GRENADE		0x04

#define ZOMBIE_SOLDIER_FLINCH_DELAY			2		// at most one flinch every n secs

#define ZOMBIE_CRAB          "monster_headcrab" // headcrab jumps from zombie

class CZombieSoldier : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int IgnoreConditions () override;
	void SpawnCrab(); // headcrab jumps from zombie

	float m_flNextFlinch;
	float m_flExplodeTime = -1;//time when we explode
	float flHeadDmg;

	void GibMonster();

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();

	void Killed(entvars_t* pevAttacker, int iGib) override;
	BOOL CheckMeleeAttack1(float flDot, float flDist) override;
	BOOL CheckMeleeAttack2(float flDot, float flDist) override;

	Schedule_t* GetScheduleOfType(int Type);
	Schedule_t* GetSchedule(void);

	void SetActivity(Activity NewActivity) override;

	void PrescheduleThink() override;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	CUSTOM_SCHEDULES;

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) override { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) override { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
};

LINK_ENTITY_TO_CLASS( monster_zombie_soldier, CZombieSoldier );

const char *CZombieSoldier::pAttackHitSounds[] = 
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char *CZombieSoldier::pAttackMissSounds[] = 
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char *CZombieSoldier::pAttackSounds[] = 
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav",
};

const char *CZombieSoldier::pIdleSounds[] = 
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav",
};

const char *CZombieSoldier::pAlertSounds[] = 
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char *CZombieSoldier::pPainSounds[] = 
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav",
};

const GibLimit ZombGibLimits[] =
{
	{ 1 },
	{ 1 },
};
const GibData ZombGibs = { "models/gibs/gibs_zombie.mdl", 0, 2, ZombGibLimits };
void CZombieSoldier::GibMonster()
{
	CGib::SpawnRandomGibs(pev, 2, ZombGibs);

	CBaseMonster::GibMonster();
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CZombieSoldier :: Classify ()
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MONSTER;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CZombieSoldier :: SetYawSpeed ()
{
	int ys;

	ys = 120;

#if 0
	switch ( m_Activity )
	{
	}
#endif

	pev->yaw_speed = ys;
}

int CZombieSoldier :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	// Take 30% damage from bullets
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	// HACK HACK -- until we fix this.
	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombieSoldier :: PainSound()
{
	int pitch = 95 + RANDOM_LONG(0,9);

	if (RANDOM_LONG(0,5) < 2)
		EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pPainSounds[ RANDOM_LONG(0,ARRAYSIZE(pPainSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombieSoldier :: AlertSound()
{
	int pitch = 95 + RANDOM_LONG(0,9);

	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAlertSounds[ RANDOM_LONG(0,ARRAYSIZE(pAlertSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombieSoldier :: IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5,5);

	// Play a random idle sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pIdleSounds[ RANDOM_LONG(0,ARRAYSIZE(pIdleSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}

void CZombieSoldier :: AttackSound()
{
	int pitch = 100 + RANDOM_LONG(-5,5);

	// Play a random attack sound
	EMIT_SOUND_DYN ( ENT(pev), CHAN_VOICE, pAttackSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackSounds)-1) ], 1.0, ATTN_NORM, 0, pitch );
}


//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CZombieSoldier :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
		case ZOMBIE_SOLDIER_AE_ATTACK_RIGHT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash right!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieSoldierDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
				}
				// Play a random attack hit sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else // Play a random attack miss sound
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_SOLDIER_AE_ATTACK_LEFT:
		{
			// do stuff for this event.
	//		ALERT( at_console, "Slash left!\n" );
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieSoldierDmgOneSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_SOLDIER_AE_ATTACK_BOTH:
		{
			// do stuff for this event.
			CBaseEntity *pHurt = CheckTraceHullAttack( 70, gSkillData.zombieSoldierDmgBothSlash, DMG_SLASH );
			if ( pHurt )
			{
				if ( pHurt->pev->flags & (FL_MONSTER|FL_CLIENT) )
				{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
				}
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackHitSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackHitSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );
			}
			else
				EMIT_SOUND_DYN ( ENT(pev), CHAN_WEAPON, pAttackMissSounds[ RANDOM_LONG(0,ARRAYSIZE(pAttackMissSounds)-1) ], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5,5) );

			if (RANDOM_LONG(0,1))
				AttackSound();
		}
		break;

		case ZOMBIE_SOLDIER_AE_PULL_GRENADE:
		{
			SetBodygroup(ZecuBodygroup::Grenade, Grenade::armed);

			m_flExplodeTime = gpGlobals->time + 2.5f;
		}

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
			break;
	}
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CZombieSoldier::CheckMeleeAttack1(float flDot, float flDist)
{
	if (GetBodygroup(ZecuBodygroup::Body) == 4)
	{
		if (flDist <= 80 && flDot >= 0.7)
			return TRUE;
	}
	else
	{
		if (flDist <= 160 && flDot >= 0.7)
			return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack2
//=========================================================
BOOL CZombieSoldier::CheckMeleeAttack2(float flDot, float flDist)
{
	if (GetBodygroup(ZecuBodygroup::Grenade) == Grenade::armed)
		return FALSE;
	else CBaseMonster::CheckMeleeAttack2(flDot, flDist);
}

void CZombieSoldier::PrescheduleThink()
{
	if (GetBodygroup(ZecuBodygroup::Grenade) == Grenade::armed)
	{
		if ((gpGlobals->time >= m_flExplodeTime))
		{
			CGrenade::ShootTimed(pev, pev->origin + pev->view_ofs, g_vecZero, 0.01);
		}

		if (m_IdealActivity == ACT_WALK)
			m_IdealActivity = ACT_RUN;
	}

}

//=========================================================
// Spawn Headcrab - headcrab jumps from zombie
//=========================================================
void CZombieSoldier::SpawnCrab()
{
	MAKE_VECTORS(pev->angles);
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36;
	CBaseEntity* pCrab = CBaseEntity::Create(ZOMBIE_CRAB, vecSrc, pev->angles, edict()); // create the crab

	// setup 
	pCrab->pev->health = gSkillData.headcrabHealth - flHeadDmg;
	pCrab->pev->spawnflags |= SF_MONSTER_FALL_TO_GROUND; // make the crab fall

	// remove crab zombie
	switch (GetBodygroup(ZecuBodygroup::Crab))
	{
	case Crab::gasmask_crab:
		SetBodygroup(ZecuBodygroup::Crab, Crab::gasmask_exposed);
		break;
	case Crab::massn_crab:
		SetBodygroup(ZecuBodygroup::Crab, Crab::massn_exposed);
		break;
	default:
		SetBodygroup(ZecuBodygroup::Crab, Crab::soldier_exposed);
		break;
	}
	
}

//=========================================================
// Spawn
//=========================================================
void CZombieSoldier :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/zombie_soldier.mdl");
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_GREEN;
	pev->health			= gSkillData.zombieSoldierHealth;
	pev->view_ofs		= VEC_VIEW;// position of the eyes relative to monster's origin.
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;
	m_afCapability		= bits_CAP_DOORS_GROUP;

	if (pev->body == -1)
	{// -1 chooses a random body
		pev->body = RANDOM_LONG(0, 5);// pick a body, any body
	}

	switch (pev->body)
	{
	case 0:
		SetBodygroup(ZecuBodygroup::Crab, Crab::gasmask_crab); // gasmask for the "grunt" body
		break;
	case 5:
		SetBodygroup(ZecuBodygroup::Crab, Crab::massn_crab); // black cloth for the "Black Ops" body
		break;
	default:
		SetBodygroup(ZecuBodygroup::Crab, Crab::soldier_crab);
		break;
	}

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CZombieSoldier :: Precache()
{
	int i;

	PRECACHE_MODEL("models/zombie_soldier.mdl");
	PRECACHE_MODEL("models/gibs/gibs_zombie.mdl");
	UTIL_PrecacheOther(ZOMBIE_CRAB);
	UTIL_PrecacheOther("item_healthkit");

	for ( i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackHitSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackMissSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND((char *)pAttackSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND((char *)pIdleSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND((char *)pAlertSounds[i]);

	for ( i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND((char *)pPainSounds[i]);
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
enum
{
	SCHED_ZECU_PULL_GRENADE = LAST_COMMON_SCHEDULE + 1,
};
//=========================================================
// PullGrenade
//=========================================================

Task_t	tlPullGrenade[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_ARM	},
};

Schedule_t	slPullGrenade[] =
{
	{
		tlPullGrenade,
		ARRAYSIZE(tlPullGrenade),
		0,
		0,
		"Pull Grenade"
	},
};

DEFINE_CUSTOM_SCHEDULES(CZombieSoldier)
{
	slPullGrenade
};

IMPLEMENT_CUSTOM_SCHEDULES(CZombieSoldier, CBaseMonster);

//=========================================================
// Any custom shedules?
//=========================================================
Schedule_t* CZombieSoldier::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_ZECU_PULL_GRENADE:
		return slPullGrenade;
		break;

	}
	return CBaseMonster::GetScheduleOfType(Type);

}

//=========================================================
// Load up the schedules so ai isn't dumb
//=========================================================
Schedule_t* CZombieSoldier::GetSchedule(void)
{
	// Call another switch class, to check the monster's attitude
	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_COMBAT:
	{
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK2) && GetBodygroup(ZecuBodygroup::Grenade) == Grenade::armed)
			ClearConditions(bits_COND_CAN_MELEE_ATTACK2);


		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			if (GetBodygroup(ZecuBodygroup::Body) == 4)
				return GetScheduleOfType(SCHED_MELEE_ATTACK1);
			if (GetBodygroup(ZecuBodygroup::Grenade) != Grenade::armed)
				return GetScheduleOfType(SCHED_ZECU_PULL_GRENADE);
			if (GetBodygroup(ZecuBodygroup::Grenade) == Grenade::armed)
				return GetScheduleOfType(SCHED_CHASE_ENEMY);
		}
	}
	break;
	}
	return CBaseMonster::GetSchedule();
}

//=========================================================
// SetActivity 
//=========================================================
void CZombieSoldier::SetActivity(Activity NewActivity)
{
	int	iSequence = -1;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_WALK:
		if (GetBodygroup(ZecuBodygroup::Grenade) == Grenade::armed)
		{
			NewActivity = ACT_RUN;
		}
		iSequence = LookupActivity(NewActivity);
		break;
	case ACT_RUN:
		if (GetBodygroup(ZecuBodygroup::Grenade) != Grenade::armed)
		{
			NewActivity = ACT_WALK;
		}
		iSequence = LookupSequence("gren_run");
		break;
	default:
		iSequence = LookupActivity(NewActivity);
		break;
	}

	m_Activity = NewActivity; // Go ahead and set this so it doesn't keep trying when the anim is not present

	// Set to the desired anim, or default anim if the desired is not present
	if (iSequence > -1)
	{
		if (pev->sequence != iSequence || !m_fSequenceLoops)
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;	// Set to the reset anim (if it's there)
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		// Not available try to get default anim
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
}


int CZombieSoldier::IgnoreConditions ()
{
	int iIgnore = CBaseMonster::IgnoreConditions();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
	{
#if 0
		if (pev->health < 20)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
		else
#endif			
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE|bits_COND_HEAVY_DAMAGE);
	}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
	{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_SOLDIER_FLINCH_DELAY;
	}

	return iIgnore;
	
}

//=========================================================
// Hitbox handling - store total head damage 
//=========================================================
void CZombieSoldier::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_HEAD:
	{
		flHeadDmg += flDamage;
		break;
	}
	}

	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CZombieSoldier::Killed(entvars_t* pevAttacker, int iGib)
{
	if (GetBodygroup(ZecuBodygroup::Body) == 4)
		CBaseEntity::Create("item_healthkit", pev->origin, pev->angles);
	else
		CBaseEntity::Create("weapon_handgrenade", pev->origin, pev->angles);

	if (GetBodygroup(ZecuBodygroup::Crab) == 0 || GetBodygroup(ZecuBodygroup::Crab) == 2 || GetBodygroup(ZecuBodygroup::Crab) == 4)
	{
		if (flHeadDmg < gSkillData.headcrabHealth)
		{
			SpawnCrab();
		}
	}

	CBaseMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// DEAD HGRUNT ZOMBIE PROP
//=========================================================
class CDeadZombieSoldier : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_ALIEN_MONSTER; }

	void GibMonster();

	void KeyValue( KeyValueData *pkvd ) override;

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[ 2 ];
};

char *CDeadZombieSoldier::m_szPoses[] = { "dead_on_stomach", "dead_on_back" };

void CDeadZombieSoldier::KeyValue( KeyValueData *pkvd )
{
	if( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_zombie_soldier_dead, CDeadZombieSoldier );

//=========================================================
// ********** DeadZombieSoldier SPAWN **********
//=========================================================
void CDeadZombieSoldier::Spawn()
{
	PRECACHE_MODEL( "models/zombie_soldier.mdl" );
	SET_MODEL( ENT( pev ), "models/zombie_soldier.mdl" );

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[ m_iPose ] );

	if( pev->sequence == -1 )
	{
		ALERT( at_console, "Dead hgrunt with bad pose\n" );
	}

	if (pev->body == -1)
	{// -1 chooses a random body
		pev->body = RANDOM_LONG(0, 4);// pick a body, any body
	}

	// Corpses have less health
	pev->health = 8;

	MonsterInitDead();
}

void CDeadZombieSoldier::GibMonster()
{
	CGib::SpawnRandomGibs(pev, 2, ZombGibs);

	CBaseMonster::GibMonster();
}

