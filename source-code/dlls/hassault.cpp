/***
*
*	Copyright (c) 2020, Magic Nipples.
*
*	Use and modification of this code is allowed as long
*	as credit is provided! Enjoy!
*
* -----------------------------------------
* Half-Life:Extended code by trvps (blsha), Copyright 2021. Provide credit
* -----------------------------------------
****/
//=========================================================
// Human Assault - Yore Dead
//=========================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"monsters.h"
#include	"squadmonster.h"

extern DLL_GLOBAL int		g_iSkillLevel;

namespace AssBodygroup
{
	enum AssBodygroup
	{
		Head = 2,
		Gun
	};
}
namespace Head
{
	enum Head
	{
		Random = -1,
		Gasmask = 0,
		Gasmask2,
		Bandana,
		Bandana2,
		Bandana3,
		Bandana4,
		Wolverine,
		Fancyhelmet,
		Fancyhelmet2,
	};
}

namespace Gun
{
	enum Gun
	{
		Minigun = 0,
		Unarmed
	};
}



//=========================================================
// Monster-specific Schedule Types
//=========================================================
enum
{
	SCHED_ASSAULT_SPINUP = LAST_COMMON_SCHEDULE + 1,
	SCHED_ASSAULT_SPINDOWN,
	SCHED_ASSAULT_FLINCH,
	SCHED_ASSAULT_MELEE,
	SCHED_ASSAULT_CHASE,
};

//=========================================================
// Monster-Specific tasks
//=========================================================
enum
{
	TASK_ASSAULT_SPINUP = LAST_COMMON_TASK + 2,
	TASK_ASSAULT_SPIN,
	TASK_ASSAULT_SPINDOWN,
};

//=========================================================
// Animation Events
//=========================================================
#define		HASSAULT_AE_SHOOT1		( 1 )
#define		HASSAULT_AE_SHOOT2		( 2 )
#define		HASSAULT_AE_SHOOT3		( 3 )
#define		HASSAULT_AE_SHOOT4		( 4 )
#define		HASSAULT_AE_SHOOT5		( 5 )
#define		HASSAULT_AE_SHOOT6		( 6 )
#define		HASSAULT_AE_SHOOT7		( 7 )
#define		HASSAULT_AE_SHOOT8		( 8 )
#define		HASSAULT_AE_SHOOT9		( 9 )
#define		HASSAULT_AE_MELEE		( 10 )

//=========================================================
// Let's define the class!
//=========================================================
class CHAssault : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	int Classify(void);
	void SetYawSpeed(void);
	void AlertSound(void);
	void DeathSound() override;
	void SpinDown(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void Killed(entvars_t* pevAttacker, int iGib);
	void FireGun(void);
	void Melee(void);
	void CallForBackup(char* szClassname, float flDist, EHANDLE hEnemy, Vector& vecLocation);

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	BOOL CheckRangeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack1(float flDot, float flDist);
	BOOL CheckMeleeAttack2(float flDot, float flDist);

	void StartTask(Task_t* pTask);
	Schedule_t* GetScheduleOfType(int Type);
	Schedule_t* GetSchedule(void);

	void KeyValue(KeyValueData* pkvd) override;

	inline Activity	GetActivity(void) { return m_Activity; }
	int m_iShell;

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

	int m_iGruntHead;
	int m_iWeaponIdx;

	static const char* pDieSounds[];
	static const char* pAlertSounds[];

private:
	int m_ifirestate;
	BOOL bAlerted;

};

const char* CHAssault::pDieSounds[] =
{
	"hassault/assault_die1.wav",
	"hassault/assault_die2.wav",
	"hassault/assault_die3.wav",
};
const char* CHAssault::pAlertSounds[] =
{
	"hassault/hw_alert1.wav",
	"hassault/hw_alert2.wav",
	"hassault/hw_alert3.wav",
	"hassault/hw_alert4.wav",
	"hassault/hw_alert.wav",
};

//=========================================================
// DieSound
//=========================================================
void CHAssault::DeathSound()
{
	EMIT_SOUND(ENT(pev), CHAN_VOICE, pDieSounds[RANDOM_LONG(0, ARRAYSIZE(pDieSounds) - 1)], 1.0, ATTN_NORM);
}


//=========================================================
// Tasks specific to this monster
//=========================================================

Task_t	tlAssaultSpinUp[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			0				},
	{ TASK_ASSAULT_SPINUP,		0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,		0.5						},
	{ TASK_ASSAULT_SPIN,		0				},
	{ TASK_SET_SCHEDULE,		(float)SCHED_RANGE_ATTACK1 },
};

Schedule_t slAssaultSpinUp[] =
{
	{
		tlAssaultSpinUp,
		ARRAYSIZE(tlAssaultSpinUp),
		0,
		0,
		"Assault Spin Up"
}
};

//=========================================================

Task_t	tlAssaultSpinDown[] =
{
	{ TASK_STOP_MOVING,			0	},
	{ TASK_ASSAULT_SPINDOWN,	0	},
	{ TASK_SET_SCHEDULE,		(float)SCHED_ASSAULT_CHASE },
	//{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_IDLE_STAND	},
};

Schedule_t slAssaultSpinDown[] =
{
	{
		tlAssaultSpinDown,
		ARRAYSIZE(tlAssaultSpinDown),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_HEAR_SOUND,

		bits_SOUND_WORLD |
		bits_SOUND_PLAYER,
		"Assault Spin Down"
}
};

//=========================================================

Task_t tlAssaultFlinch[] =
{
	{ TASK_ASSAULT_SPINDOWN,	0	},
	{ TASK_REMEMBER,			(float)bits_MEMORY_FLINCHED },
	{ TASK_STOP_MOVING,			0	},
	{ TASK_SMALL_FLINCH,		0	},
};

Schedule_t slAssaultFlinch[] =
{
	{
		tlAssaultFlinch,
		ARRAYSIZE(tlAssaultFlinch),
		0,
		0,
		"Assault Small Flinch"
	},
};

//=========================================================

Task_t	tlAssaultMeleeAttack[] =
{
	{ TASK_ASSAULT_SPINDOWN,	0				},
	{ TASK_STOP_MOVING,			0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slAssaultMeleeAttack[] =
{
	{
		tlAssaultMeleeAttack,
		ARRAYSIZE(tlAssaultMeleeAttack),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Assault Melee"
	},
};

//=========================================================

Task_t tlAssaultChase[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CHASE_ENEMY_FAILED	},
	{ TASK_GET_PATH_TO_ENEMY,	(float)0		},
	{ TASK_RUN_PATH,			(float)0		},
	{ TASK_WAIT_FOR_MOVEMENT,	(float)0		},
};

Schedule_t slAssaultChase[] =
{
	{
		tlAssaultChase,
		ARRAYSIZE(tlAssaultChase),
		bits_COND_NEW_ENEMY |
		bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_TASK_FAILED |
		bits_COND_HEAR_SOUND,

		bits_SOUND_PLAYER,
		"Assault Chase"
	},
};

//=========================================================

DEFINE_CUSTOM_SCHEDULES(CHAssault)
{
	slAssaultSpinUp,
		slAssaultSpinDown,
		slAssaultFlinch,
		slAssaultMeleeAttack,
		slAssaultChase,
};
IMPLEMENT_CUSTOM_SCHEDULES(CHAssault, CBaseMonster);


LINK_ENTITY_TO_CLASS(monster_human_assault, CHAssault);

TYPEDESCRIPTION	CHAssault::m_SaveData[] =
{
	DEFINE_FIELD(CHAssault, bAlerted, FIELD_BOOLEAN),
	DEFINE_FIELD(CHAssault, m_ifirestate, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CHAssault, CBaseMonster);

//=========================================================
// Spawn
//=========================================================
void CHAssault::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/hassault.mdl");

	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = gSkillData.hassaultHealth;
	pev->view_ofs = VEC_VIEW;	// position of the eyes relative to monster's origin.

	m_flFieldOfView = 0.3;	// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_bloodColor = BLOOD_COLOR_RED;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, 3);// pick a skin, any skin
	}

	if (m_iGruntHead == Head::Random)
	{
		m_iGruntHead = RANDOM_LONG(0, 8);
	}

	SetBodygroup(AssBodygroup::Head, m_iGruntHead);
	SetBodygroup(AssBodygroup::Gun, m_iWeaponIdx);

	m_ifirestate = -1;
	bAlerted = false;
	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CHAssault::Precache()
{
	int i;
	PRECACHE_MODEL("models/hassault.mdl");
	m_iShell = PRECACHE_MODEL("models/shell.mdl");

	PRECACHE_SOUND("hassault/hw_gun3.wav");
	PRECACHE_SOUND("hassault/hw_spin.wav");
	PRECACHE_SOUND("hassault/hw_spindown.wav");
	PRECACHE_SOUND("hassault/hw_spinup.wav");

	PRECACHE_SOUND("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND("weapons/cbar_hitbod3.wav");

	for (i = 0; i < ARRAYSIZE(pDieSounds); i++)
		PRECACHE_SOUND((char*)pDieSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char*)pAlertSounds[i]);
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHAssault::Classify(void)
{
	return m_iClass ? m_iClass : CLASS_HUMAN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CHAssault::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 150;
		break;

	case ACT_WALK:
		ys = 120;
		break;

	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 180;
		break;

	case ACT_RANGE_ATTACK1:
		ys = 120;
		break;

	case ACT_MELEE_ATTACK1:
		ys = 50;
		break;

	default:
		ys = 90;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// Human Assault yells like Mr.T
//=========================================================
void CHAssault::AlertSound()
{
	if (m_hEnemy != NULL)
	{
		//if (!bAlerted)
			EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), 1.0, ATTN_IDLE, 0, 100);

		CallForBackup("monster_human_grunt", 2048, m_hEnemy, m_vecEnemyLKP);

		//bAlerted = TRUE;
	}
}

void CHAssault::SpinDown()
{
	if (m_iWeaponIdx == Gun::Minigun)
	{
		if (m_ifirestate >= 0)
		{
			m_ifirestate = -1;
			STOP_SOUND(ENT(pev), CHAN_ITEM, "hassault/hw_spin.wav");
			STOP_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_gun3.wav");
			EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, "hassault/hw_spindown.wav", 1, ATTN_NORM, 0, 100);
		}
	}
}

//=========================================================
// Functions for each animation event
//=========================================================
void CHAssault::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case HASSAULT_AE_SHOOT1:
	case HASSAULT_AE_SHOOT2:
	case HASSAULT_AE_SHOOT3:
	case HASSAULT_AE_SHOOT4:
	case HASSAULT_AE_SHOOT5:
	case HASSAULT_AE_SHOOT6:
	case HASSAULT_AE_SHOOT7:
	case HASSAULT_AE_SHOOT8:
	case HASSAULT_AE_SHOOT9:

		switch (g_iSkillLevel)
		{
		default:
			pev->framerate = 1.75;
			break;

		case SKILL_HARD:
			pev->framerate = 2;
			break;
		case SKILL_HARDEST:
			pev->framerate = 2.35;
			break;
		}
		FireGun();
		break;


	case HASSAULT_AE_MELEE:
		Melee();
		break;

	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Stop all sounds when dead
//=========================================================
void CHAssault::Killed(entvars_t* pevAttacker, int iGib)
{
	SpinDown();

	SetUse(NULL);
	CBaseMonster::Killed(pevAttacker, iGib);
}

void CHAssault::FireGun()
{
	if (m_iWeaponIdx == Gun::Minigun)
	{
		Vector vecShootOrigin;

		UTIL_MakeVectors(pev->angles);
		vecShootOrigin = pev->origin + Vector(0, 0, 35);
		Vector vecShootDir = ShootAtEnemy(vecShootOrigin);
		pev->effects |= EF_MUZZLEFLASH;
		
		int BulletCount;
		if (RANDOM_LONG(0,25) < 1)// a chance to fire two bullets cause c'mon stop me
			BulletCount = 2;
		else
		{
			BulletCount = 1;
		}
		Vector	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT(40, 90) + gpGlobals->v_up * RANDOM_FLOAT(75, 200) + gpGlobals->v_forward * RANDOM_FLOAT(-40, 40);
		EjectBrass(vecShootOrigin - vecShootDir, vecShellVelocity, pev->angles.y, m_iShell, TE_BOUNCE_SHELL);
		FireBullets(BulletCount, vecShootOrigin, vecShootDir, VECTOR_CONE_15DEGREES, 2048, BULLET_MONSTER_MP5, 1);

		CSoundEnt::InsertSound(bits_SOUND_COMBAT, pev->origin, 384, 0.3);

		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "hassault/hw_gun3.wav", 1, ATTN_NORM, 0, 100);
	}
	
}

void CHAssault::Melee()
{
	CBaseEntity* pHurt = CheckTraceHullAttack(50, gSkillData.zombieDmgOneSlash, DMG_CLUB);
	if (pHurt)
	{
		if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
		{
			switch (RANDOM_LONG(0, 3))
			{
			case 0:
				pHurt->pev->punchangle.z = -15;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
				break;
			case 1:
				pHurt->pev->punchangle.z = 15;
				pHurt->pev->punchangle.y = 10;
				pHurt->pev->punchangle.x = -5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
				break;
			case 2:
				pHurt->pev->punchangle.z = -15;
				pHurt->pev->punchangle.y = -10;
				pHurt->pev->punchangle.x = 5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
				break;
			case 3:
				pHurt->pev->punchangle.z = 15;
				pHurt->pev->punchangle.x = -5;
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * -100;
				break;
			}
		}

		// Play a random attack hit sound
		switch (RANDOM_LONG(0, 2))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/cbar_hitbod1.wav", 1, ATTN_NORM, 0, 100); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/cbar_hitbod2.wav", 1, ATTN_NORM, 0, 100); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "weapons/cbar_hitbod3.wav", 1, ATTN_NORM, 0, 100); break;
		}
	}
}

void CHAssault::CallForBackup(char* szClassname, float flDist, EHANDLE hEnemy, Vector& vecLocation)
{
	//ALERT( at_console, "help\n" );

	// skip ones not on my netname
	if (FStringNull(pev->netname))
		return;

	CBaseEntity* pEntity = NULL;

	while ((pEntity = UTIL_FindEntityByString(pEntity, "netname", STRING(pev->netname))) != NULL)
	{
		float d = (pev->origin - pEntity->pev->origin).Length();
		if (d < flDist)
		{
			CBaseMonster* pMonster = pEntity->MyMonsterPointer();
			if (pMonster)
			{
				pMonster->m_afMemory |= bits_MEMORY_PROVOKED;
				pMonster->PushEnemy(hEnemy, vecLocation);
			}
		}
	}
}

//=========================================================
// shoot while are melee range | UNDONE!!
//=========================================================
BOOL CHAssault::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_iWeaponIdx != Gun::Minigun)
	{
		return FALSE;
	}
	if (flDot > 0.5)
		//if (flDot > 0.5 && flDist > 70)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Added 0.52 melee animatons so don't just return false
//=========================================================
BOOL CHAssault::CheckMeleeAttack1(float flDot, float flDist)
{
	//return FALSE;
	if (flDot > 0.5 && flDist < 70)
	{
		return TRUE;
	}
	return FALSE;
}

BOOL CHAssault::CheckMeleeAttack2(float flDot, float flDist)
{
	return FALSE;
}

void CHAssault::StartTask(Task_t* pTask)
{
	//ALERT(at_console, "m_ifirestate %i\n", m_ifirestate);
	switch (pTask->iTask)
	{
	case TASK_ASSAULT_SPINUP:
		if (m_ifirestate == -1)
		{
			m_ifirestate = 0;
			STOP_SOUND(ENT(pev), CHAN_WEAPON, "hassault/hw_gun3.wav");
			EMIT_SOUND_DYN(ENT(pev), CHAN_AUTO, "hassault/hw_spinup.wav", 1, ATTN_NORM, 0, 100);
		}
		TaskComplete();
		break;

	case TASK_ASSAULT_SPIN:
		m_ifirestate = 1;
		STOP_SOUND(ENT(pev), CHAN_AUTO, "hassault/hw_spinup.wav");
		EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "hassault/hw_spin.wav", 1, ATTN_NORM, 0, 100);
		TaskComplete();
		break;

	case TASK_ASSAULT_SPINDOWN:
		SpinDown();
		TaskComplete();
		break;


	default:
		CBaseMonster::StartTask(pTask);
		break;
	}
}

//=========================================================
// Any custom shedules?
//=========================================================
Schedule_t* CHAssault::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_ASSAULT_SPINUP:
		return slAssaultSpinUp;
		break;

	case SCHED_ASSAULT_SPINDOWN:
		return slAssaultSpinDown;
		break;

	case SCHED_SMALL_FLINCH:
	case SCHED_ASSAULT_FLINCH:
		return slAssaultFlinch;
		break;

	case SCHED_MELEE_ATTACK1:
	case SCHED_ASSAULT_MELEE:
		return slAssaultMeleeAttack;
		break;

	case SCHED_CHASE_ENEMY:
	case SCHED_ASSAULT_CHASE:
		return slAssaultChase;

	}
	return CBaseMonster::GetScheduleOfType(Type);

}

//=========================================================
// Load up the schedules so ai isn't dumb
//=========================================================
Schedule_t* CHAssault::GetSchedule(void)
{
	// Call another switch class, to check the monster's attitude
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		SpinDown();
		break;

	case MONSTERSTATE_COMBAT:
	{
		if (HasConditions(bits_COND_NEW_ENEMY))
			AlertSound();

		if (HasConditions(bits_COND_ENEMY_DEAD) || HasConditions(bits_COND_ENEMY_TOOFAR))
		{
			SpinDown();
			return CBaseMonster::GetSchedule();
		}

		if (HasConditions(bits_COND_LIGHT_DAMAGE) && !HasMemory(bits_MEMORY_FLINCHED))
			return GetScheduleOfType(SCHED_ASSAULT_FLINCH);

		if ((HasConditions(!bits_COND_SEE_ENEMY)) || (HasConditions(bits_COND_ENEMY_OCCLUDED)))
			return GetScheduleOfType(SCHED_ASSAULT_SPINDOWN);

		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
			return GetScheduleOfType(SCHED_ASSAULT_MELEE);

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			if (m_ifirestate == -1)
				return GetScheduleOfType(SCHED_ASSAULT_SPINUP);
			if (m_ifirestate == 0)
				return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}
	}
	break;

	}
	//if all else fails, the base probably knows what to do
	return CBaseMonster::GetSchedule();
}

//=========================================================
// TraceAttack - make sure we're not taking it in the armour
//=========================================================
void CHAssault::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	// check for helmet shot
	if (ptr->iHitgroup == 11)
	{
		// absorb damage
		flDamage -= 20;
		if (flDamage <= 0)
		{
			UTIL_Ricochet(ptr->vecEndPos, 1.0);
			flDamage = 0.01;
		}
		
	}
	CBaseMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CHAssault::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_iGruntHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("gun", pkvd->szKeyName))
	{
		m_iWeaponIdx = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}