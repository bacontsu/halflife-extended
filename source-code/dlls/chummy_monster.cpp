#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

//========================================================
// Chumtoad Monster
//========================================================
//chumtoad monster anims enum <-unused
/*enum chummy_e {
	CHUB_IDLE1 = 0,
	CHUB_IDLE2,
	CHUB_IDLECROAK,
	CHUB_HOP,
	CHUB_JUMP,
	CHUB_DIE,
};*/

#define		CHUB_IDLE				0
#define		CHUB_BORED				1
#define		CHUB_SCARED_BY_ENT		2
//#define		CHUB_SCARED_BY_LIGHT	3 <-unused
#define		CHUB_SMELL_FOOD			4
#define		CHUB_EAT				5

class CChubMonster : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	void IdleSound() override;
	void EXPORT MonsterThink() override;
	void PickNewDest(int iCondition);
	float	m_flNextSmellTime;
	int		Classify() override;
	int		ISoundMask() override;
	virtual int GetVoicePitch() { return 100; }
	virtual float GetSoundVolue() { return 1.0; }
	void Move(float flInterval) override;
	Vector Center() override;
	Vector BodyTarget(const Vector& posSrc) override;

	void Killed(entvars_t* pevAttacker, int iGib);

	int	ObjectCaps() override { return CBaseMonster::ObjectCaps() | FCAP_IMPULSE_USE; }

	void GibMonster() override;
	void	Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value) override;

	void PrescheduleThink();

//	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;


	static const char* pIdleSounds[];

	int		m_iMode;
	// -----------------------------
};


const char* CChubMonster::pIdleSounds[] =
{
	"chumtoad/cht_croak_short.wav",
	"chumtoad/cht_croak_medium.wav",
	"chumtoad/cht_croak_long.wav",
	"chumtoad/chub_draw.wav"
};


LINK_ENTITY_TO_CLASS(monster_chumtoad, CChubMonster);


//=========================================================
// Center - returns the real center of the toad.  The 
// bounding box is much larger than the actual creature so 
// this is needed for targeting
//=========================================================
Vector CChubMonster::Center()
{
	return Vector(pev->origin.x, pev->origin.y, pev->origin.z + 10);
}


Vector CChubMonster::BodyTarget(const Vector& posSrc)
{
	return Center();
}

//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CChubMonster::ISoundMask()
{
	return	bits_SOUND_CARCASS | bits_SOUND_MEAT;
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CChubMonster::Classify()
{
	return m_iClass ? m_iClass : CLASS_CHUMTOAD;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CChubMonster::SetYawSpeed()
{
	int ys;

	ys = 70;

	pev->yaw_speed = ys;
}

//=========================================================
// GibMonster
//=========================================================
void CChubMonster::GibMonster()
{
	CBaseMonster::GibMonster();
}

//=========================================================
// Spawn
//=========================================================
void CChubMonster::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("monster_chumtoad");

	SET_MODEL(ENT(pev), "models/chumtoad.mdl");
	UTIL_SetSize(pev, Vector(-9, -8, 0), Vector(9, 8, 16));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->effects = 0;
	pev->health = gSkillData.chumHealth;
	pev->view_ofs = Vector(0, 0, 8);// position of the eyes relative to monster's origin
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;
	m_iMode = CHUB_IDLE;


	MonsterInit();
	SetActivity(ACT_IDLE);
	SetUse(&CChubMonster::Use);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChubMonster::Precache()
{
	PRECACHE_MODEL("models/chumtoad.mdl");

	PRECACHE_SOUND("player/pl_slosh1.wav");
	PRECACHE_SOUND("player/pl_slosh3.wav");
	PRECACHE_SOUND("player/pl_slosh2.wav");
	PRECACHE_SOUND("player/pl_slosh4.wav");

	UTIL_PrecacheOther("weapon_chummy");

	PRECACHE_SOUND_ARRAY(pIdleSounds);
}

//=========================================================
// IdleSound
//=========================================================
#define CHUB_ATTN_IDLE (float)1.5
void CChubMonster::IdleSound()
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolue(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// Monster think
//=========================================================
void CChubMonster::MonsterThink()
{
	if (pev->deadflag == DEAD_NO)
	{
		if (FNullEnt(FIND_CLIENT_IN_PVS(edict())))
		{
			pev->nextthink = gpGlobals->time + RANDOM_FLOAT(1, 1.5);
		}
		else
			pev->nextthink = gpGlobals->time + 0.1;// keep monster thinking

	float flInterval = StudioFrameAdvance(); // animate

	UpdateShockEffect();
	RunAI();

	switch (m_iMode)
	{
	case	CHUB_IDLE:
	case	CHUB_EAT:
	{
		// if not moving, sample environment to see if anything scary is around. Do a radius search 'look' at random.
		if (RANDOM_LONG(0, 70) == 1)
		{
			Look(256);
			if (HasConditions(bits_COND_SEE_FEAR))
			{
				// if see something scary
				//ALERT ( at_aiconsole, "Scared\n" );
				Eat(10 + (RANDOM_LONG(0, 4)));// chummy will ignore bodies for 10 to 15 seconds
				PickNewDest(CHUB_SCARED_BY_ENT);
				SetActivity(ACT_WALK);
			}
			else if (RANDOM_LONG(0, 70) == 1)
			{
				// if chummy doesn't see anything, there's still a chance that it will move. (boredom)
				//ALERT ( at_aiconsole, "Bored\n" );
				PickNewDest(CHUB_BORED);
				SetActivity(ACT_WALK);

				if (m_iMode == CHUB_EAT)//chummy can't really eat, so it'll just inspect corpses out of curiosity
				{
					// chummy will ignore bodies for 10 to 15 seconds if it got bored while looking at corpses. 
					Eat(10 + (RANDOM_LONG(0, 4)));
				}
			}
		}

		// don't do this stuff if eating!
		if (m_iMode == CHUB_IDLE)
		{
			if (FShouldEat())
			{
				Listen();
			}
			if (HasConditions(bits_COND_SMELL_FOOD))
			{
				CSound* pSound;

				pSound = CSoundEnt::SoundPointerForIndex(m_iAudibleList);

				// chummy smells food and is just standing around. Go to food unless food isn't on same z-plane.
				if (pSound && fabs(pSound->m_vecOrigin.z - pev->origin.z) <= 3)
				{
					PickNewDest(CHUB_SMELL_FOOD);
					SetActivity(ACT_WALK);
					pev->speed *= 0.75;
				}
			}
		}

		break;
	}
	}
		if (m_flGroundSpeed != 0)
		{
			Move(flInterval);
		}
	}

}
//=========================================================
// chummy's move function
//=========================================================
void CChubMonster::Move(float flInterval)
{
	float		flWaypointDist;
	Vector		vecApex;

	// local move to waypoint.
	flWaypointDist = (m_Route[m_iRouteIndex].vecLocation - pev->origin).Length2D();
	MakeIdealYaw(m_Route[m_iRouteIndex].vecLocation);
	ChangeYaw(pev->yaw_speed);
	
	UTIL_MakeVectors(pev->angles);

	if (RANDOM_LONG(0, 2) == 1)
	{
		// randomly check for blocked path.(more random load balancing)
		if (!WALK_MOVE(ENT(pev), pev->ideal_yaw, 4, WALKMOVE_NORMAL))
		{
			SetActivity(ACT_IDLE);
			if (m_iMode == CHUB_SMELL_FOOD)
			{
				m_iMode = CHUB_EAT;
			}
			else
			{
				m_iMode = CHUB_IDLE;
			}
		}
	}

	WALK_MOVE(ENT(pev), pev->ideal_yaw, m_flGroundSpeed * flInterval, WALKMOVE_NORMAL);
	

	// if the waypoint is closer than step size, then stop after next step (ok for chummy to overshoot)
	if (flWaypointDist <= m_flGroundSpeed * flInterval)
	{
		// take truncated step and stop

		SetActivity(ACT_IDLE);

		if (m_iMode == CHUB_SMELL_FOOD)
		{
			m_iMode = CHUB_EAT;
		}
		else
		{
			m_iMode = CHUB_IDLE;
		}
	}

	if (RANDOM_LONG(0, 149) == 1 && m_iMode != CHUB_SMELL_FOOD)
	{
		// random skitter while moving as long as not on a b-line to get out of light or going to food
		PickNewDest(FALSE);
	}
}

//=========================================================
// Picks a new spot for chummy to run to.
//=========================================================
void CChubMonster::PickNewDest(int iCondition)
{
	Vector	vecNewDir;
	Vector	vecDest;
	float	flDist;

	m_iMode = iCondition;

	if (m_iMode == CHUB_SMELL_FOOD)
	{
		// find the food and go there.
		CSound* pSound;

		pSound = CSoundEnt::SoundPointerForIndex(m_iAudibleList);

		if (pSound)
		{
			m_Route[0].vecLocation.x = pSound->m_vecOrigin.x + (3 - RANDOM_LONG(0, 5));
			m_Route[0].vecLocation.y = pSound->m_vecOrigin.y + (3 - RANDOM_LONG(0, 5));
			m_Route[0].vecLocation.z = pSound->m_vecOrigin.z;
			m_Route[0].iType = bits_MF_TO_LOCATION;
			m_movementGoal = RouteClassify(m_Route[0].iType);
			return;
		}
	}

	do
	{
		// picks a random spot, requiring that it be at least 128 units away
		// else, the chummy will pick a spot too close to itself and run in 
		// circles. this is a hack but buys me time to work on the real monsters.
		vecNewDir.x = RANDOM_FLOAT(-1, 1);
		vecNewDir.y = RANDOM_FLOAT(-1, 1);
		flDist = 256 + (RANDOM_LONG(0, 255));
		vecDest = pev->origin + vecNewDir * flDist;

	} while ((vecDest - pev->origin).Length2D() < 128);

	m_Route[0].vecLocation.x = vecDest.x;
	m_Route[0].vecLocation.y = vecDest.y;
	m_Route[0].vecLocation.z = pev->origin.z;
	m_Route[0].iType = bits_MF_TO_LOCATION;
	m_movementGoal = RouteClassify(m_Route[0].iType);
}

//=========================================================
// CChubMonster - USE - pick him up and give to player
//=========================================================
void CChubMonster::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	if (pev->deadflag == DEAD_NO)
	{
		Vector vecToadOrigin = pActivator->pev->origin;
		Vector vecToadAngles = pActivator->pev->angles;

		CBaseEntity* pChumAmmo;
		pChumAmmo = CChubMonster::DropItem("weapon_chummy", vecToadOrigin, vecToadAngles);
		pChumAmmo->pev->effects |= EF_NODRAW;

		pev->solid = SOLID_NOT;
		UTIL_Remove(this);
	}
}

//=========================================================
// PrescheduleThink
//=========================================================
void CChubMonster::PrescheduleThink()
{
	
	// at random, initiate a blink if not already blinking or sleeping
	if ((pev->skin == 0) && RANDOM_LONG(0, 50) == 0)
	{// start blinking!
		pev->skin = 2;
	}
	else if (pev->skin != 0)
	{// already blinking
		pev->skin--;
	}
}

void CChubMonster::Killed(entvars_t* pevAttacker, int iGib)
{
	pev->skin = 2;
	CBaseMonster::Killed(pevAttacker, iGib);
}