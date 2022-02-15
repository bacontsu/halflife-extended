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
* -----------------------------------------------------------------------------------------------------------------
* Half-Life Extended code by tear (blsha), Copyright 2021. Feel free to use/modify this as long as credit provided
* -----------------------------------------------------------------------------------------------------------------
* 
****/
//=========================================================
// monster template
//=========================================================
// UNDONE: Holster weapon?

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"weapons.h"
#include	"soundent.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is otis dying for scripted sequences?
#define		OTIS_AE_DRAW		( 2 )
#define		OTIS_AE_SHOOT		( 3 )
#define		OTIS_AE_HOLSTER		( 4 )
#define		OTIS_AE_RELOAD		( 5 )
#define		OTIS_AE_MELEE		( 10 )

#define OTIS_GLOCK		5// it's 5 so gearbox Otises will remain with their Deagles (which used to be 0) 
#define OTIS_357		1
#define OTIS_DEAGLE 	2
#define OTIS_NOGUN		3
#define OTIS_DONUT		4
#define OTIS_MP5		6
#define OTIS_SHOTGUN	7

#define SF_DONTDROPGUN (1024)

namespace OtisBodyGroup
{
enum OtisBodyGroup
{
	Arms = 0,
	Gun,
	Vest,
	Helmet,
	Glasses
};
}

namespace OtisWeapon
{
enum OtisWeapon
{

	Random = -1,
	Glock_holstered = 0,
	Glock_drawn,
	Python_holstered,
	Python_drawn,
	Deagle_holstered,
	Deagle_drawn,
	Donut,
	MP5_holstered,
	MP5_drawn,
	Shotgun_holstered,
	Shotgun_drawn,
	None
};
}

namespace OtisVest
{
	enum OtisVest
	{
		Random = -1,
		No_vest = 0,
		No_vest_pants,
		vest_jeans,
		vest_pants,
		No_vest_d,
		No_vest_pants_d,
		vest_jeans_d,
		vest_pants_d,
	};
}

namespace OtisHelm
{
enum OtisHelm
{
	Random = -1,
	No_Helmet = 0,
	Helmet,
	Helmet_glass,
	Cap,
	Beret,
};
}

namespace OtisArms
{
	enum OtisArms
	{
		Random = -1,
		Default = 0,
		RolledUp,
		Default_d,
		RolledUp_d,
	};
}

namespace OtisGlasses
{
	enum OtisGlasses
	{
		Random = -1,
		None = 0,
		Cool,
		Uncool
	};
}

class COtis : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  ISoundMask() override;

	void OtisFireDeagle();
	void OtisFireGlock();
	void OtisFire357();
	void OtisFireShotgun();
	void OtisFireMP5();

	void AlertSound() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;

	void SetActivity(Activity NewActivity);
	
	void RunTask( Task_t *pTask ) override;
	void StartTask( Task_t *pTask ) override;
	int	ObjectCaps() override { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	BOOL CheckRangeAttack1 ( float flDot, float flDist ) override;
	
	void DeclineFollowing() override;

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type ) override;
	Schedule_t *GetSchedule () override;
	MONSTERSTATE GetIdealState () override;

	void DeathSound() override;
	void PainSound() override;
	
	void TalkInit();

	void TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType) override;
	void Killed( entvars_t *pevAttacker, int iGib ) override;

	void KeyValue( KeyValueData* pkvd ) override;
	
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	//These were originally used to store off the setting AND track state,
	//but state is now tracked by calling GetBodygroup
	int m_iOtisGun;
	int m_iOtisHead;
	int m_iOtisArms;
	int m_iOtisVest;
	int m_iOtisGlasses;

	int m_iClipSize;

	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS( monster_otis, COtis );

TYPEDESCRIPTION	COtis::m_SaveData[] = 
{
	DEFINE_FIELD( COtis, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( COtis, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( COtis, m_flPlayerDamage, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COtis, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

//=========================================================
// Monster-specific Schedule Types
//=========================================================
enum
{
	SCHED_OTIS_DISARM = LAST_TALKMONSTER_SCHEDULE + 1,
};

Task_t	tlOtFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slOtFollow[] =
{
	{
		tlOtFollow,
		ARRAYSIZE ( tlOtFollow ),
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// OtisDraw- much better looking draw schedule for when
// otis knows who he's gonna attack.
//=========================================================
Task_t	tlOtisEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float) ACT_ARM },
};

Schedule_t slOtisEnemyDraw[] = 
{
	{
		tlOtisEnemyDraw,
		ARRAYSIZE ( tlOtisEnemyDraw ),
		0,
		0,
		"Otis Enemy Draw"
	}
};

Task_t	tlOtFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slOtFaceTarget[] =
{
	{
		tlOtFaceTarget,
		ARRAYSIZE ( tlOtFaceTarget ),
		bits_COND_CLIENT_PUSH	|
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleOtStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleOtStand[] =
{
	{ 
		tlIdleOtStand,
		ARRAYSIZE ( tlIdleOtStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

Task_t	tlOtisDisarm[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE,	(float)ACT_DISARM },
};

Schedule_t slOtisDisarm[] =
{
	{
		tlOtisDisarm,
		ARRAYSIZE(tlOtisDisarm),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER,
		"Otis Disarm"
	}
};

DEFINE_CUSTOM_SCHEDULES( COtis )
{
	slOtFollow,
	slOtisEnemyDraw,
	slOtFaceTarget,
	slIdleOtStand,
	slOtisDisarm,
};


IMPLEMENT_CUSTOM_SCHEDULES( COtis, CTalkMonster );

void COtis :: StartTask( Task_t *pTask )
{
	CTalkMonster::StartTask( pTask );	
}

void COtis :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask( pTask );
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int COtis :: ISoundMask () 
{
	return	bits_SOUND_WORLD	|
			bits_SOUND_COMBAT	|
			bits_SOUND_CARCASS	|
			bits_SOUND_MEAT		|
			bits_SOUND_GARBAGE	|
			bits_SOUND_DANGER	|
			bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	COtis :: Classify ()
{
	return m_iClass ? m_iClass : CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - otis says "Freeze!"
//=========================================================
void COtis :: AlertSound()
{
	if ( m_hEnemy != NULL )
	{
		if ( FOkToSpeak() )
		{
			PlaySentence( "OT_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE );
		}
	}

}
//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void COtis :: SetYawSpeed ()
{
	int ys;

	ys = 0;

	switch ( m_Activity )
	{
	case ACT_IDLE:		
		ys = 70;
		break;
	case ACT_WALK:
		ys = 70;
		break;
	case ACT_RUN:
		ys = 90;
		break;
	default:
		ys = 70;
		break;
	}

	pev->yaw_speed = ys;
}


//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL COtis :: CheckRangeAttack1 ( float flDot, float flDist )
{
	if ( flDist <= 1024 && flDot >= 0.5 )
	{
		if ( gpGlobals->time > m_checkAttackTime )
		{
			TraceResult tr;
			
			Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
			CBaseEntity *pEnemy = m_hEnemy;
			Vector shootTarget = ( (pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin) + m_vecEnemyLKP );
			UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr );
			m_checkAttackTime = gpGlobals->time + 1;
			if ( tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy) )
				m_lastAttackCheck = TRUE;
			else
				m_lastAttackCheck = FALSE;
			m_checkAttackTime = gpGlobals->time + 1.5;
		}
		return m_lastAttackCheck;
	}
	return FALSE;
}


//=========================================================
// OtisFirePistol - shoots one round from the pistol at
// the enemy otis is facing.
//=========================================================
void COtis :: OtisFireDeagle ()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );

	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_4DEGREES, 1024, BULLET_PLAYER_EAGLE);
	
	int pitchShift = RANDOM_LONG( 0, 20 );
	
	// Only shift about half the time
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "weapons/de_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift );

	CSoundEnt::InsertSound ( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

void COtis::OtisFireGlock()
{
	Vector vecShootOrigin;
	pev->framerate = 1.55;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}
void COtis::OtisFire357()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 1024, BULLET_PLAYER_357);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	// UNDONE: Reload?
	m_cAmmoLoaded--;// take away a bullet!
}

void COtis::OtisFireShotgun()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	FireBullets(6, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 1); // shoot +-7.5 degrees

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	m_cAmmoLoaded--;// take away a bullet!

}

void COtis::OtisFireMP5()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5, 1); // shoot +-5 degrees

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks1.wav", 1, ATTN_NORM, 0, 100 + pitchShift); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks2.wav", 1, ATTN_NORM, 0, 100 + pitchShift); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/hks3.wav", 1, ATTN_NORM, 0, 100 + pitchShift); break;
	}

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	m_cAmmoLoaded--;// take away a bullet!
}
		
//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void COtis :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch (pEvent->event)
	{
	case OTIS_AE_SHOOT:
	{

		switch (pev->weapons)
		{
		case OTIS_GLOCK:
			OtisFireGlock();
			break;
		case OTIS_357:
			OtisFire357();
			break;
		case OTIS_DEAGLE:
			OtisFireDeagle();
			break;
		case OTIS_SHOTGUN:
			OtisFireShotgun();
			break;
		case OTIS_MP5:
			OtisFireMP5();
			break;
		}

	}
	break;

	case OTIS_AE_DRAW:
	{
		// barney's bodygroup switches here so he can pull gun from holster
		switch (pev->weapons)
		{
		case OTIS_GLOCK:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Glock_drawn);
			m_iOtisGun = OtisWeapon::Glock_drawn;
			break;
		case OTIS_357:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Python_drawn);
			m_iOtisGun = OtisWeapon::Python_drawn;
			break;
		case OTIS_DEAGLE:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Deagle_drawn);
			m_iOtisGun = OtisWeapon::Deagle_drawn;
			break;
		case OTIS_SHOTGUN:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Shotgun_drawn);
			m_iOtisGun = OtisWeapon::Shotgun_drawn;
			break;
		case OTIS_MP5:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::MP5_drawn);
			m_iOtisGun = OtisWeapon::MP5_drawn;
			break;
		}
		m_fGunDrawn = TRUE;
	}
	break;

	case OTIS_AE_HOLSTER:
	{
		// change bodygroup to replace gun in holster
		switch (pev->weapons)
		{
		case OTIS_GLOCK:

			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Glock_holstered);
			m_iOtisGun = OtisWeapon::Glock_holstered;
			break;
		case OTIS_357:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Python_holstered);
			m_iOtisGun = OtisWeapon::Python_holstered;
			break;
		case OTIS_DEAGLE:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Deagle_holstered);
			m_iOtisGun = OtisWeapon::Deagle_holstered;
			break;
		case OTIS_SHOTGUN:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Shotgun_holstered);
			m_iOtisGun = OtisWeapon::Shotgun_holstered;
			break;
		case OTIS_MP5:
			SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::MP5_holstered);
			m_iOtisGun = OtisWeapon::MP5_holstered;
			break;
		}
		m_fGunDrawn = FALSE;
	}
	break;

	case OTIS_AE_RELOAD:
	{
		EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barney/ba_reload1.wav", 1, ATTN_NORM);
	}
	case OTIS_AE_MELEE:
	{
		CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
		if (pHurt)
		{
			if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
			{
				pHurt->pev->punchangle.z = -18;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
			}
		}
	}
	break;


	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Spawn
//=========================================================
void COtis :: Spawn()
{
	Precache( );

	SET_MODEL(ENT(pev), "models/otis.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	pev->health			= gSkillData.otisHealth + 10;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_MELEE_ATTACK1;

	//Note: This originally didn't use SetBodygroup
	if( m_iOtisHead == OtisHelm::Random )// pick random helmet
		m_iOtisHead = RANDOM_LONG( 0, 4 );

	if (m_iOtisGlasses == OtisGlasses::Random)// pick random glasses
		m_iOtisGlasses = RANDOM_LONG(0, 2);

	if( m_iOtisGun == OtisWeapon::Random )// pick random gun
		pev->weapons = RANDOM_LONG(1, 5);


	if (m_iOtisArms == OtisArms::Random)// pick random sleeves
	{
		if (m_iOtisVest != OtisVest::Random)
		{
			if (m_iOtisVest <= OtisVest::vest_pants)//if the shirt is light blue...
			{
				m_iOtisArms = RANDOM_LONG(0, 1);//pick light blue sleeves
			}
			else m_iOtisArms = RANDOM_LONG(0, 1) + 2;//else dark sleeves
		}
		else m_iOtisArms = RANDOM_LONG(0, 3);
	}
	if (m_iOtisVest == OtisVest::Random)// pick random pants and vest
	{
		if (m_iOtisArms <= OtisArms::RolledUp)//if the sleeves are light blue...
		{
			m_iOtisVest = RANDOM_LONG(0, 3);//pick a light blue shirt
		}
		else m_iOtisVest = RANDOM_LONG(0, 3) + 4;//else dark shirt
	}

	if (pev->skin == -1)
		pev->skin = RANDOM_LONG(0, 7);


	SetBodygroup( OtisBodyGroup::Gun, m_iOtisGun );
	SetBodygroup( OtisBodyGroup::Glasses, m_iOtisGlasses);
	SetBodygroup( OtisBodyGroup::Helmet, m_iOtisHead );
	SetBodygroup(OtisBodyGroup::Arms, m_iOtisArms);
	SetBodygroup(OtisBodyGroup::Vest, m_iOtisVest);

	// initialise weapons
	switch (pev->weapons)
	{
	case OTIS_357:
		SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Python_holstered);
		m_iOtisGun = OtisWeapon::Python_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 6;
		break;
	case OTIS_DEAGLE:
		SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Deagle_holstered);
		m_iOtisGun = OtisWeapon::Deagle_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 8;
		break;
	case OTIS_SHOTGUN:
		SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Shotgun_holstered);
		m_iOtisGun = OtisWeapon::Shotgun_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 8;
		break;
	case OTIS_MP5:
		SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::MP5_holstered);
		m_iOtisGun = OtisWeapon::MP5_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 30;
		break;
	case OTIS_NOGUN:
		SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::None);
		m_iOtisGun = OtisWeapon::None;
		m_fGunDrawn = false;
		break;
	default:
		pev->weapons = OTIS_GLOCK;
		SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::Glock_holstered);
		m_iOtisGun = OtisWeapon::Glock_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 17;
		break;
	}
	
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_cAmmoLoaded = m_iClipSize;

	MonsterInit();
	SetUse( &COtis::FollowerUse );
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void COtis :: Precache()
{
	PRECACHE_MODEL("models/otis.mdl");

	PRECACHE_SOUND("barney/ba_attack1.wav" );
	PRECACHE_SOUND("barney/ba_attack2.wav" );

	PRECACHE_SOUND( "weapons/de_shot1.wav" );

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");
	
	// every new otis must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}	

// Init talk data
void COtis :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER]  =	"OT_ANSWER";
	m_szGrp[TLK_QUESTION] =	"OT_QUESTION";
	m_szGrp[TLK_IDLE] =		"OT_IDLE";
	m_szGrp[TLK_STARE] =		"OT_STARE";
	m_szGrp[TLK_USE] =		"OT_OK";
	m_szGrp[TLK_UNUSE] =	"OT_WAIT";
	m_szGrp[TLK_STOP] =		"OT_STOP";

	m_szGrp[TLK_NOSHOOT] =	"OT_SCARED";
	m_szGrp[TLK_HELLO] =	"OT_HELLO";

	m_szGrp[TLK_PLHURT1] =	"!BA_CUREA";
	m_szGrp[TLK_PLHURT2] =	"!BA_CUREB"; 
	m_szGrp[TLK_PLHURT3] =	"!BA_CUREC";

	m_szGrp[TLK_PHELLO] =	NULL;	//"OT_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] =	NULL;	//"OT_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "OT_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] =	"OT_SMELL";
	
	m_szGrp[TLK_WOUND] =	"OT_WOUND";
	m_szGrp[TLK_MORTAL] =	"OT_MORTAL";

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
}

int COtis :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;

	if ( m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT) )
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if ( m_hEnemy == NULL )
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ( (m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing( pevAttacker, pev->origin ) )
			{
				// Alright, now I'm pissed!
				PlaySentence( "OT_MAD", 4, VOL_NORM, ATTN_NORM );

				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				// Hey, be careful with that
				PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "OT_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}

	return ret;
}

	
//=========================================================
// PainSound
//=========================================================
void COtis :: PainSound ()
{
	if (gpGlobals->time < m_painTime)
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void COtis :: DeathSound ()
{
	switch (RANDOM_LONG(0,2))
	{
	case 0: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN( ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void COtis::TraceAttack( entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType)
{
	switch( ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (GetBodygroup(OtisBodyGroup::Vest) == OtisVest::vest_jeans || GetBodygroup(OtisBodyGroup::Vest) == OtisVest::vest_pants)
		{
			if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
			{
				flDamage = flDamage / 2;
			}
			break;
		}
		
	case 10:
		if (m_iOtisHead == OtisHelm::Helmet || m_iOtisHead == OtisHelm::Helmet_glass)
		{
			if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
			{
				flDamage -= 20;
				if (flDamage <= 0)
				{
					UTIL_Ricochet(ptr->vecEndPos, 1.0);
					flDamage = 0.01;
				}
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}


void COtis::Killed( entvars_t *pevAttacker, int iGib )
{
	if ( GetBodygroup( OtisBodyGroup::Gun ) != OtisWeapon::None )
	{// drop the gun!
		Vector vecGunPos;
		Vector vecGunAngles;

		if (pev->spawnflags & SF_DONTDROPGUN)
		{
			return;
		}
		else
		{
			if (GetBodygroup(OtisBodyGroup::Gun) != OtisWeapon::None)
			{
				Vector vecGunPos;
				Vector vecGunAngles;

				SetBodygroup(OtisBodyGroup::Gun, OtisWeapon::None);
				m_iOtisGun = OtisWeapon::None;

				GetAttachment(0, vecGunPos, vecGunAngles);

				if (pev->weapons == OTIS_GLOCK)
				{
					CBaseEntity* pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
				}
				if (pev->weapons == OTIS_357)
				{
					CBaseEntity* pGun = DropItem("weapon_357", vecGunPos, vecGunAngles);
				}
				if (pev->weapons == OTIS_DEAGLE)
				{
					CBaseEntity* pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
				}
				if (pev->weapons == OTIS_MP5)
				{
					CBaseEntity* pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
				}
				if (pev->weapons == OTIS_SHOTGUN)
				{
					CBaseEntity* pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
				}
				pev->weapons = OTIS_NOGUN;
			}
		}
		
		
	}

	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* COtis :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
		{
			// face enemy, then draw.
			return slOtisEnemyDraw;
		}
		break;

	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that otis will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slOtFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slOtFollow;

	case SCHED_OTIS_DISARM:
		return slOtisDisarm;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleOtStand;
		}
		else
			return psched;	
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t *COtis :: GetSchedule ()
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
	{
		PlaySentence( "OT_KILL", 4, VOL_NORM, ATTN_NORM );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		{
// dead enemy
			if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return CBaseMonster :: GetSchedule();
			}

			// always act surprized with a new enemy
			if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE) )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
				
			// wait for one schedule to draw gun
			if (!m_fGunDrawn )
				return GetScheduleOfType( SCHED_ARM_WEAPON );

			if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );

			if (m_cAmmoLoaded <= 0)
			{
				m_cAmmoLoaded = m_iClipSize;
				return GetScheduleOfType(SCHED_RELOAD);
			}
		}
		break;

	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:

		if (m_fGunDrawn == TRUE)// if we're no longer fighting then holster the gun
		{
			return GetScheduleOfType(SCHED_OTIS_DISARM);
		}

		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}
			else
			{
				if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				{
					return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
				}
				return GetScheduleOfType( SCHED_TARGET_FACE );
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
		{
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		}

		// try to say something about smells
		TrySmellTalk();
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

//=========================================================
// SetActivity 
//=========================================================
void COtis::SetActivity(Activity NewActivity)
{
	int	iSequence = -1;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
	{
		switch (pev->weapons)
		{
		case OTIS_GLOCK:
			iSequence = LookupSequence("shootgun");
			break;
		case OTIS_357:
		case OTIS_DEAGLE:
			iSequence = LookupSequence("shoot357");
			break;
		case OTIS_SHOTGUN:
			iSequence = LookupSequence("shootsgun");
			break;
		case OTIS_MP5:
			iSequence = LookupSequence("shootmp5");
			break;
		}
	}
	break;
	case ACT_ARM:
	{
		if (pev->weapons >= OTIS_MP5)
		{
			iSequence = LookupSequence("draw_mp5");
		}
		else iSequence = LookupSequence("draw");
	}
	break;
	case ACT_DISARM:
	{
		if (pev->weapons >= OTIS_MP5)
		{
			iSequence = LookupSequence("disarm_mp5");
		}
		else iSequence = LookupSequence("disarm");
	}
	break;
	case ACT_RELOAD:
	{
		switch (pev->weapons)
		{
		case OTIS_GLOCK:
			iSequence = LookupSequence("reload");
			break;
		case OTIS_357:
			iSequence = LookupSequence("reload_357");
			break;
		case OTIS_DEAGLE:
			iSequence = LookupSequence("reload_deagle");
			break;
		case OTIS_SHOTGUN:
			iSequence = LookupSequence("reload_shotgun");
			break;
		case OTIS_MP5:
			iSequence = LookupSequence("reload_mp5");
			break;
		}
	}
	break;
	case ACT_RUN:
	{
		if (pev->health <= gSkillData.barneyHealth * 0.25)
		{
			// limp!
			iSequence = LookupSequence("limpingrun");
		}

		else
		{
			if ((pev->weapons == OTIS_SHOTGUN || pev->weapons == OTIS_MP5) && m_fGunDrawn == TRUE)
			{
				iSequence = LookupSequence("run_mp5");
			}
			else iSequence = LookupSequence("run");
		}
	}
	break;
	case ACT_WALK:
	{
		if (pev->health <= gSkillData.barneyHealth * 0.25)
		{
			// limp!
			iSequence = LookupSequence("limpingwalk");
		}

		else
		{
			if ((pev->weapons == OTIS_SHOTGUN || pev->weapons == OTIS_MP5) && m_fGunDrawn == TRUE)
			{
				iSequence = LookupSequence("walk_mp5");
			}
			else iSequence = LookupSequence("walk");
		}
	}
	break;
	case ACT_IDLE:
	{
		if ((pev->weapons == OTIS_SHOTGUN || pev->weapons == OTIS_MP5) && m_fGunDrawn == TRUE)
		{
			iSequence = LookupActivity(ACT_IDLE_ANGRY);
		}
		iSequence = LookupActivity(NewActivity);
	}
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
		//ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}

}

MONSTERSTATE COtis :: GetIdealState ()
{
	return CTalkMonster::GetIdealState();
}



void COtis::DeclineFollowing()
{
	PlaySentence( "OT_POK", 2, VOL_NORM, ATTN_NORM );
}

void COtis::KeyValue( KeyValueData* pkvd )
{
	if( FStrEq( "head", pkvd->szKeyName ) )
	{
		m_iOtisHead = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	if( FStrEq( "vest", pkvd->szKeyName ) )
	{
		m_iOtisVest = atoi( pkvd->szValue );
		pkvd->fHandled = true;
	}
	if (FStrEq("arms", pkvd->szKeyName))
	{
		m_iOtisArms = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("glasses", pkvd->szKeyName))
	{
		m_iOtisGlasses = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}



//=========================================================
// DEAD OTIS PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadOtis : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify () override { return	CLASS_PLAYER_ALLY; }

	void KeyValue( KeyValueData *pkvd ) override;
	int m_iOtisArms;
	int m_iOtisHead;
	int m_iOtisVest;

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static char *m_szPoses[5];
};

char *CDeadOtis::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "dead_sitting" };

void CDeadOtis::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_iOtisHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("vest", pkvd->szKeyName))
	{
		m_iOtisVest = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("arms", pkvd->szKeyName))
	{
		m_iOtisArms = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else 
		CBaseMonster::KeyValue( pkvd );
}

LINK_ENTITY_TO_CLASS( monster_otis_dead, CDeadOtis );

//=========================================================
// ********** DeadOtis SPAWN **********
//=========================================================
void CDeadOtis:: Spawn( )
{
	PRECACHE_MODEL("models/otis.mdl");
	SET_MODEL(ENT(pev), "models/otis.mdl");

	pev->effects		= 0;
	pev->yaw_speed		= 8;
	pev->sequence		= 0;
	m_bloodColor		= BLOOD_COLOR_RED;

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead otis with bad pose\n" );
	}
	// Corpses have less health
	pev->health			= 8;//gSkillData.otisHealth;

	//Note: This originally didn't use SetBodygroup
	if (m_iOtisHead == OtisHelm::Random)// pick random helmet
	{
		m_iOtisHead = RANDOM_LONG(0, 1);
	}
	if (m_iOtisArms == OtisArms::Random)// pick random sleeves
	{
		m_iOtisArms = RANDOM_LONG(0, 1);
	}
	if (m_iOtisVest == OtisVest::Random)// pick random pants and vest
	{
		m_iOtisVest = RANDOM_LONG(0, 3);
	}
	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, 7);
	}
	SetBodygroup(OtisBodyGroup::Helmet, m_iOtisHead);
	SetBodygroup(OtisBodyGroup::Arms, m_iOtisArms);
	SetBodygroup(OtisBodyGroup::Vest, m_iOtisVest);

	MonsterInitDead();
}


