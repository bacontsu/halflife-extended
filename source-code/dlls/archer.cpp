//=========================================
//
// HL:E Archer by tear
//
//=========================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include    "flyingmonster.h"
#include	"nodes.h"
#include	"soundent.h"
#include	"animation.h"
#include	"effects.h"
#include	"weapons.h"
#include	"decals.h"
#include	"game.h"

int			   iArcherSpitSprite;





//=========================================================
// Bullsquid's spit projectile
//=========================================================
class CArcherSpit : public CBaseEntity
{
public:
	void Spawn() override;

	static void Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity);
	void Touch(CBaseEntity* pOther) override;
	void EXPORT Animate();

	int		Save(CSave& save) override;
	int		Restore(CRestore& restore) override;
	static	TYPEDESCRIPTION m_SaveData[];



	int  m_maxFrame;
};

LINK_ENTITY_TO_CLASS(archerspit, CArcherSpit);

TYPEDESCRIPTION	CArcherSpit::m_SaveData[] =
{
	DEFINE_FIELD(CArcherSpit, m_maxFrame, FIELD_INTEGER),
};

IMPLEMENT_SAVERESTORE(CArcherSpit, CBaseEntity);

void CArcherSpit::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING("archerspit");

	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;

	SET_MODEL(ENT(pev), "sprites/cnt1.spr");
	pev->frame = 0;
	pev->scale = 0.25;

	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));

	m_maxFrame = (float)MODEL_FRAMES(pev->modelindex) - 1;
}

void CArcherSpit::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->frame++)
	{
		if (pev->frame > m_maxFrame)
		{
			pev->frame = 0;
		}
	}
}

void CArcherSpit::Shoot(entvars_t* pevOwner, Vector vecStart, Vector vecVelocity)
{
	CArcherSpit* pSpit = GetClassPtr((CArcherSpit*)NULL);
	pSpit->Spawn();

	UTIL_SetOrigin(pSpit->pev, vecStart);
	pSpit->pev->velocity = vecVelocity;
	pSpit->pev->owner = ENT(pevOwner);

	pSpit->SetThink(&CArcherSpit::Animate);
	pSpit->pev->nextthink = gpGlobals->time + 0.1;
}

void CArcherSpit::Touch(CBaseEntity* pOther)
{
	TraceResult tr;
	int		iPitch;

	// splat sound
	iPitch = RANDOM_FLOAT(90, 110);

	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, iPitch);

	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, iPitch);
		break;
	}

	if (!pOther->pev->takedamage)
	{

		// make a splat on the wall
		UTIL_TraceLine(pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT(pev), &tr);
		UTIL_DecalTrace(&tr, DECAL_SMALLSCORCH1 + RANDOM_LONG(0, 1));

		/*// make some flecks
		MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, tr.vecEndPos);
		WRITE_BYTE(TE_SPRITE_SPRAY);
		WRITE_COORD(tr.vecEndPos.x);	// pos
		WRITE_COORD(tr.vecEndPos.y);
		WRITE_COORD(tr.vecEndPos.z);
		WRITE_COORD(tr.vecPlaneNormal.x);	// dir
		WRITE_COORD(tr.vecPlaneNormal.y);
		WRITE_COORD(tr.vecPlaneNormal.z);
		WRITE_SHORT(iArcherSpitSprite);	// model
		WRITE_BYTE(5);			// count
		WRITE_BYTE(30);			// speed
		WRITE_BYTE(80);			// noise ( client will divide by 100 )
		MESSAGE_END();*/
	}
	else
	{
		pOther->TakeDamage(pev, pev, gSkillData.archerDmgPlasma, DMG_GENERIC);
	}

	SetThink(&CArcherSpit::SUB_Remove);
	pev->nextthink = gpGlobals->time;
}




//=====================================================
// Monster's anim events go here
//=====================================================
#define ARCHER_AE_BITE			( 1 )
#define ARCHER_AE_SHOOT			( 2 )

class CArcher : public CFlyingMonster
{
public:
	void  Spawn() override;
	void  Precache() override;
	void  SetYawSpeed() override;
	int   Classify() override;
	void  HandleAnimEvent(MonsterEvent_t* pEvent) override;
	CUSTOM_SCHEDULES;

	int	Save(CSave& save) override;
	int Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType(int Type) override;

	void Killed(entvars_t* pevAttacker, int iGib) override;
	void BecomeDead() override;

	void EXPORT CombatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
	void EXPORT BiteTouch(CBaseEntity* pOther);

	void  StartTask(Task_t* pTask) override;
	void  RunTask(Task_t* pTask) override;

	BOOL  CheckMeleeAttack1(float flDot, float flDist) override;
	BOOL  CheckRangeAttack1(float flDot, float flDist) override;

	float ChangeYaw(int speed) override;
	Activity GetStoppedActivity() override;

	void  Move(float flInterval) override;
	void  MoveExecute(CBaseEntity* pTargetEnt, const Vector& vecDir, float flInterval) override;
	void  MonsterThink() override;
	void  Stop() override;
	void  Swim();
	Vector DoProbe(const Vector& Probe);

	float VectorToPitch(const Vector& vec);
	float FlPitchDiff();
	float ChangePitch(int speed);

	Vector m_SaveVelocity;
	float m_idealDist;

	float m_flBlink;

	float m_flEnemyTouched;
	BOOL  m_bOnAttack;

	float m_flMaxSpeed;
	float m_flMinSpeed;
	float m_flMaxDist;

	CBeam* m_pBeam;

	float m_flNextAlert;

	float m_flLastPitchTime;

	float m_flNextSpitTime;// last time the bullsquid used the spit attack.

	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pAttackSounds[];
	static const char* pBiteSounds[];
	static const char* pDieSounds[];
	static const char* pPainSounds[];

	void IdleSound() override;
	void AlertSound() override;
	void AttackSound();
	void BiteSound();
	void DeathSound() override;
	void PainSound() override;
};
LINK_ENTITY_TO_CLASS(monster_archer, CArcher);

TYPEDESCRIPTION	CArcher::m_SaveData[] =
{
	//DEFINE_FIELD(CArcher, m_flLastHurtTime, FIELD_TIME),
	DEFINE_FIELD(CArcher, m_flNextSpitTime, FIELD_TIME),
	//DEFINE_FIELD(CArcher, m_flNextEatTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE(CArcher, CFlyingMonster);

const char* CArcher::pIdleSounds[] =
{
	"archer/arch_idle1.wav",
	"archer/arch_idle2.wav",
	"archer/arch_idle3.wav",
};

const char* CArcher::pAlertSounds[] =
{
	"archer/arch_alert2.wav",
	"archer/arch_alert3.wav",
};

const char* CArcher::pAttackSounds[] =
{
	"archer/arch_plasma.wav",
};

const char* CArcher::pBiteSounds[] =
{
	"archer/arch_bite1.wav",
	"archer/arch_bite2.wav",
};

const char* CArcher::pPainSounds[] =
{
	"archer/arch_pain2.wav",
	"archer/arch_pain3.wav",
	"archer/arch_pain1.wav",
	"archer/arch_pain4.wav",
};

const char* CArcher::pDieSounds[] =
{
	"archer/arch_die2.wav",
	"archer/arch_die1.wav",
	"archer/arch_die3.wav",
};

#define EMIT_ARCH_SOUND( chan, array ) \
	EMIT_SOUND_DYN ( ENT(pev), chan , array [ RANDOM_LONG(0,ARRAYSIZE( array )-1) ], 1.0, 0.6, 0, RANDOM_LONG(95,105) ); 

void CArcher::IdleSound()
{
	EMIT_ARCH_SOUND(CHAN_VOICE, pIdleSounds);
}

void CArcher::AlertSound()
{
	EMIT_ARCH_SOUND(CHAN_VOICE, pAlertSounds);
}

void CArcher::AttackSound()
{
	EMIT_ARCH_SOUND(CHAN_VOICE, pAttackSounds);
}

void CArcher::BiteSound()
{
	EMIT_ARCH_SOUND(CHAN_WEAPON, pBiteSounds);
}

void CArcher::DeathSound()
{
	EMIT_ARCH_SOUND(CHAN_VOICE, pDieSounds);
}

void CArcher::PainSound()
{
	EMIT_ARCH_SOUND(CHAN_VOICE, pPainSounds);
}

//=========================================================
// monster-specific tasks and states
//=========================================================
enum
{
	TASK_ARCHER_CIRCLE_ENEMY = LAST_COMMON_TASK + 1,
	TASK_ARCHER_SWIM,
	TASK_ARCHER_FLOAT,
};

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

static Task_t	tlSwimAround[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_ARCHER_SWIM,		0.0 },
};

static Schedule_t	slSwimAround[] =
{
	{
		tlSwimAround,
		ARRAYSIZE(tlSwimAround),
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_PLAYER |
		bits_SOUND_COMBAT,
		"SwimAround"
	},
};

static Task_t	tlSwimAgitated[] =
{
	{ TASK_STOP_MOVING,				(float)0 },
	{ TASK_SET_ACTIVITY,			(float)ACT_RUN },
	{ TASK_WAIT,					(float)2.0 },
};

static Schedule_t	slSwimAgitated[] =
{
	{
		tlSwimAgitated,
		ARRAYSIZE(tlSwimAgitated),
		0,
		0,
		"SwimAgitated"
	},
};


static Task_t	tlCircleEnemy[] =
{
	{ TASK_SET_ACTIVITY,			(float)ACT_WALK },
	{ TASK_ARCHER_CIRCLE_ENEMY, 0.0 },
};

static Schedule_t	slCircleEnemy[] =
{
	{
		tlCircleEnemy,
		ARRAYSIZE(tlCircleEnemy),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK1,
		0,
		"CircleEnemy"
	},
};


Task_t tlATwitchDie[] =
{
	{ TASK_STOP_MOVING,			0		 },
	{ TASK_SOUND_DIE,			(float)0 },
	{ TASK_DIE,					(float)0 },
	{ TASK_ARCHER_FLOAT,	(float)0 },
};

Schedule_t slATwitchDie[] =
{
	{
		tlATwitchDie,
		ARRAYSIZE(tlATwitchDie),
		0,
		0,
		"Die"
	},
};


DEFINE_CUSTOM_SCHEDULES(CArcher)
{
	slSwimAround,
		slSwimAgitated,
		slCircleEnemy,
		slATwitchDie,
};
IMPLEMENT_CUSTOM_SCHEDULES(CArcher, CFlyingMonster);

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CArcher::Classify()
{
	return	CLASS_ALIEN_MONSTER;
}

//=========================================================
// CheckRangeAttack1
//=========================================================
BOOL CArcher::CheckRangeAttack1(float flDot, float flDist)
{
	if (IsMoving() && flDist >= 512)
	{
		// squid will far too far behind if he stops running to spit at this distance from the enemy.
		return FALSE;
	}

	if (flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpitTime)
	{
		if (m_hEnemy != NULL)
		{
			if (fabs(pev->origin.z - m_hEnemy->pev->origin.z) > 256)
			{
				// don't try to spit at someone up really high or down really low.
				return FALSE;
			}
		}

		if (IsMoving())
		{
			// don't spit again for a long time, resume chasing enemy.
			m_flNextSpitTime = gpGlobals->time + 5;
		}
		else
		{
			// not moving, so spit again pretty soon.
			m_flNextSpitTime = gpGlobals->time + 0.5;
		}

		return TRUE;
	}

	return FALSE;
}

//=========================================================
// CheckMeleeAttack1
//=========================================================
BOOL CArcher::CheckMeleeAttack1(float flDot, float flDist)
{
	if (flDot >= 0.7 && m_flEnemyTouched > gpGlobals->time - 0.2)
	{
		return TRUE;
	}
	return FALSE;
}

void CArcher::BiteTouch(CBaseEntity* pOther)
{
	// bite if we hit who we want to eat
	if (pOther == m_hEnemy)
	{
		m_flEnemyTouched = gpGlobals->time;
		m_bOnAttack = TRUE;
	}
}

void CArcher::CombatUse(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (!ShouldToggle(useType, m_bOnAttack))
		return;

	if (m_bOnAttack)
	{
		m_bOnAttack = 0;
	}
	else
	{
		m_bOnAttack = 1;
	}
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CArcher::SetYawSpeed()
{
	pev->yaw_speed = 100;
}



//=========================================================
// Killed - overrides CFlyingMonster.
//
void CArcher::Killed(entvars_t* pevAttacker, int iGib)
{
	CBaseMonster::Killed(pevAttacker, iGib);
	pev->velocity = Vector(0, 0, 0);
}

void CArcher::BecomeDead()
{
	pev->takedamage = DAMAGE_YES;// don't let autoaim aim at corpses.

	// give the corpse half of the monster's original maximum health. 
	pev->health = pev->max_health / 2;
	pev->max_health = 5; // max_health now becomes a counter for how many blood decals the corpse can place.
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CArcher::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	int bDidAttack = FALSE;
	switch (pEvent->event)
	{
	
	case ARCHER_AE_BITE:
	{
		if (m_hEnemy != NULL && FVisible(m_hEnemy))
		{
			// SOUND HERE!
			CBaseEntity* pHurt = CheckTraceHullAttack(70, gSkillData.archerDmgBite, DMG_SLASH);
			if (pHurt)
			{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
				{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
				}
				// Play a random attack hit sound
				EMIT_ARCH_SOUND(CHAN_VOICE, pBiteSounds);
			}
			bDidAttack = TRUE;
	}
	break;

	case ARCHER_AE_SHOOT:
	{
		if (m_hEnemy)
		{
			Vector	vecSpitOffset;
			Vector	vecSpitDir;

			UTIL_MakeVectors(pev->angles);

			// !!!HACKHACK - the spot at which the spit originates (in front of the mouth) was measured in 3ds and hardcoded here.
			// we should be able to read the position of bones at runtime for this info.
			vecSpitOffset = (gpGlobals->v_right * 8 + gpGlobals->v_forward * 37 + gpGlobals->v_up * 23);
			vecSpitOffset = (pev->origin + vecSpitOffset);
			vecSpitDir = ((m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs) - vecSpitOffset).Normalize();

			vecSpitDir.x += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.y += RANDOM_FLOAT(-0.05, 0.05);
			vecSpitDir.z += RANDOM_FLOAT(-0.05, 0);


			// do stuff for this event.
			AttackSound();

			/*// spew the spittle temporary ents.
			MESSAGE_BEGIN(MSG_PVS, SVC_TEMPENTITY, vecSpitOffset);
			WRITE_BYTE(TE_SPRITE_SPRAY);
			WRITE_COORD(vecSpitOffset.x);	// pos
			WRITE_COORD(vecSpitOffset.y);
			WRITE_COORD(vecSpitOffset.z);
			WRITE_COORD(vecSpitDir.x);	// dir
			WRITE_COORD(vecSpitDir.y);
			WRITE_COORD(vecSpitDir.z);
			WRITE_SHORT(iArcherSpitSprite);	// model
			WRITE_BYTE(15);			// count
			WRITE_BYTE(210);			// speed
			WRITE_BYTE(25);			// noise ( client will divide by 100 )
			MESSAGE_END();*/

			CArcherSpit::Shoot(pev, vecSpitOffset, vecSpitDir * 900);
		}
		bDidAttack = TRUE;
	}
	break;
		break;
	}
	default:
		CFlyingMonster::HandleAnimEvent(pEvent);
		break;
	}

	if (bDidAttack)
	{
		Vector vecSrc = pev->origin + gpGlobals->v_forward * 32;
		UTIL_Bubbles(vecSrc - Vector(8, 8, 8), vecSrc + Vector(8, 8, 8), 16);
	}
}

//=========================================================
// Spawn
//=========================================================
void CArcher::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/archer.mdl");
	UTIL_SetSize(pev, Vector(-32, -32, -32), Vector(32, 32, 32));

	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.archerHealth;
	pev->view_ofs = Vector(0, 0, 16);
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	SetBits(pev->flags, FL_SWIM);
	SetFlyingSpeed(190);
	SetFlyingMomentum(2.5);	// Set momentum constant

	m_afCapability = bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1 | bits_CAP_SWIM;

	MonsterInit();

	SetTouch(&CArcher::BiteTouch);
	SetUse(&CArcher::CombatUse);

	m_idealDist = 384;
	m_flMinSpeed = 80;
	m_flMaxSpeed = 300;
	m_flMaxDist = 384;

	Vector Forward;
	UTIL_MakeVectorsPrivate(pev->angles, Forward, 0, 0);
	pev->velocity = m_flightSpeed * Forward.Normalize();
	m_SaveVelocity = pev->velocity;
}

//==========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CArcher::Precache()
{
	PRECACHE_MODEL("models/archer.mdl");

	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);
	PRECACHE_SOUND_ARRAY(pDieSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);

	iArcherSpitSprite = PRECACHE_MODEL("sprites/bhit.spr");// client side spittle.
	PRECACHE_MODEL("sprites/cnt1.spr");// spit projectile.
}


//=========================================================
// GetSchedule
//=========================================================
Schedule_t* CArcher::GetSchedule()
{
	// ALERT( at_console, "GetSchedule( )\n" );
	switch (m_MonsterState)
	{
	case MONSTERSTATE_IDLE:
		m_flightSpeed = 80;
		return GetScheduleOfType(SCHED_IDLE_WALK);

	case MONSTERSTATE_ALERT:
		m_flightSpeed = 150;
		return GetScheduleOfType(SCHED_IDLE_WALK);

	case MONSTERSTATE_COMBAT:
		m_flMaxSpeed = 400;
		// eat them
		if (HasConditions(bits_COND_CAN_MELEE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_MELEE_ATTACK1);
		}
		// chase them down and eat them
		if (HasConditions(bits_COND_CAN_RANGE_ATTACK1))
		{
			return GetScheduleOfType(SCHED_RANGE_ATTACK1);
		}
		if (HasConditions(bits_COND_HEAVY_DAMAGE))
		{
			m_bOnAttack = TRUE;
		}
		if (pev->health < pev->max_health - 20)
		{
			m_bOnAttack = TRUE;
		}

		return GetScheduleOfType(SCHED_STANDOFF);
	}

	return CFlyingMonster::GetSchedule();
}


//= ========================================================
//=========================================================
Schedule_t * CArcher::GetScheduleOfType(int Type)
{
	// ALERT( at_console, "GetScheduleOfType( %d ) %d\n", Type, m_bOnAttack );
	switch (Type)
	{
	case SCHED_IDLE_WALK:
		return slSwimAround;
	case SCHED_STANDOFF:
		return slCircleEnemy;
	case SCHED_FAIL:
		return slSwimAgitated;
	case SCHED_DIE:
		return slATwitchDie;
	case SCHED_CHASE_ENEMY:
		AttackSound();
	}

	return CBaseMonster::GetScheduleOfType(Type);
}



//=========================================================
// Start task - selects the correct activity and performs
// any necessary calculations to start the next task on the
// schedule.
//=========================================================
void CArcher::StartTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ARCHER_CIRCLE_ENEMY:
		break;
	case TASK_ARCHER_SWIM:
		break;
	case TASK_SMALL_FLINCH:
		if (m_idealDist > 128)
		{
			m_flMaxDist = 512;
			m_idealDist = 512;
		}
		else
		{
			m_bOnAttack = TRUE;
		}
		CFlyingMonster::StartTask(pTask);
		break;

	case TASK_ARCHER_FLOAT:
		SetSequenceByName("bellyup");
		break;

	default:
		CFlyingMonster::StartTask(pTask);
		break;
	}
}

void CArcher::RunTask(Task_t* pTask)
{
	switch (pTask->iTask)
	{
	case TASK_ARCHER_CIRCLE_ENEMY:
		if (m_hEnemy == NULL)
		{
			TaskComplete();
		}
		else if (FVisible(m_hEnemy))
		{
			Vector vecFrom = m_hEnemy->EyePosition();

			Vector vecDelta = (pev->origin - vecFrom).Normalize();
			Vector vecSwim = CrossProduct(vecDelta, Vector(0, 0, 1)).Normalize();

			if (DotProduct(vecSwim, m_SaveVelocity) < 0)
				vecSwim = vecSwim * -1.0;

			Vector vecPos = vecFrom + vecDelta * m_idealDist + vecSwim * 32;

			// ALERT( at_console, "vecPos %.0f %.0f %.0f\n", vecPos.x, vecPos.y, vecPos.z );

			TraceResult tr;

			UTIL_TraceHull(vecFrom, vecPos, ignore_monsters, large_hull, m_hEnemy->edict(), &tr);

			if (tr.flFraction > 0.5)
				vecPos = tr.vecEndPos;

			m_SaveVelocity = m_SaveVelocity * 0.8 + 0.2 * (vecPos - pev->origin).Normalize() * m_flightSpeed;

			// ALERT( at_console, "m_SaveVelocity %.2f %.2f %.2f\n", m_SaveVelocity.x, m_SaveVelocity.y, m_SaveVelocity.z );

			if (HasConditions(bits_COND_ENEMY_FACING_ME) && m_hEnemy->FVisible(this))
			{
				m_flNextAlert -= 0.1;

				if (m_idealDist < m_flMaxDist)
				{
					m_idealDist += 4;
				}
				if (m_flightSpeed > m_flMinSpeed)
				{
					m_flightSpeed -= 2;
				}
				else if (m_flightSpeed < m_flMinSpeed)
				{
					m_flightSpeed += 2;
				}
				if (m_flMinSpeed < m_flMaxSpeed)
				{
					m_flMinSpeed += 0.5;
				}
			}
			else
			{
				m_flNextAlert += 0.1;

				if (m_idealDist > 128)
				{
					m_idealDist -= 4;
				}
				if (m_flightSpeed < m_flMaxSpeed)
				{
					m_flightSpeed += 4;
				}
			}
			// ALERT( at_console, "%.0f\n", m_idealDist );
		}
		else
		{
			m_flNextAlert = gpGlobals->time + 0.2;
		}

		if (m_flNextAlert < gpGlobals->time)
		{
			// ALERT( at_console, "AlertSound()\n");
			AlertSound();
			m_flNextAlert = gpGlobals->time + RANDOM_FLOAT(3, 5);
		}

		break;
	case TASK_ARCHER_SWIM:
		if (m_fSequenceFinished)
		{
			TaskComplete();
		}
		break;
	case TASK_DIE:
		if (m_fSequenceFinished)
		{
			pev->deadflag = DEAD_DEAD;

			TaskComplete();
		}
		break;

	case TASK_ARCHER_FLOAT:
		pev->angles.x = UTIL_ApproachAngle(0, pev->angles.x, 20);
		pev->velocity = pev->velocity * 0.8;
		if (pev->waterlevel > 1 && pev->velocity.z < 64)
		{
			pev->velocity.z += 8;
		}
		else
		{
			pev->velocity.z -= 8;
		}
		// ALERT( at_console, "%f\n", pev->velocity.z );
		break;

	default:
		CFlyingMonster::RunTask(pTask);
		break;
	}
}

float CArcher::VectorToPitch(const Vector& vec)
{
	float pitch;
	if (vec.z == 0 && vec.x == 0)
		pitch = 0;
	else
	{
		pitch = (int)(atan2(vec.z, sqrt(vec.x * vec.x + vec.y * vec.y)) * 180 / M_PI);
		if (pitch < 0)
			pitch += 360;
	}
	return pitch;
}

//=========================================================
void CArcher::Move(float flInterval)
{
	CFlyingMonster::Move(flInterval);
}

float CArcher::FlPitchDiff()
{
	float	flPitchDiff;
	float	flCurrentPitch;

	flCurrentPitch = UTIL_AngleMod(pev->angles.z);

	if (flCurrentPitch == pev->idealpitch)
	{
		return 0;
	}

	flPitchDiff = pev->idealpitch - flCurrentPitch;

	if (pev->idealpitch > flCurrentPitch)
	{
		if (flPitchDiff >= 180)
			flPitchDiff = flPitchDiff - 360;
	}
	else
	{
		if (flPitchDiff <= -180)
			flPitchDiff = flPitchDiff + 360;
	}
	return flPitchDiff;
}

float CArcher::ChangePitch(int speed)
{
	if (pev->movetype == MOVETYPE_FLY)
	{
		float diff = FlPitchDiff();
		float target = 0;
		if (m_IdealActivity != GetStoppedActivity())
		{
			if (diff < -20)
				target = 45;
			else if (diff > 20)
				target = -45;
		}

		if (m_flLastPitchTime == 0)
		{
			m_flLastPitchTime = gpGlobals->time - gpGlobals->frametime;
		}

		float delta = gpGlobals->time - m_flLastPitchTime;

		m_flLastPitchTime = gpGlobals->time;

		if (delta > 0.25)
		{
			delta = 0.25;
		}

		pev->angles.x = UTIL_Approach(target, pev->angles.x, 220.0 * delta);
	}
	return 0;
}

float CArcher::ChangeYaw(int speed)
{
	if (pev->movetype == MOVETYPE_FLY)
	{
		float diff = FlYawDiff();
		float target = 0;

		if (m_IdealActivity != GetStoppedActivity())
		{
			if (diff < -20)
				target = 20;
			else if (diff > 20)
				target = -20;
		}

		if (m_flLastZYawTime == 0)
		{
			m_flLastZYawTime = gpGlobals->time - gpGlobals->frametime;
		}

		float delta = gpGlobals->time - m_flLastZYawTime;

		m_flLastZYawTime = gpGlobals->time;

		if (delta > 0.25)
		{
			delta = 0.25;
		}

		pev->angles.z = UTIL_Approach(target, pev->angles.z, 220.0 * delta);
	}
	return CFlyingMonster::ChangeYaw(speed);
}


Activity CArcher::GetStoppedActivity()
{
	if (pev->movetype != MOVETYPE_FLY)		// UNDONE: Ground idle here, IDLE may be something else
		return ACT_IDLE;
	return ACT_WALK;
}

void CArcher::MoveExecute(CBaseEntity* pTargetEnt, const Vector& vecDir, float flInterval)
{
	m_SaveVelocity = vecDir * m_flightSpeed;
}


void CArcher::MonsterThink()
{
	CFlyingMonster::MonsterThink();

	if (pev->deadflag == DEAD_NO)
	{
		if (m_MonsterState != MONSTERSTATE_SCRIPT)
		{
			Swim();
		}
	}
}

void CArcher :: Stop() 
{
	if (!m_bOnAttack)
		m_flightSpeed = 80.0;
}

void CArcher::Swim( )
{
	int retValue = 0;

	Vector start = pev->origin;

	Vector Angles;
	Vector Forward, Right, Up;

	if (FBitSet( pev->flags, FL_ONGROUND))
	{
		pev->angles.x = 0;
		pev->angles.y += RANDOM_FLOAT( -45, 45 );
		ClearBits( pev->flags, FL_ONGROUND );

		Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
		UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

		pev->velocity = Forward * 200 + Up * 200;

		return;
	}

	if (m_bOnAttack && m_flightSpeed < m_flMaxSpeed)
	{
		m_flightSpeed += 40;
	}
	if (m_flightSpeed < 180)
	{
		if (m_IdealActivity == ACT_RUN)
			SetActivity( ACT_WALK );
		if (m_IdealActivity == ACT_WALK)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "walk %.2f\n", pev->framerate );
	}
	else
	{
		if (m_IdealActivity == ACT_WALK)
			SetActivity( ACT_RUN );
		if (m_IdealActivity == ACT_RUN)
			pev->framerate = m_flightSpeed / 150.0;
		// ALERT( at_console, "run  %.2f\n", pev->framerate );
	}

/*
	if (!m_pBeam)
	{
		m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
		m_pBeam->PointEntInit( pev->origin + m_SaveVelocity, entindex( ) );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->SetColor( 255, 180, 96 );
		m_pBeam->SetBrightness( 192 );
	}
*/
#define PROBE_LENGTH 150
	Angles = UTIL_VecToAngles( m_SaveVelocity );
	Angles.x = -Angles.x;
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);

	Vector f, u, l, r, d;
	f = DoProbe(start + PROBE_LENGTH   * Forward);
	r = DoProbe(start + PROBE_LENGTH/3 * Forward+Right);
	l = DoProbe(start + PROBE_LENGTH/3 * Forward-Right);
	u = DoProbe(start + PROBE_LENGTH/3 * Forward+Up);
	d = DoProbe(start + PROBE_LENGTH/3 * Forward-Up);

	Vector SteeringVector = f+r+l+u+d;
	m_SaveVelocity = (m_SaveVelocity + SteeringVector/2).Normalize();

	Angles = Vector( -pev->angles.x, pev->angles.y, pev->angles.z );
	UTIL_MakeVectorsPrivate(Angles, Forward, Right, Up);
	// ALERT( at_console, "%f : %f\n", Angles.x, Forward.z );

	float flDot = DotProduct( Forward, m_SaveVelocity );
	if (flDot > 0.5)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed;
	else if (flDot > 0)
		pev->velocity = m_SaveVelocity = m_SaveVelocity * m_flightSpeed * (flDot + 0.5);
	else
		pev->velocity = m_SaveVelocity = m_SaveVelocity * 80;

	// ALERT( at_console, "%.0f %.0f\n", m_flightSpeed, pev->velocity.Length() );


	// ALERT( at_console, "Steer %f %f %f\n", SteeringVector.x, SteeringVector.y, SteeringVector.z );

/*
	m_pBeam->SetStartPos( pev->origin + pev->velocity );
	m_pBeam->RelinkBeam( );
*/

	// ALERT( at_console, "speed %f\n", m_flightSpeed );
	
	Angles = UTIL_VecToAngles( m_SaveVelocity );

	// Smooth Pitch
	//
	if (Angles.x > 180)
		Angles.x = Angles.x - 360;
	pev->angles.x = UTIL_Approach(Angles.x, pev->angles.x, 50 * 0.1 );
	if (pev->angles.x < -80) pev->angles.x = -80;
	if (pev->angles.x >  80) pev->angles.x =  80;

	// Smooth Yaw and generate Roll
	//
	float turn = 360;
	// ALERT( at_console, "Y %.0f %.0f\n", Angles.y, pev->angles.y );

	if (fabs(Angles.y - pev->angles.y) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y;
	}
	if (fabs(Angles.y - pev->angles.y + 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y + 360;
	}
	if (fabs(Angles.y - pev->angles.y - 360) < fabs(turn))
	{
		turn = Angles.y - pev->angles.y - 360;
	}

	float speed = m_flightSpeed * 0.1;

	// ALERT( at_console, "speed %.0f %f\n", turn, speed );
	if (fabs(turn) > speed)
	{
		if (turn < 0.0)
		{
			turn = -speed;
		}
		else
		{
			turn = speed;
		}
	}
	pev->angles.y += turn;
	pev->angles.z -= turn;
	pev->angles.y = fmod((pev->angles.y + 360.0), 360.0);

	static float yaw_adj;

	yaw_adj = yaw_adj * 0.8 + turn;

	// ALERT( at_console, "yaw %f : %f\n", turn, yaw_adj );

	SetBoneController( 0, -yaw_adj / 4.0 );

	// Roll Smoothing
	//
	turn = 360;
	if (fabs(Angles.z - pev->angles.z) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z;
	}
	if (fabs(Angles.z - pev->angles.z + 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z + 360;
	}
	if (fabs(Angles.z - pev->angles.z - 360) < fabs(turn))
	{
		turn = Angles.z - pev->angles.z - 360;
	}
	speed = m_flightSpeed/2 * 0.1;
	if (fabs(turn) < speed)
	{
		pev->angles.z += turn;
	}
	else
	{
		if (turn < 0.0)
		{
			pev->angles.z -= speed;
		}
		else
		{
			pev->angles.z += speed;
		}
	}
	if (pev->angles.z < -20) pev->angles.z = -20;
	if (pev->angles.z >  20) pev->angles.z =  20;

	UTIL_MakeVectorsPrivate( Vector( -Angles.x, Angles.y, Angles.z ), Forward, Right, Up);

	// UTIL_MoveToOrigin ( ENT(pev), pev->origin + Forward * speed, speed, MOVE_STRAFE );
}


Vector CArcher::DoProbe(const Vector &Probe)
{
	Vector WallNormal = Vector(0,0,-1); // WATER normal is Straight Down for fish.
	float frac;
	BOOL bBumpedSomething = ProbeZ(pev->origin, Probe, &frac);

	TraceResult tr;
	TRACE_MONSTER_HULL(edict(), pev->origin, Probe, dont_ignore_monsters, edict(), &tr);
	if ( tr.fAllSolid || tr.flFraction < 0.99 )
	{
		if (tr.flFraction < 0.0) tr.flFraction = 0.0;
		if (tr.flFraction > 1.0) tr.flFraction = 1.0;
		if (tr.flFraction < frac)
		{
			frac = tr.flFraction;
			bBumpedSomething = TRUE;
			WallNormal = tr.vecPlaneNormal;
		}
	}

	if (bBumpedSomething && (m_hEnemy == NULL || tr.pHit != m_hEnemy->edict()))
	{
		Vector ProbeDir = Probe - pev->origin;

		Vector NormalToProbeAndWallNormal = CrossProduct(ProbeDir, WallNormal);
		Vector SteeringVector = CrossProduct( NormalToProbeAndWallNormal, ProbeDir);

		float SteeringForce = m_flightSpeed * (1-frac) * (DotProduct(WallNormal.Normalize(), m_SaveVelocity.Normalize()));
		if (SteeringForce < 0.0)
		{
			SteeringForce = -SteeringForce;
		}
		SteeringVector = SteeringForce * SteeringVector.Normalize();
		
		return SteeringVector;
	}
	return Vector(0, 0, 0);
}