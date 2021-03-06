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
// monster template
//=========================================================

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

namespace BarneyBodyGroup
{
	enum BarneyBodyGroup
	{
		Head = 0,
		Helmet,
		Vest,
		Glasses,
		Gun
	};
}

namespace BarneyHead
{
	enum BarneyHead
	{
		Random = -1,
		Barney = 0,
		Clint,
		Jonny,
		Marley,
		Tommy,
		Leonel,
		Bill,
		Ted,
		BarneyHL2,
	};
}

namespace BarneyHelm
{
	enum BarneyHelm
	{
		Random = -1,
		No_helm = 0,
		Helm,
		Helm_glass,
		Cap,
		Beret
	};
}

namespace BarneyVest
{
	enum BarneyVest
	{
		Random = -1,
		Civ = 0,
		Vest,
		Jacket
	};
}

namespace BarneyGlasses
{
	enum BarneyGlasses
	{
		Random = -1,
		None = 0,
		Cool,
		Uncool
	};
}

namespace BarneyGun
{
	enum BarneyGun
	{
		Glock_holstered = 0,
		Glock_drawn,
		Python_holstered,
		Python_drawn,
		Deagle_holstered,
		Deagle_drawn,
		MP5_holstered,
		MP5_drawn,
		Shotgun_holstered,
		Shotgun_drawn,
		None
	};
}

//=========================================================
// monster-specific DEFINE's
//=========================================================
#define	NUM_BARNEY_SKINS		4 // four skin variations available for Barney model

#define BARNEY_GLOCK		1
#define BARNEY_357			3
#define BARNEY_DEAGLE 		5
#define BARNEY_MP5 			6
#define BARNEY_SHOTGUN 		7
#define BARNEY_NOGUN 10

#define SF_DONTDROPGUN (1024)

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
// first flag is barney dying for scripted sequences?
#define		BARNEY_AE_DRAW		( 2 )
#define		BARNEY_AE_SHOOT		( 3 )
#define		BARNEY_AE_HOLSTER	( 4 )
#define		BARNEY_AE_RELOAD	( 5 )
#define		BARNEY_AE_MELEE		( 10 )

#define	BARNEY_BODY_GUNHOLSTERED	0
#define	BARNEY_BODY_GUNDRAWN		1
#define BARNEY_BODY_GUNGONE			2

class CBarney : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  ISoundMask() override;

	void BarneyFirePistol();
	void BarneyFire357();
	void BarneyFireDeagle();
	void BarneyFireMP5();
	void BarneyFireShotgun();

	void AlertSound() override;
	int  Classify() override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void RunTask(Task_t* pTask) override;
	void StartTask(Task_t* pTask) override;
	int	ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	BOOL CheckRangeAttack1(float flDot, float flDist) override;

	void DeclineFollowing() override;

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;
	MONSTERSTATE GetIdealState() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit();

	void SetActivity(Activity NewActivity) override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];

	void KeyValue(KeyValueData* pkvd);

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;

	int m_BarneyGun;
	int m_BarneyHead;
	int m_BarneyHelm;
	int m_BarneyVest;
	int m_BarneyGlasses;

	int		m_iShotgunShell;

	int m_iClipSize;


	// UNDONE: What is this for?  It isn't used?
	float	m_flPlayerDamage;// how much pain has the player inflicted on me?

	CUSTOM_SCHEDULES;
};

LINK_ENTITY_TO_CLASS(monster_barney, CBarney);

TYPEDESCRIPTION	CBarney::m_SaveData[] =
{
	DEFINE_FIELD(CBarney, m_fGunDrawn, FIELD_BOOLEAN),
	DEFINE_FIELD(CBarney, m_painTime, FIELD_TIME),
	DEFINE_FIELD(CBarney, m_checkAttackTime, FIELD_TIME),
	DEFINE_FIELD(CBarney, m_lastAttackCheck, FIELD_BOOLEAN),
	DEFINE_FIELD(CBarney, m_flPlayerDamage, FIELD_FLOAT),
	DEFINE_FIELD(CBarney, m_iClipSize, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CBarney, CTalkMonster);

//=========================================================
// Monster-specific Schedule Types
//=========================================================
enum
{
	SCHED_BARNEY_DISARM = LAST_TALKMONSTER_SCHEDULE + 1,
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE(tlBaFollow),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

//=========================================================
// BarneyDraw- much better looking draw schedule for when
// barney knows who he's gonna attack.
//=========================================================
Task_t	tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_FACE_ENEMY,					0				},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,	(float)ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] =
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE(tlBarneyEnemyDraw),
		0,
		0,
		"Barney Enemy Draw"
	}
};

Task_t	tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE(tlBaFaceTarget),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleBaStand[] =
{
	{
		tlIdleBaStand,
		ARRAYSIZE(tlIdleBaStand),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

Task_t	tlBarneyDisarm[] =
{
	{ TASK_STOP_MOVING,					0				},
	{ TASK_PLAY_SEQUENCE,	(float)ACT_DISARM },
};

Schedule_t slBarneyDisarm[] =
{
	{
		tlBarneyDisarm,
		ARRAYSIZE(tlBarneyDisarm),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags - change these, and you'll break the talking code.
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|

		bits_SOUND_DANGER,
		"Barney Disarm"
	}
};

DEFINE_CUSTOM_SCHEDULES(CBarney)
{
	slBaFollow,
		slBarneyEnemyDraw,
		slBaFaceTarget,
		slIdleBaStand,
		slBarneyDisarm,
};


IMPLEMENT_CUSTOM_SCHEDULES(CBarney, CTalkMonster);

void CBarney::StartTask(Task_t* pTask)
{
	CTalkMonster::StartTask(pTask);
}

void CBarney::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
		if (m_hEnemy != NULL && (m_hEnemy->IsPlayer()))
		{
			pev->framerate = 1.5;
		}
		CTalkMonster::RunTask(pTask);
		break;
	default:
		CTalkMonster::RunTask(pTask);
		break;
	}
}




//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. 
//=========================================================
int CBarney::ISoundMask()
{
	return	bits_SOUND_WORLD |
		bits_SOUND_COMBAT |
		bits_SOUND_CARCASS |
		bits_SOUND_MEAT |
		bits_SOUND_GARBAGE |
		bits_SOUND_DANGER |
		bits_SOUND_PLAYER;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CBarney::Classify()
{
	return	m_iClass ? m_iClass : CLASS_PLAYER_ALLY;
}

//=========================================================
// ALertSound - barney says "Freeze!"
//=========================================================
void CBarney::AlertSound()
{
	if (m_hEnemy != NULL)
	{
		if (FOkToSpeak())
		{
			PlaySentence("BA_ATTACK", RANDOM_FLOAT(2.8, 3.2), VOL_NORM, ATTN_IDLE);
		}
	}

}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CBarney::SetYawSpeed()
{
	int ys;

	ys = 0;

	switch (m_Activity)
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
BOOL CBarney::CheckRangeAttack1(float flDot, float flDist)
{
	if (flDist <= 1024 && flDot >= 0.5)
	{
		if (gpGlobals->time > m_checkAttackTime)
		{
			TraceResult tr;

			Vector shootOrigin = pev->origin + Vector(0, 0, 55);
			CBaseEntity* pEnemy = m_hEnemy;
			Vector shootTarget = ((pEnemy->BodyTarget(shootOrigin) - pEnemy->pev->origin) + m_vecEnemyLKP);
			UTIL_TraceLine(shootOrigin, shootTarget, dont_ignore_monsters, ENT(pev), &tr);
			m_checkAttackTime = gpGlobals->time + 1;
			if (tr.flFraction == 1.0 || (tr.pHit != NULL && CBaseEntity::Instance(tr.pHit) == pEnemy))
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
// BarneyFirePistol - shoots one round from the pistol at
// the enemy barney is facing.
//=========================================================
void CBarney::BarneyFirePistol()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM, 1);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/pl_gun3.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	m_cAmmoLoaded--;// take away a bullet!

}

void CBarney::BarneyFire357()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 1024, BULLET_PLAYER_357, 1);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/357_shot1.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	
	m_cAmmoLoaded--;// take away a bullet!

}

void CBarney::BarneyFireDeagle()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_6DEGREES, 1024, BULLET_PLAYER_EAGLE, 1);

	int pitchShift = RANDOM_LONG(0, 20);

	// Only shift about half the time
	if (pitchShift > 10)
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, 100 + pitchShift);

	CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

	m_cAmmoLoaded--;// take away a bullet!

}

void CBarney::BarneyFireShotgun()
{
	Vector vecShootOrigin;

	UTIL_MakeVectors(pev->angles);
	vecShootOrigin = pev->origin + Vector(0, 0, 55);
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	Vector angDir = UTIL_VecToAngles(vecShootDir);
	SetBlending(0, angDir.x);
	pev->effects = EF_MUZZLEFLASH;

	Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
	EjectBrass(vecShootOrigin - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL);
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

void CBarney::BarneyFireMP5()
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
void CBarney::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BARNEY_AE_SHOOT:
	{

		switch (pev->weapons)
		{
		case BARNEY_GLOCK:
			BarneyFirePistol();
			break;
		case BARNEY_357:
			BarneyFire357();
			break;
		case BARNEY_DEAGLE:
			BarneyFireDeagle();
			break;
		case BARNEY_SHOTGUN:
			BarneyFireShotgun();
			break;
		case BARNEY_MP5:
			BarneyFireMP5();
			break;
		}

	}
	break;

	case BARNEY_AE_DRAW:
	{
		// barney's bodygroup switches here so he can pull gun from holster
		switch (pev->weapons)
		{
		case BARNEY_GLOCK:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Glock_drawn);
			m_BarneyGun = BarneyGun::Glock_drawn;
			break;
		case BARNEY_357:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Python_drawn);
			m_BarneyGun = BarneyGun::Python_drawn;
			break;
		case BARNEY_DEAGLE:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Deagle_drawn);
			m_BarneyGun = BarneyGun::Deagle_drawn;
			break;
		case BARNEY_SHOTGUN:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Shotgun_drawn);
			m_BarneyGun = BarneyGun::Shotgun_drawn;
			break;
		case BARNEY_MP5:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::MP5_drawn);
			m_BarneyGun = BarneyGun::MP5_drawn;
			break;
		}
		m_fGunDrawn = TRUE;
	}
	break;

	case BARNEY_AE_HOLSTER:
	{
		// change bodygroup to replace gun in holster
		switch (pev->weapons)
		{
		case BARNEY_GLOCK:

			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Glock_holstered);
			m_BarneyGun = BarneyGun::Glock_holstered;
			break;
		case BARNEY_357:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Python_holstered);
			m_BarneyGun = BarneyGun::Python_holstered;
			break;
		case BARNEY_DEAGLE:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Deagle_holstered);
			m_BarneyGun = BarneyGun::Deagle_holstered;
			break;
		case BARNEY_SHOTGUN:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Shotgun_holstered);
			m_BarneyGun = BarneyGun::Shotgun_holstered;
			break;
		case BARNEY_MP5:
			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::MP5_holstered);
			m_BarneyGun = BarneyGun::MP5_holstered;
			break;
		}
		m_fGunDrawn = FALSE;
	}
	break;

	case BARNEY_AE_RELOAD:
		{
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "barney/ba_reload1.wav", 1, ATTN_NORM);
		}
	case BARNEY_AE_MELEE:
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
void CBarney::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/barney.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;

	// this block is for backwards compatibility with early HL:E maps and Valve maps
	if (m_BarneyHead == 0 && m_BarneyHelm == 0 && m_BarneyVest == 0 && pev->weapons == 0)// all these parameters are empty, probably a deprecated Barney
	{
		m_BarneyHead = BarneyHead::Random;
		m_BarneyHelm = BarneyHelm::Random;
		m_BarneyVest = BarneyVest::Random;
		m_BarneyGlasses = BarneyGlasses::Random;
		pev->skin = -1;
	}

	// choose random body and skin
	if (m_BarneyHead == BarneyHead::Random)
		m_BarneyHead = RANDOM_LONG(0, 7);

	if (m_BarneyHelm == BarneyHelm::Random)
		m_BarneyHelm = RANDOM_LONG(0, 4);

	if (m_BarneyVest == BarneyVest::Random)
		m_BarneyVest = RANDOM_LONG(0, 2);

	if (m_BarneyGlasses == BarneyGlasses::Random)
		m_BarneyGlasses = RANDOM_LONG(0, 2);
	

	if (pev->skin == -1)
		pev->skin = RANDOM_LONG(0, NUM_BARNEY_SKINS - 1);// pick a skin, any skin

	
	SetBodygroup(BarneyBodyGroup::Head, m_BarneyHead);
	SetBodygroup(BarneyBodyGroup::Helmet, m_BarneyHelm);
	SetBodygroup(BarneyBodyGroup::Vest, m_BarneyVest);
	SetBodygroup(BarneyBodyGroup::Glasses, m_BarneyGlasses);

	// initialise weapons
	switch (pev->weapons)
	{
	case BARNEY_357:
		SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Python_holstered);
		m_BarneyGun = BarneyGun::Python_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 6;
		break;
	case BARNEY_DEAGLE:
		SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Deagle_holstered);
		m_BarneyGun = BarneyGun::Deagle_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 8;
		break;
	case BARNEY_SHOTGUN:
		SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Shotgun_holstered);
		m_BarneyGun = BarneyGun::Shotgun_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 8;
		break;
	case BARNEY_MP5:
		SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::MP5_holstered);
		m_BarneyGun = BarneyGun::MP5_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 30;
		break;
	case BARNEY_NOGUN:
		SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::None);
		m_BarneyGun = BarneyGun::None;
		m_fGunDrawn = false;
		break;
	default:
		pev->weapons = BARNEY_GLOCK;
		SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::Glock_holstered);
		m_BarneyGun = BarneyGun::Glock_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 17;
		break;
	}

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP | bits_CAP_MELEE_ATTACK1;
	m_cAmmoLoaded = m_iClipSize;

	MonsterInit();
	SetUse(&CBarney::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CBarney::Precache()
{
	PRECACHE_MODEL("models/barney.mdl");

	m_iShotgunShell = PRECACHE_MODEL("models/shotgunshell.mdl");

	PRECACHE_SOUND("barney/ba_reload1.wav");

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");

	PRECACHE_SOUND("hgrunt/gr_mgun1.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

// Init talk data
void CBarney::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "BA_ANSWER";
	m_szGrp[TLK_QUESTION] = "BA_QUESTION";
	m_szGrp[TLK_IDLE] = "BA_IDLE";
	m_szGrp[TLK_STARE] = "BA_STARE";
	m_szGrp[TLK_USE] = "BA_OK";
	m_szGrp[TLK_UNUSE] = "BA_WAIT";
	m_szGrp[TLK_STOP] = "BA_STOP";

	m_szGrp[TLK_NOSHOOT] = "BA_SCARED";
	m_szGrp[TLK_HELLO] = "BA_HELLO";

	m_szGrp[TLK_PLHURT1] = "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BA_CUREB";
	m_szGrp[TLK_PLHURT3] = "!BA_CUREC";

	m_szGrp[TLK_PHELLO] = NULL;	//"BA_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;	//"BA_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";		// UNDONE

	m_szGrp[TLK_SMELL] = "BA_SMELL";

	m_szGrp[TLK_WOUND] = "BA_WOUND";
	m_szGrp[TLK_MORTAL] = "BA_MORTAL";

	// get voice for head - just one barney voice for now
	m_voicePitch = 100;
}


BOOL IsFacing(entvars_t* pevTest, const Vector& reference)
{
	Vector vecDir = (reference - pevTest->origin);
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward, angle;
	angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate(angle, forward, NULL, NULL);
	// He's facing me, he meant it
	if (DotProduct(forward, vecDir) > 0.96)	// +/- 15 degrees or so
	{
		return TRUE;
	}
	return FALSE;
}


int CBarney::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// make sure friends talk about it if player hurts talkmonsters...
	int ret = CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
	if (!IsAlive() || pev->deadflag == DEAD_DYING)
		return ret;

	if (m_MonsterState != MONSTERSTATE_PRONE && (pevAttacker->flags & FL_CLIENT))
	{
		m_flPlayerDamage += flDamage;

		// This is a heurstic to determine if the player intended to harm me
		// If I have an enemy, we can't establish intent (may just be crossfire)
		if (m_hEnemy == NULL)
		{
			// If the player was facing directly at me, or I'm already suspicious, get mad
			if ((m_afMemory & bits_MEMORY_SUSPICIOUS) || IsFacing(pevAttacker, pev->origin))
			{
				// Alright, now I'm pissed!
				PlaySentence("BA_MAD", 4, VOL_NORM, ATTN_NORM);

				Remember(bits_MEMORY_PROVOKED);
				StopFollowing(TRUE);
			}
			else
			{
				// Hey, be careful with that
				PlaySentence("BA_SHOT", 4, VOL_NORM, ATTN_NORM);
				Remember(bits_MEMORY_SUSPICIOUS);
			}
		}
		else if (!(m_hEnemy->IsPlayer()) && pev->deadflag == DEAD_NO)
		{
			PlaySentence("BA_SHOT", 4, VOL_NORM, ATTN_NORM);
		}
	}

	return ret;
}


//=========================================================
// PainSound
//=========================================================
void CBarney::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CBarney::DeathSound()
{
	switch (RANDOM_LONG(0, 2))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}


void CBarney::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if (m_BarneyVest == BarneyVest::Vest)
		{
			if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST))
			{
				flDamage = flDamage / 2;
			}
		}
		break;
	case 10:
		if (m_BarneyHelm == BarneyHelm::Helm || m_BarneyHelm == BarneyHelm::Helm_glass)
		{
			if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
			{
				flDamage -= 20;
				if (flDamage <= 0)
				{
					UTIL_Ricochet(ptr->vecEndPos, 1.0);
					UTIL_Sparks(ptr->vecEndPos);
					flDamage = 0.01;
				}
			}
		}
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CBarney::Killed(entvars_t* pevAttacker, int iGib)
{
	if (pev->spawnflags & SF_DONTDROPGUN)
	{
		return;
	}
	else
	{
		if (GetBodygroup(BarneyBodyGroup::Gun) != BarneyGun::None)
		{
			Vector vecGunPos;
			Vector vecGunAngles;

			SetBodygroup(BarneyBodyGroup::Gun, BarneyGun::None);
			m_BarneyGun = BarneyGun::None;

			GetAttachment(0, vecGunPos, vecGunAngles);

			if (pev->weapons == BARNEY_GLOCK)
			{
				CBaseEntity* pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_357)
			{
				CBaseEntity* pGun = DropItem("weapon_357", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_DEAGLE)
			{
				CBaseEntity* pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_MP5)
			{
				CBaseEntity* pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_SHOTGUN)
			{
				CBaseEntity* pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
		}
	}

	UTIL_SetSize(pev, Vector(-33, -13, 0), Vector(33, 13, 0));
	UTIL_SetOrigin(pev, pev->origin);

	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Schedule_t* CBarney::GetScheduleOfType(int Type)
{
	Schedule_t* psched;

	switch (Type)
	{
	case SCHED_ARM_WEAPON:
		if (m_hEnemy != NULL)
		{
			// face enemy, then draw.
			return slBarneyEnemyDraw;
		}
		break;

		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that barney will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slBaFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slBaFollow;

	case SCHED_BARNEY_DISARM:
		return slBarneyDisarm;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
		{
			// just look straight ahead.
			return slIdleBaStand;
		}
		else
			return psched;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

//=========================================================
// GetSchedule - Decides which type of schedule best suits
// the monster's current state and conditions. Then calls
// monster's member function to get a pointer to a schedule
// of the proper type.
//=========================================================
Schedule_t* CBarney::GetSchedule()
{
	if (HasConditions(bits_COND_HEAR_SOUND))
	{
		CSound* pSound;
		pSound = PBestSound();

		ASSERT(pSound != NULL);
		if (pSound && (pSound->m_iType & bits_SOUND_DANGER))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_BEST_SOUND);
	}
	if (HasConditions(bits_COND_ENEMY_DEAD) && FOkToSpeak())
	{
		PlaySentence("BA_KILL", 4, VOL_NORM, ATTN_NORM);
	}

	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{
		// dead enemy
		if (HasConditions(bits_COND_ENEMY_DEAD))
		{
			// call base class, all code to handle dead enemies is centralized there.
			return CTalkMonster::GetSchedule();
		}

		// always act surprized with a new enemy
		if (HasConditions(bits_COND_NEW_ENEMY) && HasConditions(bits_COND_LIGHT_DAMAGE))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);

		// wait for one schedule to draw gun
		if (!m_fGunDrawn)
			return GetScheduleOfType(SCHED_ARM_WEAPON);

		if (HasConditions(bits_COND_HEAVY_DAMAGE))
			return GetScheduleOfType(SCHED_TAKE_COVER_FROM_ENEMY);

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
			return GetScheduleOfType(SCHED_BARNEY_DISARM);
		}

		if (HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType(SCHED_SMALL_FLINCH);
		}

		if (m_hEnemy == NULL && IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}
			else
			{
				if (HasConditions(bits_COND_CLIENT_PUSH))
				{
					return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				return GetScheduleOfType(SCHED_TARGET_FACE);
			}
		}
		

		if (HasConditions(bits_COND_CLIENT_PUSH))
		{
			return GetScheduleOfType(SCHED_MOVE_AWAY);
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
void CBarney::SetActivity(Activity NewActivity)
{
	int	iSequence = -1;
	void* pmodel = GET_MODEL_PTR(ENT(pev));

	switch (NewActivity)
	{
	case ACT_RANGE_ATTACK1:
	{
		switch (pev->weapons)
		{
		case BARNEY_GLOCK:
			iSequence = LookupSequence("shootgun");
			break;
		case BARNEY_357:
		case BARNEY_DEAGLE:
			iSequence = LookupSequence("shoot357");
			break;
		case BARNEY_SHOTGUN:
			iSequence = LookupSequence("shootsgun");
			break;
		case BARNEY_MP5:
			iSequence = LookupSequence("shootmp5");
			break;
		}
	}
	break;
	case ACT_ARM:
	{
		if (pev->weapons == BARNEY_SHOTGUN || pev->weapons == BARNEY_MP5)
		{
			iSequence = LookupSequence("draw_mp5");
		}
		else iSequence = LookupSequence("draw");
	}
	break;
	case ACT_DISARM:
	{
		if (pev->weapons == BARNEY_SHOTGUN || pev->weapons == BARNEY_MP5)
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
		case BARNEY_GLOCK:
			iSequence = LookupSequence("reload");
			break;
		case BARNEY_357:
			iSequence = LookupSequence("reload_357");
			break;
		case BARNEY_DEAGLE:
			iSequence = LookupSequence("reload_deagle");
			break;
		case BARNEY_SHOTGUN:
			iSequence = LookupSequence("reload_shotgun");
			break;
		case BARNEY_MP5:
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
			if ((pev->weapons == BARNEY_SHOTGUN || pev->weapons == BARNEY_MP5) && m_fGunDrawn == TRUE)
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
			if ((pev->weapons == BARNEY_SHOTGUN || pev->weapons == BARNEY_MP5) && m_fGunDrawn == TRUE)
			{
				iSequence = LookupSequence("walk_mp5");
			}
			else iSequence = LookupSequence("walk");
		}
	}
	break;
	case ACT_IDLE:
	{
		if ((pev->weapons == BARNEY_SHOTGUN || pev->weapons == BARNEY_MP5) && m_fGunDrawn == TRUE)
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
		ALERT(at_console, "%s has no sequence for act:%d\n", STRING(pev->classname), NewActivity);
		pev->sequence = 0;	// Set to the reset anim (if it's there)
	}
	
}

MONSTERSTATE CBarney::GetIdealState()
{
	return CTalkMonster::GetIdealState();
}



void CBarney::DeclineFollowing()
{
	PlaySentence("BA_POK", 2, VOL_NORM, ATTN_NORM);
}


void CBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_BarneyHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("vest", pkvd->szKeyName))
	{
		m_BarneyVest = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("helmet", pkvd->szKeyName))
	{
		m_BarneyHelm = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("glasses", pkvd->szKeyName))
	{
		m_BarneyGlasses = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CTalkMonster::KeyValue(pkvd);
	}
}


//=========================================================
// HEV Barney
//=========================================================

namespace HEVBarneyBodyGroup
{
	enum HEVBarneyBodyGroup
	{
		Body = 0,
		Head,
		Helmet,
		Gun,
		Glasses
	};
}

namespace HEVBarneyHead
{
	enum HEVBarneyHead
	{
		Random = -1,
		Barney = 0,
		Clint,
		Jonny,
		Marley,
		Tommy,
		Leonel,
		Bill,
		Ted
	};
}

namespace HEVBarneyGlasses
{
	enum HEVBarneyGlasses
	{
		Random = -1,
		None = 0,
		Cool,
		Uncool
	};
}

namespace HEVBarneyHelm
{
	enum HEVBarneyHelm
	{
		Random = -1,
		No_helm = 0,
		Helm,
		Helm_glass,
		Cap,
		Beret,
	};
}

namespace HEVBarneyGun
{
	enum HEVBarneyGun
	{
		Glock_holstered = 0,
		Glock_drawn,
		Python_holstered,
		Python_drawn,
		Deagle_holstered,
		Deagle_drawn,
		MP5_drawn,
		MP5_holstered,
		Shotgun_drawn,
		Shotgun_holstered,
		None
	};
}

class CHevBarney : public CBarney
{
public:
	void Spawn() override;
	void Precache() override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void Killed(entvars_t* pevAttacker, int iGib) override;
	void HandleAnimEvent(MonsterEvent_t* pEvent) override;

	void KeyValue(KeyValueData* pkvd) override;

	int m_HEVBarneyGun;
	int m_HEVBarneyGlasses;
	int m_HEVBarneyHead;
	int m_HEVBarneyHelm;
};

LINK_ENTITY_TO_CLASS(monster_barney_hev, CHevBarney);

#define NUM_HEVBARNEY_SKINS 2

//=========================================================
// Spawn
//=========================================================
void CHevBarney::Spawn()
{
	Precache();
	SET_MODEL(ENT(pev), "models/barney_hev.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector(0, 0, 50);// position of the eyes relative to monster's origin.
	m_flFieldOfView = VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so npc will notice player and say hello
	m_MonsterState = MONSTERSTATE_NONE;
	m_fGunDrawn = FALSE;

	// this block is for backwards compatibility with early HL:E maps and Valve maps
	if (m_HEVBarneyHead == 0 && m_HEVBarneyHelm == 0 && m_HEVBarneyGlasses == 0 && pev->weapons == 0)// all these parameters are empty, probably a deprecated HEVBarney
	{
		m_HEVBarneyHead = HEVBarneyHead::Random;
		m_HEVBarneyHelm = HEVBarneyHelm::Random;
		m_HEVBarneyGlasses = HEVBarneyGlasses::Random;
		pev->skin = -1;
	}

	//choose random body and skin
	if (m_HEVBarneyHead == HEVBarneyHead::Random)
		m_HEVBarneyHead = RANDOM_LONG(0, 7);

	if (m_HEVBarneyGlasses == HEVBarneyGlasses::Random)
		m_HEVBarneyGlasses = RANDOM_LONG(0, 2);

	if (m_HEVBarneyHelm == HEVBarneyHelm::Random)
		m_HEVBarneyHelm = RANDOM_LONG(0, 4);

	if (pev->skin == -1)
		pev->skin = RANDOM_LONG(0, NUM_HEVBARNEY_SKINS - 1);// pick a skin, any skin


	// initialise weapons
	switch (pev->weapons)
	{
	case BARNEY_357:
		SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Python_holstered);
		m_HEVBarneyGun = HEVBarneyGun::Python_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 6;
		break;
	case BARNEY_DEAGLE:
		SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Deagle_holstered);
		m_HEVBarneyGun = HEVBarneyGun::Deagle_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 8;
		break;
	case BARNEY_SHOTGUN:
		SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Shotgun_holstered);
		m_HEVBarneyGun = HEVBarneyGun::Shotgun_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 8;
		break;
	case BARNEY_MP5:
		SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::MP5_holstered);
		m_HEVBarneyGun = HEVBarneyGun::MP5_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 30;
		break;
	case BARNEY_NOGUN:
		SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::None);
		m_HEVBarneyGun = HEVBarneyGun::None;
		m_fGunDrawn = false;
		break;
	default:
		pev->weapons = BARNEY_GLOCK;
		SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Glock_holstered);
		m_HEVBarneyGun = HEVBarneyGun::Glock_holstered;
		m_fGunDrawn = false;
		m_iClipSize = 17;
		break;
	}

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_cAmmoLoaded = m_iClipSize;

	SetBodygroup(HEVBarneyBodyGroup::Head, m_HEVBarneyHead);
	SetBodygroup(HEVBarneyBodyGroup::Glasses, m_HEVBarneyGlasses);
	SetBodygroup(HEVBarneyBodyGroup::Helmet, m_HEVBarneyHelm);
	SetBodygroup(HEVBarneyBodyGroup::Gun, m_HEVBarneyGun);

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;


	MonsterInit();
	SetUse(&CHevBarney::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHevBarney::Precache()
{
	PRECACHE_MODEL("models/barney_hev.mdl");
	PRECACHE_SOUND("fvox/flatline.wav");

	PRECACHE_SOUND("barney/ba_pain1.wav");
	PRECACHE_SOUND("barney/ba_pain2.wav");
	PRECACHE_SOUND("barney/ba_pain3.wav");

	PRECACHE_SOUND("barney/ba_die1.wav");
	PRECACHE_SOUND("barney/ba_die2.wav");
	PRECACHE_SOUND("barney/ba_die3.wav");

	// every new barney must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();
	CTalkMonster::Precache();
}

void CHevBarney::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case HITGROUP_HEAD:
		flDamage *= gSkillData.monHead;
		break;

	case HITGROUP_STOMACH:
		UTIL_Sparks(ptr->vecEndPos);
		flDamage *= 0.6;
		break;

	case 10:
		if (m_HEVBarneyHelm == HEVBarneyHelm::Helm || m_HEVBarneyHelm == HEVBarneyHelm::Helm_glass)
		{
			if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
			{

				flDamage -= 20;
				if (flDamage <= 0)
				{
					UTIL_Ricochet(ptr->vecEndPos, 1.0);
					UTIL_Sparks(ptr->vecEndPos);
					flDamage *= 0.01;
				}
			}
		}
		else break;
	default:
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage -= 20;
			if (flDamage <= 0)
			{
				UTIL_Ricochet(ptr->vecEndPos, 1.0);
				UTIL_Sparks(ptr->vecEndPos);
				flDamage *= 0.01;
			}
		}
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CHevBarney::Killed(entvars_t* pevAttacker, int iGib)
{
	if (pev->spawnflags & SF_DONTDROPGUN)
	{
		return;
	}
	else
	{
		if (GetBodygroup(HEVBarneyBodyGroup::Gun) != HEVBarneyGun::None)
		{
			Vector vecGunPos;
			Vector vecGunAngles;

			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::None);
			m_HEVBarneyGun = HEVBarneyGun::None;

			GetAttachment(0, vecGunPos, vecGunAngles);

			if (pev->weapons == BARNEY_GLOCK)
			{
				CBaseEntity* pGun = DropItem("weapon_9mmhandgun", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_357)
			{
				CBaseEntity* pGun = DropItem("weapon_357", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_DEAGLE)
			{
				CBaseEntity* pGun = DropItem("weapon_eagle", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_MP5)
			{
				CBaseEntity* pGun = DropItem("weapon_9mmAR", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
			if (pev->weapons == BARNEY_SHOTGUN)
			{
				CBaseEntity* pGun = DropItem("weapon_shotgun", vecGunPos, vecGunAngles);
				pev->weapons = BARNEY_NOGUN;
			}
		}
	}

	UTIL_SetSize(pev, Vector(-33, -13, 0), Vector(33, 13, 0));
	UTIL_SetOrigin(pev, pev->origin);

	SetUse(NULL);
	CTalkMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//
// Returns number of events handled, 0 if none.
//=========================================================
void CHevBarney::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case BARNEY_AE_DRAW:
	{
		// barney's bodygroup switches here so he can pull gun from holster
		switch (pev->weapons)
		{
		case BARNEY_GLOCK:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Glock_drawn);
			m_HEVBarneyGun = HEVBarneyGun::Glock_drawn;
			break;
		case BARNEY_357:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Python_drawn);
			m_HEVBarneyGun = HEVBarneyGun::Python_drawn;
			break;
		case BARNEY_DEAGLE:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Deagle_drawn);
			m_HEVBarneyGun = HEVBarneyGun::Deagle_drawn;
			break;
		case BARNEY_SHOTGUN:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Shotgun_drawn);
			m_HEVBarneyGun = HEVBarneyGun::Shotgun_drawn;
			break;
		case BARNEY_MP5:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::MP5_drawn);
			m_HEVBarneyGun = HEVBarneyGun::MP5_drawn;
			break;
		}
		m_fGunDrawn = TRUE;
	}
	break;
	case BARNEY_AE_HOLSTER:
	{
		// change bodygroup to replace gun in holster
		switch (pev->weapons)
		{
		case BARNEY_GLOCK:

			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Glock_holstered);
			m_HEVBarneyGun = HEVBarneyGun::Glock_holstered;
			break;
		case BARNEY_357:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Python_holstered);
			m_HEVBarneyGun = HEVBarneyGun::Python_holstered;
			break;
		case BARNEY_DEAGLE:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Deagle_holstered);
			m_HEVBarneyGun = HEVBarneyGun::Deagle_holstered;
			break;
		case BARNEY_SHOTGUN:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::Shotgun_holstered);
			m_HEVBarneyGun = HEVBarneyGun::Shotgun_holstered;
			break;
		case BARNEY_MP5:
			SetBodygroup(HEVBarneyBodyGroup::Gun, HEVBarneyGun::MP5_holstered);
			m_HEVBarneyGun = HEVBarneyGun::MP5_holstered;
			break;
		}
		m_fGunDrawn = FALSE;
	}
	break;
	case BARNEY_AE_RELOAD:
	case BARNEY_AE_MELEE:
	case BARNEY_AE_SHOOT:
		CBarney::HandleAnimEvent(pEvent);
	break;

	default:
		CTalkMonster::HandleAnimEvent(pEvent);
	}
}

void CHevBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_HEVBarneyHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("helmet", pkvd->szKeyName))
	{
		m_HEVBarneyHelm = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("glasses", pkvd->szKeyName))
	{
		m_HEVBarneyGlasses = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CTalkMonster::KeyValue(pkvd);
	}
}

//=========================================================
// DEAD BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadBarney : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_PLAYER_ALLY; }

	void KeyValue(KeyValueData* pkvd) override;

	int m_BarneyHead;
	int m_BarneyHelm;
	int m_BarneyVest;
	int m_BarneyGlasses;

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[6];
};

const char* CDeadBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "laseridle", "dead_sitting" };

void CDeadBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_BarneyHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("vest", pkvd->szKeyName))
	{
		m_BarneyVest = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("helmet", pkvd->szKeyName))
	{
		m_BarneyHelm = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("glasses", pkvd->szKeyName))
	{
		m_BarneyGlasses = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_barney_dead, CDeadBarney);

//=========================================================
// ********** DeadBarney SPAWN **********
//=========================================================
void CDeadBarney::Spawn()
{
	PRECACHE_MODEL("models/barney.mdl");
	SET_MODEL(ENT(pev), "models/barney.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead barney with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8;

	if (m_BarneyHead == BarneyHead::Random)
	{
		m_BarneyHead = RANDOM_LONG(0, 7);
	}
	if (m_BarneyHelm == BarneyHelm::Random)
	{
		m_BarneyHelm = RANDOM_LONG(0, 4);
	}
	if (m_BarneyVest == BarneyVest::Random)
	{
		m_BarneyVest = RANDOM_LONG(0, 2);
	}
	if (m_BarneyGlasses == BarneyGlasses::Random)
	{
		m_BarneyGlasses = RANDOM_LONG(0, 2);
	}

	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, NUM_BARNEY_SKINS - 1);// pick a skin, any skin
	}

	SetBodygroup(BarneyBodyGroup::Gun, pev->weapons);
	SetBodygroup(BarneyBodyGroup::Head, m_BarneyHead);
	SetBodygroup(BarneyBodyGroup::Helmet, m_BarneyHelm);
	SetBodygroup(BarneyBodyGroup::Vest, m_BarneyVest);
	SetBodygroup(BarneyBodyGroup::Glasses, m_BarneyGlasses);

	MonsterInitDead();
}

//=========================================================
// DEAD HEV BARNEY PROP
//
// Designer selects a pose in worldcraft, 0 through num_poses-1
// this value is added to what is selected as the 'first dead pose'
// among the monster's normal animations. All dead poses must
// appear sequentially in the model file. Be sure and set
// the m_iFirstPose properly!
//
//=========================================================
class CDeadHEVBarney : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_PLAYER_ALLY; }

	void KeyValue(KeyValueData* pkvd) override;

	int m_HEVBarneyGun;
	int m_HEVBarneyHead;
	int m_HEVBarneyHelm;
	int m_BarneyGun;
	int m_HEVBarneyGlasses;

	int	m_iPose;// which sequence to display	-- temporary, don't need to save
	static const char* m_szPoses[6];
};

const char* CDeadHEVBarney::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "laseridle", "dead_sitting" };

void CDeadHEVBarney::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_HEVBarneyHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("helmet", pkvd->szKeyName))
	{
		m_HEVBarneyHelm = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("glasses", pkvd->szKeyName))
	{
		m_HEVBarneyGlasses = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}

LINK_ENTITY_TO_CLASS(monster_barney_hev_dead, CDeadHEVBarney);

//=========================================================
// ********** DeadHEVBarney SPAWN **********
//=========================================================
void CDeadHEVBarney::Spawn()
{
	PRECACHE_MODEL("models/barney_hev.mdl");
	SET_MODEL(ENT(pev), "models/barney_hev.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead barney with bad pose\n");
	}
	// Corpses have less health
	pev->health = 8;

		//choose random body and skin
	if (m_HEVBarneyHead == HEVBarneyHead::Random)
	{
		m_HEVBarneyHead = RANDOM_LONG(0, 7);
	}
	if (m_HEVBarneyGlasses == HEVBarneyGlasses::Random)
	{
		m_HEVBarneyGlasses = RANDOM_LONG(0, 2);
	}
	if (m_HEVBarneyHelm == HEVBarneyHelm::Random)
	{
		m_HEVBarneyHelm = RANDOM_LONG(0, 4);
	}

	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, NUM_HEVBARNEY_SKINS - 1);// pick a skin, any skin
	}


	SetBodygroup(HEVBarneyBodyGroup::Head, m_HEVBarneyHead);
	SetBodygroup(HEVBarneyBodyGroup::Glasses, m_HEVBarneyGlasses);
	SetBodygroup(HEVBarneyBodyGroup::Helmet, m_HEVBarneyHelm);
	SetBodygroup(HEVBarneyBodyGroup::Gun, pev->weapons);

	MonsterInitDead();
}
