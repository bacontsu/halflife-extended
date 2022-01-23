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
// Generic Monster - purely for scripted sequence work.
//=========================================================
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include "soundent.h"
#include	"talkmonster.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"


#define	SF_HEAD_CONTROLLER							8 
#define	SF_FOLLOWER_USE								32 
#define SF_CAP_DOORS								1024
//=========================================================
// Monster's Anim Events Go Here
//=========================================================

class CGenericMonster : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	int ISoundMask () override;
	int	ObjectCaps() override { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }

	// Override these to set behavior
	Schedule_t* GetScheduleOfType(int Type) override;
	Schedule_t* GetSchedule() override;

	CUSTOM_SCHEDULES;

};
LINK_ENTITY_TO_CLASS( monster_generic, CGenericMonster );

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CGenericMonster :: Classify ()
{
	return m_iClass ? m_iClass : CLASS_PLAYER_ALLY;
}

//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CGenericMonster :: SetYawSpeed ()
{
	int ys;

	switch ( m_Activity )
	{
	case ACT_IDLE:
	default:
		ys = 90;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CGenericMonster :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{
	case 0:
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
		break;
	}
}

//=========================================================
// ISoundMask - generic monster can't hear.
//=========================================================
int CGenericMonster :: ISoundMask ()
{
	return	bits_SOUND_NONE;
}

//=========================================================
// Spawn
//=========================================================
void CGenericMonster :: Spawn()
{
	Precache();

	SET_MODEL( ENT(pev), STRING(pev->model) );

/*
	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) )
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
*/

	if ( FStrEq( STRING(pev->model), "models/player.mdl" ) || FStrEq( STRING(pev->model), "models/holo.mdl" ) )
		UTIL_SetSize(pev, VEC_HULL_MIN, VEC_HULL_MAX);
	else
		UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;
	m_flFieldOfView		= 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState		= MONSTERSTATE_NONE;

	if (pev->health <= 0)
	{
		pev->health = 20;
	}

	MonsterInit();
	if (pev->spawnflags & SF_CAP_DOORS)
	{
		m_afCapability = bits_CAP_DOORS_GROUP;
	}

	if (pev->spawnflags & SF_HEAD_CONTROLLER)
	{
		m_afCapability = bits_CAP_TURN_HEAD;
	}
	

	MonsterInit();
	if (pev->spawnflags & SF_FOLLOWER_USE)
		SetUse(&CGenericMonster::FollowerUse);
	else SetUse(NULL);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CGenericMonster :: Precache()
{
	CTalkMonster::Precache();
	TalkInit();
	PRECACHE_MODEL( (char *)STRING(pev->model) );
}	

//=========================================================
// AI Schedules Specific to this monster
//=========================================================

Task_t	tlGenFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slGenFollow[] =
{
	{
		tlGenFollow,
		ARRAYSIZE(tlGenFollow),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlGenStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slGenStopFollowing[] =
{
	{
		tlGenStopFollowing,
		ARRAYSIZE(tlGenStopFollowing),
		0,
		0,
		"StopFollowing"
	},
};

Task_t	tlGenFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slGenFaceTarget[] =
{
	{
		tlGenFaceTarget,
		ARRAYSIZE(tlGenFaceTarget),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t	tlGenIdleSciStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slGenIdleSciStand[] =
{
	{
		tlGenIdleSciStand,
		ARRAYSIZE(tlGenIdleSciStand),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND |
		bits_COND_SMELL |
		bits_COND_CLIENT_PUSH |
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT |// sound flags
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		bits_SOUND_DANGER |
		bits_SOUND_MEAT |// scents
		bits_SOUND_CARCASS |
		bits_SOUND_GARBAGE,
		"IdleSciStand"

	},
};

DEFINE_CUSTOM_SCHEDULES(CGenericMonster)
{
	slGenFollow,
		slGenFaceTarget,
		slGenIdleSciStand,
		slGenStopFollowing
};

IMPLEMENT_CUSTOM_SCHEDULES(CGenericMonster, CTalkMonster);

Schedule_t* CGenericMonster::GetScheduleOfType(int Type)
{
	Schedule_t* psched;

	switch (Type)
	{
		// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

	case SCHED_TARGET_CHASE:
		return slGenFollow;

	case SCHED_CANT_FOLLOW:
		return slGenStopFollowing;
	}

	return CTalkMonster::GetScheduleOfType(Type);
}

Schedule_t* CGenericMonster::GetSchedule()
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity* pEnemy = m_hEnemy;

	switch (m_MonsterState)
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		// Behavior for following the player
		if (IsFollowing())
		{
			if (!m_hTargetEnt->IsAlive())
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing(FALSE);
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if (pEnemy != NULL)
				relationship = IRelationship(pEnemy);

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if (relationship != R_DL && relationship != R_HT)
			{
				// If I'm already close enough to my target
				if (TargetDistance() <= 128)
				{
					if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
						return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}


				return GetScheduleOfType(SCHED_TARGET_FACE);	// Just face and follow.
			}
		}
	}
	return CTalkMonster::GetSchedule();
}
