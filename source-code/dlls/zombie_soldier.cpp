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

#define SF_DROP_MEDKIT 128
#define SF_DROP_GRENADE 256


//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define	ZOMBIE_SOLDIER_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_SOLDIER_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_SOLDIER_AE_ATTACK_BOTH		0x03

#define ZOMBIE_SOLDIER_FLINCH_DELAY			2		// at most one flinch every n secs

class CZombieSoldier : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int IgnoreConditions () override;

	float m_flNextFlinch;

	void GibMonster();

	void PainSound() override;
	void AlertSound() override;
	void IdleSound() override;
	void AttackSound();

	void Killed(entvars_t* pevAttacker, int iGib) override;

	static const char *pAttackSounds[];
	static const char *pIdleSounds[];
	static const char *pAlertSounds[];
	static const char *pPainSounds[];
	static const char *pAttackHitSounds[];
	static const char *pAttackMissSounds[];

	// No range attacks
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) override { return FALSE; }
	BOOL CheckRangeAttack2 ( float flDot, float flDist ) override { return FALSE; }
	int TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType ) override;
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

		default:
			CBaseMonster::HandleAnimEvent( pEvent );
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
		pev->body = RANDOM_LONG(0, 4);// pick a body, any body
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

void CZombieSoldier::Killed(entvars_t* pevAttacker, int iGib)
{

	if (pev->spawnflags & SF_DROP_GRENADE)
	{
		DropItem("weapon_handgrenade", Vector(pev->origin - Vector(0, 0, 20)), pev->angles);
	}
	if (pev->spawnflags & SF_DROP_MEDKIT)
	{
		DropItem("item_healthkit", Vector(pev->origin - Vector(0, 0, 20)), pev->angles);
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

