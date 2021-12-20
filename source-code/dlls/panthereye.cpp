/************************************************************
*															*
*			Panthereye AKA Diablo

 Originally made by JujU from HL: Invasion team
 all credit for the initial code goes to them

 Modified and adjusted by tear for Half-Life: Extended
*															*
************************************************************/


#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include    "weapons.h"
#include	"game.h"
#include	"squadmonster.h"

//--------------------------------------
//this is from hl-invasion extdll.h
//but we'll define it right here since this is one of the only pieces of code that need all that
#ifndef max
#define max(a,b)    (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)    (((a) < (b)) ? (a) : (b))
#endif
//--------------------------------------

//=====================================================
// Monster's anim events
// Constantes associées à plusieurs animations
//=====================================================

#define DIABLO_AE_KICK_NORMAL			( 1 )
#define DIABLO_AE_KICK_LOW				( 2 )
#define DIABLO_AE_KICK_HIGH				( 3 )
#define DIABLO_AE_STEP					( 4 )


//=====================================================
//  Schedule types :
//	Panther can leap when stuff happens
//=====================================================
enum
{
	SCHED_DIABLO_LEAP = LAST_COMMON_SCHEDULE + 1,
	SCHED_DIABLO_JUMP_AT_ENEMY_UNSTUCK,
};

//=========================================================
// monster-specific tasks
//=========================================================
enum
{
	TASK_DIABLO_DETERMINE_UNSTUCK_IDEAL = LAST_COMMON_TASK + 1,
	TASK_DIABLO_ATTEMPT_ROUTE,
	TASK_DIABLO_USE_ROUTE
};


//=====================================================
//Définition de la classe
//=====================================================


class CDiablo : public CSquadMonster
{
public:
	void Spawn(void);					// initialisation
	void Precache(void);				// on précache les sonds et les models
	void SetYawSpeed(void);			// vitesse de rotation
	int  Classify(void);				// "camp" du monstre : alien ou humain

	void HandleAnimEvent(MonsterEvent_t* pEvent);
	Schedule_t* GetSchedule(void);						// analyse des bit_COND_ ...
	Schedule_t* GetScheduleOfType(int Type);			// ... et en retourne un comportement
	void StartTask(Task_t* pTask) override;
	void RunTask(Task_t* pTask) override;

	void EXPORT LeapTouch(CBaseEntity* pOther);

	BOOL CheckRangeAttack1(float flDot, float flDist) override;
	BOOL CheckMeleeAttack1(float flDot, float flDist);


	CBaseEntity* KickNormal(void);
	CBaseEntity* KickLow(void);
	CBaseEntity* KickHigh(void);
	void Jump();

	void IdleSound(void);
	void DeathSound(void);

	static const char* pIdleSounds[];

	void SonFrappe(BOOL Touche);

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType);

	float NextLeapTime;

	CUSTOM_SCHEDULES;										// définit tous les array de comportements

	//===
};
LINK_ENTITY_TO_CLASS(monster_panthereye, CDiablo);

const char* CDiablo::pIdleSounds[] =
{
	"panthereye/pa_idle1.wav",
	"panthereye/pa_idle2.wav",
	"panthereye/pa_idle3.wav",
	"panthereye/pa_idle4.wav",
};

void CDiablo::IdleSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	// Play a random idle sound
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG(0, ARRAYSIZE(pIdleSounds) - 1)], 1.0, ATTN_NORM, 0, pitch);
}
void CDiablo::DeathSound()
{
	int pitch = 100 + RANDOM_LONG(-5, 5);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "panthereye/pa_death1.wav", 1.0, ATTN_NORM, 0, pitch);
}
//====================================================
// Classification
//====================================================

int	CDiablo::Classify(void)
{
	return m_iClass ? m_iClass : CLASS_ALIEN_MONSTER;	// ami avec les aliens
}

//====================================================
// Vitesse de rotation
//====================================================

void CDiablo::SetYawSpeed(void)
{
	int ys;

	switch (m_Activity)
	{
	case ACT_IDLE:
		ys = 200;
		break;
	case ACT_RUN:
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 200;
		break;
	case ACT_RANGE_ATTACK1:
		ys = 170;
		break;
	default:
		ys = 200;
		break;
	}

	pev->yaw_speed = ys;
}

//====================================================
// Initialisation
//====================================================

void CDiablo::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/panthereye.mdl");// model

	UTIL_SetSize(pev, Vector(-28, -48, 0), Vector(28, 48, 80));

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->health = gSkillData.panthereyeHealth;
	pev->view_ofs		= Vector ( 0, 0, 60 );// position of the eyes relative to monster's origin.
	m_flFieldOfView = 0.5;
	m_MonsterState = MONSTERSTATE_NONE;

	NextLeapTime = 0;

	SetActivity(ACT_IDLE);

	//=========
	m_afCapability = bits_CAP_HEAR |
		bits_CAP_SQUAD |	
		bits_CAP_RANGE_ATTACK1 |	
		bits_CAP_DOORS_GROUP;
	pev->effects = 0;
	//=========

	MonsterInit();
}

//==================
//==================

void CDiablo::Precache()
{
	PRECACHE_MODEL("models/panthereye.mdl");

	PRECACHE_SOUND("zombie/claw_miss1.wav");
	PRECACHE_SOUND("zombie/claw_miss2.wav");
	PRECACHE_SOUND("panthereye/pa_death1.wav");
	PRECACHE_SOUND("panthereye/pa_attack1.wav");

	PRECACHE_SOUND("zombie/claw_strike1.wav");
	PRECACHE_SOUND("zombie/claw_strike2.wav");
	PRECACHE_SOUND("zombie/claw_strike3.wav");

	int i;
	for (i = 0; i < ARRAYSIZE(pIdleSounds); i++)
		PRECACHE_SOUND((char*)pIdleSounds[i]);


}

//=================================================
//	Check Attacks
//=================================================

BOOL CDiablo::CheckRangeAttack1(float flDot, float flDist)
{
	if (NextLeapTime > gpGlobals->time)
	{
		return FALSE;
	}

	if ( flDist > 128 && flDot >= 0.5)
	{
		return TRUE;
	}
	
	return FALSE;
}



BOOL CDiablo::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDist <= 110 && flDot >= 0.7 )
	{
		return TRUE;
	}
	return FALSE;
}

//==================================================
// Coups
//==================================================

CBaseEntity* CDiablo::KickNormal(void)
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += 42;

	Vector vecEnd = vecStart + (gpGlobals->v_forward * 64);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr); 

	if (tr.pHit)		
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;		
}


CBaseEntity* CDiablo::KickLow(void)
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += 30;

	Vector vecEnd = vecStart + (gpGlobals->v_forward * 64);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}


CBaseEntity* CDiablo::KickHigh(void)
{
	TraceResult tr;

	UTIL_MakeVectors(pev->angles);
	Vector vecStart = pev->origin;
	vecStart.z += 42;

	Vector vecEnd = vecStart + (gpGlobals->v_forward * 92);

	UTIL_TraceHull(vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT(pev), &tr);

	if (tr.pHit)
	{
		CBaseEntity* pEntity = CBaseEntity::Instance(tr.pHit);
		return pEntity;
	}

	return NULL;
}

//================================================
// Sons du monstre 
//================================================

void CDiablo::SonFrappe(BOOL Touche)
{
	if (Touche)
	{
		switch (RANDOM_LONG(0, 2))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "zombie/claw_strike1.wav", 1, ATTN_STATIC);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "zombie/claw_strike2.wav", 1, ATTN_STATIC);
			break;
		case 2:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "zombie/claw_strike3.wav", 1, ATTN_STATIC);
			break;
		}
	}
	else if (!Touche)
	{
		switch (RANDOM_LONG(0, 1))
		{
		case 0:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "zombie/claw_miss1.wav", 1, ATTN_STATIC);
			break;
		case 1:
			EMIT_SOUND(ENT(pev), CHAN_WEAPON, "zombie/claw_miss2.wav", 1, ATTN_STATIC);
			break;
		}
	}
}






//================================================
// Handle Anim Event :
// évènements durant les anims
//================================================

void CDiablo::HandleAnimEvent(MonsterEvent_t* pEvent)
{

	switch (pEvent->event)
	{

	case DIABLO_AE_KICK_NORMAL:
	{
		CBaseEntity* pHurtNormal = KickNormal();
		if (pHurtNormal)
		{
			int pitch = 100 + RANDOM_LONG(-5, 5);

			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "panthereye/pa_attack1.wav", 1.0, ATTN_NORM, 0, pitch);
			UTIL_MakeVectors(pev->angles);
			pHurtNormal->pev->velocity = pHurtNormal->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 200;
			pHurtNormal->TakeDamage(pev, pev, gSkillData.panthereyeDmgClaw, DMG_SLASH);
			SonFrappe(TRUE);
		}
		else
		{
			SonFrappe(FALSE);
		}
		break;
	}

	case DIABLO_AE_KICK_LOW:
	{
		CBaseEntity* pHurtBas = KickLow();
		if (pHurtBas)
		{
			int pitch = 100 + RANDOM_LONG(-5, 5);

			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "panthereye/pa_attack1.wav", 1.0, ATTN_NORM, 0, pitch);
			UTIL_MakeVectors(pev->angles);
			pHurtBas->pev->velocity = pHurtBas->pev->velocity + gpGlobals->v_forward * 75 + gpGlobals->v_up * 75;
			pHurtBas->TakeDamage(pev, pev, gSkillData.panthereyeDmgClaw * 0.6, DMG_SLASH);
			SonFrappe(TRUE);
		}
		else
		{
			SonFrappe(FALSE);
		}
		break;
	}

	case DIABLO_AE_KICK_HIGH:
	{
		CBaseEntity* pHurtLoin = KickHigh();
		if (pHurtLoin)
		{
			int pitch = 100 + RANDOM_LONG(-5, 5);

			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "panthereye/pa_attack1.wav", 1.0, ATTN_NORM, 0, pitch);
			UTIL_MakeVectors(pev->angles);
			pHurtLoin->pev->velocity = pHurtLoin->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 200;
			pHurtLoin->TakeDamage(pev, pev, gSkillData.panthereyeDmgClaw, DMG_SLASH);
			SonFrappe(TRUE);
		}
		else
		{
			SonFrappe(FALSE);
		}
		break;
	}

	case 5://jump attack
		SetTouch(&CDiablo::LeapTouch);
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, "panthereye/pa_attack1.wav", 1, ATTN_IDLE, 0, 100);
		Jump();
		break;
	}

	CSquadMonster::HandleAnimEvent(pEvent);
}



//=================================================
// AI Schedules
//=================================================

//=== court vers l'ennemi lorsqu' il en est loin ===

Task_t	tlDiabloRangeAttack1[] =
{
	{ TASK_STOP_MOVING,			(float)0					},
	{ TASK_FACE_ENEMY,			(float)0					},
	{ TASK_PLAY_SEQUENCE,		(float)ACT_LEAP				}
};

Schedule_t	slDiabloRangeAttack1[] =
{
	{
		tlDiabloRangeAttack1,
		ARRAYSIZE(tlDiabloRangeAttack1),
		0,
		0,
		"CatLeap"
	},

};

Task_t tlPanthereyeJumpAtEnemyUnstuck_PreCheck[] =
{
	// If this fails, unstuck jump time
	{ TASK_STOP_MOVING,				(float)0									},
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_DIABLO_JUMP_AT_ENEMY_UNSTUCK	},
	{ TASK_FACE_ENEMY,				(float)0									},
	{ TASK_DIABLO_ATTEMPT_ROUTE,	(float)60									},
	// assuming success if the schedule didn't fail at ATTEMPT_ROUTE. Use that schedule then.
	// This also includes setting the fail schedule back to generic, since failing at any other
	// point might not necessarily mean being stuck
	{TASK_DIABLO_USE_ROUTE,			(float)60									}
};

Schedule_t slPanthereyeJumpAtEnemyUnstuck_PreCheck[] =
{
	{
		tlPanthereyeJumpAtEnemyUnstuck_PreCheck,
		ARRAYSIZE(tlPanthereyeJumpAtEnemyUnstuck_PreCheck),
		0,
		0,
		"Panther_JumpAtEnemy_unstuck_PC"
},
};

Task_t	tlPanthereyeJumpUnstuck[] =
{
	{ TASK_STOP_MOVING,						(float)0		},
	{ TASK_DIABLO_DETERMINE_UNSTUCK_IDEAL,  (float)0		},
	{ TASK_FACE_IDEAL,						(float)0		},   //set before going to this sched...
	{ TASK_PLAY_SEQUENCE,					(float)ACT_LEAP	}  //includes wait for anim finish

};

Schedule_t	slPanthereyeJumpAtEnemyUnstuck[] =
{
	{
		tlPanthereyeJumpUnstuck,
		ARRAYSIZE(tlPanthereyeJumpUnstuck),
		0,
		0,
		"Panther_JumpAtEnemy_unstuck"
	},
};

DEFINE_CUSTOM_SCHEDULES(CDiablo)
{
	slDiabloRangeAttack1,
	slPanthereyeJumpAtEnemyUnstuck_PreCheck,
	slPanthereyeJumpAtEnemyUnstuck,
};

IMPLEMENT_CUSTOM_SCHEDULES(CDiablo, CSquadMonster);


//==================================


Schedule_t* CDiablo::GetSchedule(void)
{
	switch (m_MonsterState)
	{
	case MONSTERSTATE_COMBAT:
	{

		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_DIABLO_LEAP);
		}

	}
	}

	return CSquadMonster::GetSchedule();
}


//===========================


Schedule_t* CDiablo::GetScheduleOfType(int Type)
{
	switch (Type)
	{
	case SCHED_DIABLO_LEAP:
		return &slDiabloRangeAttack1[0];
		break;

/*	case SCHED_FAIL:
	case SCHED_CHASE_ENEMY_FAILED:
		return &slPanthereyeJumpAtEnemyUnstuck_PreCheck[0];
		break;*/
	case SCHED_DIABLO_JUMP_AT_ENEMY_UNSTUCK:
		return slPanthereyeJumpAtEnemyUnstuck;
		break;
	}

	return CSquadMonster::GetScheduleOfType(Type);
}


//=========================================================
// start task
//=========================================================
void CDiablo::StartTask(Task_t* pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_DIABLO_ATTEMPT_ROUTE:
		// Really the first part of MOVE_TO_ENEMY_RANGE:  make the route, but complete if it passes
		pTask->iTask = TASK_MOVE_TO_TARGET_RANGE;
		m_hTargetEnt = m_hEnemy;
		CSquadMonster::StartTask(pTask);

		if (FRouteClear())
		{
			// No route?  Assume it failed
			TaskFail();
		}
		else
		{
			TaskComplete();
		}
		break;
	case TASK_DIABLO_USE_ROUTE:
	{
		// same as MOVE_TO_ENEMY_RANGE, but doesn't start by making a route (assuming that came
		// from the previous task succeeding)
		// Do nothing here, leave the rest to RunTask.

	}break;

	break;
	case TASK_DIABLO_DETERMINE_UNSTUCK_IDEAL: {
		int i;
		int alternator;
		TraceResult tr;
		Vector vecStart;
		Vector vecEnd;
		Vector vecAnglesTest(pev->angles.x, pev->angles.y, pev->angles.z);
		Vector currentForward;
		float angleShift = 0;

		// try different degrees, starting at 0 (straight forward), then alternating 45
		// degree increments left and right of that (45, -45, 90, -90, 135, -135, 180)

		alternator = -1;  // so that after the first run adds 45 degrees
		for (i = 0; i < 8; i++) {
			vecAnglesTest.y = fmod(pev->angles.y + angleShift * alternator, 360);

			vecStart = EyePosition();
			UTIL_MakeVectorsPrivate(vecAnglesTest, currentForward, NULL, NULL);
			vecEnd = vecStart + currentForward * 300;
			// test it.
			UTIL_TraceLine(vecStart, vecEnd, ignore_monsters, ENT(pev), &tr);

			const char* hitClassname = "NULL";
			float fracto = tr.flFraction;
			if (tr.pHit != NULL) {
				if (tr.pHit->v.classname != NULL) {
					hitClassname = STRING(tr.pHit->v.classname);
				}
			}

			if (tr.flFraction == 1.0) {
				// take it
				pev->ideal_yaw = vecAnglesTest.y;
				TaskComplete();
				return;
			}

			if (alternator == -1) {
				// time to add 45
				angleShift += 45;
				alternator = 1;
			}
			else {
				// invert only
				alternator = -1;
			}
		}//for loop through angle attempts

		// No good degree to take?   Huh.
		TaskFail();
	}break;
	}
	
	CSquadMonster::StartTask(pTask);
	
	
}

//=========================================================
// RunTask 
//=========================================================
void CDiablo::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_DIABLO_USE_ROUTE: {
		pTask->iTask = TASK_MOVE_TO_TARGET_RANGE;
		CSquadMonster::RunTask(pTask);
		TaskComplete();
	}break;
	}

	CSquadMonster::RunTask(pTask);

}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CDiablo::LeapTouch(CBaseEntity* pOther)
{
	if (!FClassnameIs(pOther->pev, "monster_panthereye"))
		pOther->TakeDamage(pev, pev, gSkillData.panthereyeDmgLeap, DMG_SLASH);

	//SetBits(pev->flags, FL_ONGROUND);
	pev->movetype = MOVETYPE_STEP;
	SetTouch(NULL);

}

void CDiablo::Jump()
{

	ClearBits(pev->flags, FL_ONGROUND);

	pev->movetype = MOVETYPE_TOSS;

	MakeIdealYaw(m_vecEnemyLKP);
	ChangeYaw(pev->yaw_speed);

	UTIL_SetOrigin(pev, pev->origin + Vector(0, 0, 1));// take him off ground so engine doesn't instantly reset onground 
	UTIL_MakeAimVectors(pev->angles);

	Vector vecJumpDir;
	if (m_hEnemy != NULL)
	{
		float gravity = g_psv_gravity->value;

		// How fast does the headcrab need to travel to reach that height given gravity?
		float height = (m_vecEnemyLKP.z + m_hEnemy->pev->view_ofs.z - pev->origin.z);

		float speedExtraMult = 1;
		//was a minimum of 15.  (was 11?)
		if (height < 18)
			height = 18;


		//MODDD - greater, was 40
		if (height > 55) {
			//CRUDE. Take how much height was above 40 and let it add to speedExtraMult.
			speedExtraMult = 1 + (height - 55) / 55;
			height = 55;
		}

		//was 2 * gravity * height.
		float speed = sqrt(1.7 * gravity * height * speedExtraMult);
		float time = speed / gravity;

		//must jump at least "240" far...
		vecJumpDir = max((m_vecEnemyLKP + m_hEnemy->pev->view_ofs - pev->origin).Length() - 80, 320) * gpGlobals->v_forward;

		vecJumpDir = vecJumpDir * (1.0 / time);

		// Speed to offset gravity at the desired height
		vecJumpDir.z = speed;

		// Don't jump too far/fast
		float distance = vecJumpDir.Length();

		// I'm not entirely sure what this is doing...
		// OH, I see, forcing a dist of 650 exactly, factoring out the old value.  It's how vectors work.
		// UPPED, was 950.
		if (distance > 1100)
		{
			vecJumpDir = vecJumpDir * (1100.0 / distance);
		}

	}
	else
	{
		// jump hop, don't care where
		vecJumpDir = Vector(gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z) * 350;
	}

	pev->velocity = vecJumpDir;
	NextLeapTime = gpGlobals->time + RANDOM_FLOAT(1.5f, 5.0f);
}

void CDiablo::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (ptr->iHitgroup != HITGROUP_HEAD)
	{
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage *= 0.5;
		}
	}
	if (ptr->iHitgroup == HITGROUP_HEAD)
	{
		if (bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB))
		{
			flDamage *= 0.75;// don't take full damage to the head because hehehe hahaha
		}
	}

	CSquadMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}