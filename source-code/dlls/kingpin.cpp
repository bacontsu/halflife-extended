//===============================================================
//
// HL:E KINGPIN by tear
//
//===============================================================

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"weapons.h"
#include	"soundent.h"
#include	"monsters.h"
#include	"effects.h"
#include	"animation.h"
#include	"schedule.h"
#include	"game.h"

extern DLL_GLOBAL int		g_iSkillLevel;

//=========================================================
// Monster-specific Schedule Types
//=========================================================
enum
{
	SCHED_KINGPIN_TELE = LAST_COMMON_SCHEDULE + 1,
	SCHED_KINGPIN_REVIVE,
	SCHED_KINGPIN_FLINCH,
	SCHED_KINGPIN_MELEE,
	SCHED_KINGPIN_STOPMAGE,
};

//=========================================================
// Monster-Specific tasks
//=========================================================
enum
{
	TASK_KINGPIN_TELE = LAST_COMMON_TASK + 1,
	TASK_KINGPIN_REVIVE,
};

//=========================================================
// Tasks specific to this monster
//=========================================================

//----------Reviving--------------
Task_t tlKingpinRevive[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_ARM},//raise hands
	{ TASK_REMEMBER, (float)bits_MEMORY_CUSTOM1},//remember we're doing wizardry
	{ TASK_KINGPIN_REVIVE, (float)0},//magic
	{ TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY},//shake
	{ TASK_FORGET, (float)bits_MEMORY_CUSTOM1},//we're not doing wizardry anymore
	{ TASK_PLAY_SEQUENCE, (float)ACT_DISARM},//lower hands
	
};

Schedule_t slKingpinRevive[] =
{
	{
		tlKingpinRevive,
		ARRAYSIZE(tlKingpinRevive),
		bits_COND_CAN_MELEE_ATTACK1,
		0,
		"Kingpin Revive"
	}
};

Task_t tlKingpinStopMaging[] =
{
	{ TASK_STOP_MOVING, (float)0 },
	{ TASK_FORGET, (float)bits_MEMORY_CUSTOM1},//we're not doing wizardry anymore
	{ TASK_PLAY_SEQUENCE, (float)ACT_DISARM},//lower hands

};

Schedule_t slKingpinStopMaging[] =
{
	{
		tlKingpinStopMaging,
		ARRAYSIZE(tlKingpinStopMaging),
		//bits_COND_CAN_MELEE_ATTACK1 |
		//bits_COND_CAN_RANGE_ATTACK2 |
		//bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		0,
		"Kingpin Stop"
	}
};

//----------Telepathic Attack--------------
Task_t tlKingpinTeleAttack[] =
{
	{ TASK_STOP_MOVING, (float)0},
	{ TASK_PLAY_SEQUENCE, (float)ACT_ARM},//raise hands
	{ TASK_REMEMBER, (float)bits_MEMORY_CUSTOM1},//remember we're doing wizardry
	{ TASK_KINGPIN_TELE, (float)0},//magic
	{ TASK_PLAY_SEQUENCE, (float)ACT_THREAT_DISPLAY},//shake
	{ TASK_FORGET, (float)bits_MEMORY_CUSTOM1},//we're not doing wizardry anymore
	{ TASK_PLAY_SEQUENCE, (float)ACT_DISARM},//lower hands
};

Schedule_t slKingpinTeleAttack[] =
{
	{
		tlKingpinTeleAttack,
		ARRAYSIZE(tlKingpinTeleAttack),
		bits_COND_CAN_MELEE_ATTACK1,
	//	bits_COND_CAN_RANGE_ATTACK2,
		0,
		"Kingpin Telepathic Attack"
	}
};

//Following schedules fire if we are maging and suddenly need to flinch/slash, if we are not maging base flinch/melee schedules will fire

//----------Melee Attack--------------
Task_t	tlKingpinMeleeAttack[] =
{
	{ TASK_FORGET, (float)bits_MEMORY_CUSTOM1},//we're not doing wizardry anymore
	{ TASK_PLAY_SEQUENCE, (float)ACT_DISARM},//lower hands
	{ TASK_STOP_MOVING,			(float)0				},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_MELEE_ATTACK1,		(float)0		},
};

Schedule_t	slKingpinMeleeAttack[] =
{
	{
		tlKingpinMeleeAttack,
		ARRAYSIZE(tlKingpinMeleeAttack),
		bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED,
		0,
		"Kingpin Melee"
	},
};

//----------Flinch--------------
Task_t tlKingpinFlinch[] =
{
	{ TASK_FORGET, (float)bits_MEMORY_CUSTOM1},//we're not doing wizardry anymore
	{ TASK_PLAY_SEQUENCE, (float)ACT_DISARM},//lower hands
	{ TASK_REMEMBER,			(float)bits_MEMORY_FLINCHED },
	{ TASK_STOP_MOVING,			(float)0	},
	{ TASK_SMALL_FLINCH,		(float)0	},
};

Schedule_t slKingpinFlinch[] =
{
	{
		tlKingpinFlinch,
		ARRAYSIZE(tlKingpinFlinch),
		0,
		0,
		"Kingpin Small Flinch"
	},
};


//=========================================================
// Animation Events
//=========================================================
#define		KINGPIN_AE_CLAWLEFT		( 1 )
#define		KINGPIN_AE_CLAWRIGHT	( 2 )
#define		KINGPIN_AE_CLAWBOTH		( 3 )


//=========================================================
// Class definition
//=========================================================
class CKpin : public CBaseMonster
{
public:
	void Spawn(void);
	void Precache(void);
	int Classify(void);
	void SetYawSpeed(void);

	void AlertSound(void);
	void PainSound() override;
	void IdleSound() override;

	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void Killed(entvars_t* pevAttacker, int iGib);

	void ClawAttackLeft();
	void ClawAttackRight();
	void ClawAttackBoth();

	void TelekinesisAttack();
	void ResurrectRitual();

	BOOL CheckRangeAttack1(float flDot, float flDist) override;
	BOOL CheckRangeAttack2(float flDot, float flDist) override;
	BOOL CheckMeleeAttack1(float flDot, float flDist) override;

	int TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

	inline Activity	GetActivity(void) { return m_Activity; }

	void StartTask(Task_t* pTask);
	Schedule_t* GetScheduleOfType(int Type);
	Schedule_t* GetSchedule(void);

	

	int ReviveTarget;

	virtual int		Save(CSave& save);
	virtual int		Restore(CRestore& restore);
	static	TYPEDESCRIPTION m_SaveData[];

	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];

	CBaseEntity* m_pDead;


	CUSTOM_SCHEDULES;

	float m_flNextAttack;
	float m_flNextResurrectCheck;// revive ability check is pretty costly so we will do it only every once in a while
};



TYPEDESCRIPTION	CKpin::m_SaveData[] =
{
	//DEFINE_FIELD(CKpin, Maging, FIELD_BOOLEAN),
	DEFINE_FIELD(CKpin, m_flNextAttack, FIELD_FLOAT),
	DEFINE_FIELD(CKpin, m_flNextResurrectCheck, FIELD_FLOAT),
};

IMPLEMENT_SAVERESTORE(CKpin, CBaseMonster);

LINK_ENTITY_TO_CLASS(monster_kingpin, CKpin);

DEFINE_CUSTOM_SCHEDULES(CKpin)
{
	slKingpinFlinch,
		slKingpinMeleeAttack,
		slKingpinTeleAttack,
		slKingpinRevive,
		slKingpinStopMaging,
};
IMPLEMENT_CUSTOM_SCHEDULES(CKpin, CBaseMonster);

#define REVIVE_VORT (1)
#define REVIVE_SQUID (2)
#define REVIVE_CRAB (3)
#define REVIVE_HOUNDEYE (4)
#define REVIVE_CONTROLLER (5)

const char* CKpin::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};
const char* CKpin::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* CKpin::pIdleSounds[] =
{
	"kingpin/kp_idle1.wav",
	"kingpin/kp_idle2.wav",
};

const char* CKpin::pPainSounds[] =
{
	"kingpin/kp_pain1.wav",
	"kingpin/kp_pain2.wav",
};

const char* CKpin::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};


void CKpin::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}
void CKpin::PainSound()
{
	int pitch = 95 + RANDOM_LONG(0, 9);

	if (RANDOM_LONG(0, 5) < 2)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pPainSounds[RANDOM_LONG(0, ARRAYSIZE(pPainSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}
//=========================================================
// Spawn
//=========================================================
void CKpin::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/kingpin.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->health = gSkillData.kpinHealth;
	pev->max_health = pev->health;
	pev->view_ofs = VEC_VIEW;	// position of the eyes relative to monster's origin.

	m_flFieldOfView = 0.5;	// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_bloodColor = BLOOD_COLOR_YELLOW;
	m_MonsterState = MONSTERSTATE_NONE;

	m_afCapability = bits_CAP_HEAR |
		bits_CAP_RANGE_ATTACK1 |
		bits_CAP_RANGE_ATTACK2 |
		bits_CAP_MELEE_ATTACK1;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CKpin::Precache()
{
	PRECACHE_MODEL("models/kingpin.mdl");

	PRECACHE_MODEL("sprites/Fexplo1.spr");
	PRECACHE_SOUND("debris/beamstart2.wav");
	PRECACHE_SOUND("kingpin/kp_attack.wav");
	PRECACHE_SOUND("kingpin/kp_walk.wav");
	PRECACHE_SOUND("kingpin/kp_slither.wav");
	PRECACHE_SOUND("debris/beamstart7.wav");

	int i;

	for (i = 0; i < ARRAYSIZE(pIdleSounds); i++)
		PRECACHE_SOUND((char*)pIdleSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAlertSounds); i++)
		PRECACHE_SOUND((char*)pAlertSounds[i]);

	for (i = 0; i < ARRAYSIZE(pPainSounds); i++)
		PRECACHE_SOUND((char*)pPainSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackHitSounds); i++)
		PRECACHE_SOUND((char*)pAttackHitSounds[i]);

	for (i = 0; i < ARRAYSIZE(pAttackMissSounds); i++)
		PRECACHE_SOUND((char*)pAttackMissSounds[i]);

}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CKpin::Classify(void)
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MILITARY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CKpin::SetYawSpeed(void)
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
// Kpin screeches and hisses
//=========================================================
void CKpin::AlertSound()
{
	if (m_hEnemy != NULL)
	{
		int pitch = 95 + RANDOM_LONG(0, 9);

		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG(0, ARRAYSIZE(pAlertSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
	}
}

//=========================================================
// Functions for each animation event
//=========================================================
void CKpin::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	switch (pEvent->event)
	{
	case KINGPIN_AE_CLAWLEFT:
		ClawAttackLeft();
		break;
	case KINGPIN_AE_CLAWRIGHT:
		ClawAttackRight();
		break;
	case KINGPIN_AE_CLAWBOTH:
		ClawAttackBoth();
		break;


	default:
		CBaseMonster::HandleAnimEvent(pEvent);
	}
}

//=========================================================
// Stop all sounds when dead
//=========================================================
void CKpin::Killed(entvars_t* pevAttacker, int iGib)
{
	SetUse(NULL);
	CBaseMonster::Killed(pevAttacker, iGib);
}

//=========================================================
// Checking Melee attacks
//=========================================================
BOOL CKpin::CheckMeleeAttack1(float flDot, float flDist)
{
	if (pev->health > (pev->max_health * 0.5) && flDist <= 96 && flDot >= 0.7)
	{
		TelekinesisAttack();
		return FALSE;
	}
	if (flDist <= 96 && flDot >= 0.7)
	{
		return TRUE;
	}
	return FALSE;
}

//=========================================================
// Melee attacks
//=========================================================

// swipe left
void CKpin::ClawAttackLeft()
{
	int SlashDmg = gSkillData.zombieDmgOneSlash;
	

	CBaseEntity* pHurt = CheckTraceHullAttack(70, SlashDmg, DMG_SLASH);
	if (pHurt)
	{
		if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
		{
			pHurt->pev->punchangle.z = 18;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
		}
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

	//m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(0.2, 0.5);// give player a break and don't attack again for a moment
}

//swipe right
void CKpin::ClawAttackRight()
{
	int SlashDmg = gSkillData.zombieDmgOneSlash;
	
	CBaseEntity* pHurt = CheckTraceHullAttack(70, SlashDmg, DMG_SLASH);
	if (pHurt)
	{
		if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
		{
			pHurt->pev->punchangle.z = -18;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
		}
		// Play a random attack hit sound
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	else // Play a random attack miss sound
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

	//m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(0.2, 0.5);// give player a break and don't attack again for a moment
}

//swipe both
void CKpin::ClawAttackBoth()
{
	int SlashDmg = gSkillData.zombieDmgBothSlash;

	CBaseEntity* pHurt = CheckTraceHullAttack(70, SlashDmg, DMG_SLASH);
	if (pHurt)
	{
		if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
		{
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
		}
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackHitSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));
	}
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG(0, ARRAYSIZE(pAttackMissSounds) - 1)], 1.0, ATTN_NORM, 0, 100 + RANDOM_LONG(-5, 5));

	//m_flNextAttack = gpGlobals->time + RANDOM_FLOAT(0.2, 0.5);// give player a break and don't attack again for a moment
}

//=========================================================
// Checking Range attacks
//=========================================================

//check for magic push attack
BOOL CKpin::CheckRangeAttack1(float flDot, float flDist)
{
	if (m_flNextAttack > gpGlobals->time)
	{
		return FALSE;
	}
	if (flDist > 96 && flDist <= 400 && flDot >= 0.5)
	{
		return TRUE;
	}
	else
		return FALSE;
}

//check for resurrect "attack"
BOOL CKpin::CheckRangeAttack2(float flDot, float flDist)
{
	if (m_flNextResurrectCheck > gpGlobals->time)
	{
		return FALSE;
	}

	flDist = 512;

	m_pDead = nullptr;

	CBaseEntity* pEntity = nullptr;
	CBaseEntity* pObstacle = nullptr;
	const char* pSearchTarget;

	// randomly decide whom are we going to look for
	switch (RANDOM_LONG(1, 5))
	{
	case 1:
		pSearchTarget = "monster_alien_slave";
		break;

	case 2:
		pSearchTarget = "monster_bullchicken";
		break;

	case 3:
		pSearchTarget = "monster_houndeye";
		break;

	case 4:
		pSearchTarget = "monster_headcrab";
		break;

	case 5:
		pSearchTarget = "monster_controller";
		break;
	}
		 
	// find dead bodies
	if ((pEntity = UTIL_FindEntityByClassname(pEntity, pSearchTarget)) != nullptr)
	{
		if (pEntity->pev->deadflag != DEAD_NO)
		{
			float d = (pev->origin - pEntity->Center()).Length();
			if (d < flDist)
			{
				m_pDead = pEntity;
				flDist = d;
			}
		}
	}

	// make sure nothing gets stuck in our revived targets
	if (m_pDead != nullptr)
	{
		if (pObstacle = UTIL_FindEntityInSphere(pEntity, pEntity->pev->origin, 64))
		{
			return FALSE;
		}
	}
	


	if (m_pDead != nullptr && flDist < 512)
	{
		return TRUE;
	}
	return FALSE;

	m_flNextResurrectCheck = gpGlobals->time + 3;// check only every 3 seconds
}
//=========================================================
// Range attacks
//=========================================================

//telekinesis attack
void CKpin::TelekinesisAttack()
{
	float flDmg;
	float flDist;
	EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "kingpin/kp_attack.wav", 1, ATTN_NORM, 0, 100);

	CBaseEntity* pEntity = NULL;
	// iterate on all entities in the vicinity.
	while ((pEntity = UTIL_FindEntityInSphere(pEntity, pev->origin, 400)) != NULL)
	{
		if (pEntity->IsPlayer())
		{
			flDmg = gSkillData.kpinDmgTele;

			flDist = (pEntity->Center() - pev->origin).Length();

			flDmg -= (flDist / 400) * flDmg;

			if (flDmg > 0)
			{
				pEntity->TakeDamage(pev, pev, flDmg, DMG_SONIC | DMG_NEVERGIB);
			}

			UTIL_ScreenShake(pEntity->pev->origin, 25.0, 1.5, 0.7, 2);
			UTIL_MakeVectors(pev->angles);
			pEntity->pev->velocity = pEntity->pev->velocity + gpGlobals->v_forward * 400 + gpGlobals->v_up * 400;
		}
	}
	m_flNextAttack = gpGlobals->time + 1.5;
}

//resurrect "attack"
void CKpin::ResurrectRitual()
{
	// oops, we somehow got here but forgot whom we wanted to revive
	if (m_pDead == nullptr)
	{
		// reset and pretend nothing embarassing happened
		m_flNextResurrectCheck = gpGlobals->time + 0.1;
		return;
	}
		
	Vector EffectPlace = m_pDead->pev->origin;
	Vector CorpseAngles = m_pDead->pev->angles;
	const char* CorpseClass = STRING(m_pDead->pev->classname);
	edict_t* pos;

	pos = m_pDead->edict();

	CBaseEntity::Create(CorpseClass, EffectPlace, CorpseAngles);


	//some effects
	int iBeams = 0;

	TraceResult tr;
	CBeam* pBeam[8];

	//cloud
	EMIT_SOUND(pos, CHAN_BODY, "debris/beamstart2.wav", 1, ATTN_NORM);
	UTIL_ScreenShake(EffectPlace, 6, 160, 1.0, pev->button);
	CSprite* pSpr = CSprite::SpriteCreate("sprites/Fexplo1.spr", EffectPlace, TRUE);
	pSpr->AnimateAndDie(18);
	pSpr->SetTransparency(kRenderGlow, 77, 210, 130, 255, kRenderFxNoDissipation);

	//beams
	EMIT_SOUND(pos, CHAN_ITEM, "debris/beamstart7.wav", 1, ATTN_NORM);

	while (iBeams < 4)
	{
		Vector vecDest = pev->origin + gpGlobals->v_up * 300;
		UTIL_TraceLine(EffectPlace, EffectPlace + vecDest, ignore_monsters, NULL, &tr);

		pBeam[iBeams] = CBeam::BeamCreate("sprites/lgtning.spr", 30);
		pBeam[iBeams]->PointsInit(EffectPlace, tr.vecEndPos);
		pBeam[iBeams]->SetColor(142, 0, 224);
		pBeam[iBeams]->SetNoise(65);
		pBeam[iBeams]->SetBrightness(220);
		pBeam[iBeams]->SetThink(&CBeam::SUB_Remove);
		pBeam[iBeams]->pev->nextthink = gpGlobals->time + RANDOM_FLOAT(0.5, 1.6);
		iBeams++;
	}
	


	

	UTIL_Remove(m_pDead);
}

void CKpin::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_KINGPIN_REVIVE:
		ResurrectRitual();
		m_iTaskStatus = TASKSTATUS_COMPLETE;
		break;
	case TASK_KINGPIN_TELE:
		TelekinesisAttack();
		m_iTaskStatus = TASKSTATUS_COMPLETE;
		break;
	default:
		CBaseMonster::StartTask(pTask);
		break;
	}

}

//=========================================================
// Any custom shedules?
//=========================================================
Schedule_t* CKpin::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_KINGPIN_TELE:
		return slKingpinTeleAttack;
		break;

	case SCHED_KINGPIN_REVIVE:
		return slKingpinRevive;
		break;

	case SCHED_KINGPIN_FLINCH:
		return slKingpinFlinch;
		break;

	case SCHED_KINGPIN_MELEE:
		return slKingpinMeleeAttack;
		break;

	case SCHED_KINGPIN_STOPMAGE:
		return slKingpinStopMaging;
		break;

	}
	return CBaseMonster::GetScheduleOfType(Type);
}

//=========================================================
// Load up the schedules so ai isn't dumb
//=========================================================
Schedule_t* CKpin::GetSchedule(void)
{
	// Call another switch class, to check the monster's attitude
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if (HasMemory(bits_MEMORY_CUSTOM1))// why are we maging outside of combat?
		{
			return GetScheduleOfType(SCHED_KINGPIN_STOPMAGE);
		}

		break;

	case MONSTERSTATE_COMBAT:
	{
		if (HasConditions(bits_COND_NEW_ENEMY))
			AlertSound();

		// if we were maging then before flinching we should stop maging
		if (HasConditions(bits_COND_LIGHT_DAMAGE) && !HasMemory(bits_MEMORY_FLINCHED) && HasMemory(bits_MEMORY_CUSTOM1))
		{;
			return GetScheduleOfType(SCHED_KINGPIN_FLINCH);
		}
		else if (HasConditions(bits_COND_LIGHT_DAMAGE) && !HasMemory(bits_MEMORY_FLINCHED) && !HasMemory(bits_MEMORY_CUSTOM1))
			return GetScheduleOfType(SCHED_SMALL_FLINCH);

		// since it's a telepathic attack we don't care about player's position as long as they're within range
		if (((HasConditions(!bits_COND_SEE_ENEMY)) || (HasConditions(bits_COND_ENEMY_OCCLUDED))) && HasConditions(bits_COND_CAN_RANGE_ATTACK1))
			return GetScheduleOfType(SCHED_KINGPIN_TELE);

		// if we were maging then before meleeing we should stop maging
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1) && HasMemory(bits_MEMORY_CUSTOM1))
		{
			return GetScheduleOfType(SCHED_KINGPIN_MELEE);
		}
		else if (HasConditions(bits_COND_CAN_MELEE_ATTACK1) && !HasMemory(bits_MEMORY_CUSTOM1))
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_KINGPIN_TELE);
		}
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK2))
		{
			return GetScheduleOfType(SCHED_KINGPIN_REVIVE);
		}
	}
	break;

	}
	//if all else fails, the base probably knows what to do
	return CBaseMonster::GetSchedule();
}

int CKpin::TakeDamage(entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
{
	// Take 40% damage from explosives
	if (bitsDamageType == DMG_BLAST)
	{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce(flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.4;
	}

	// HACK HACK -- until we fix this.
	return CBaseMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}