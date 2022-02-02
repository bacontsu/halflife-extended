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
* -----------------------------------------
* Half-Life:Extended code by trvps (blsha)
* -----------------------------------------
****/
//=========================================================
// human scientist (passive lab worker)
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"talkmonster.h"
#include	"schedule.h"
#include	"defaultai.h"
#include	"scripted.h"
#include	"animation.h"
#include	"soundent.h"


#define		NUM_SCIENTIST_SKINS			6 //four skins available for sci model

namespace SciBodygroup
{
	enum SciBodygroup
	{
		Head = 0,
		Clothing,
		Hands
	};
}
namespace SciHead
{
	enum SciHead
	{
		Random = -1,
		NerdHead = 0,
		EinstienHead = 1,
		LutherHead = 2,
		SlickHead = 3,
		EddsworthHead = 4,
		MichaelHead = 5,
		BorisHead = 6,
		MarleyHead = 7,
		JonnyHead = 8,
		ManuelHead = 9,
		AlfredHead = 10,
		KyleHead = 11,
		LeonelHead = 12,
		GusHead = 13,
		PolyHead = 14,
		TimHead = 15,
		TommyHead = 16,
		RosenHead = 17
	};
}
namespace SciClothing
{
	enum SciClothing
	{
		Random = -1,
		Coat = 0,
		Civ
	};
}
namespace SciHands
{
	enum SciHands
	{
		Empty = 0,
		Needle,
		Notebook,
		Stick,
		Newspaper,
		Book_red,
		Book_blue,
		Soda_green,
		Soda_brown
	};
}

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED,
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED,
};

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		SCIENTIST_AE_HEAL		( 1 )
#define		SCIENTIST_AE_NEEDLEON	( 2 )
#define		SCIENTIST_AE_NEEDLEOFF	( 3 )

//=======================================================
// Scientist
//=======================================================

class CScientist : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;

	void SetYawSpeed() override;
	int  Classify () override;
	void HandleAnimEvent( MonsterEvent_t *pEvent ) override;
	void RunTask( Task_t *pTask ) override;
	void StartTask( Task_t *pTask ) override;
	int	ObjectCaps() override { return CTalkMonster :: ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType) override;
	int FriendNumber( int arrayNumber ) override;
	void SetActivity ( Activity newActivity ) override;
	Activity GetStoppedActivity() override;
	int ISoundMask() override;
	void DeclineFollowing() override;

	float	CoverRadius() override { return 1200; }		// Need more room for cover because scientists want to get far away!
	BOOL	DisregardEnemy( CBaseEntity *pEnemy ) { return !pEnemy->IsAlive() || (gpGlobals->time - m_fearTime) > 15; }

	BOOL	CanHeal();
	void	Heal();
	void	Scream();

	// Override these to set behavior
	Schedule_t *GetScheduleOfType ( int Type ) override;
	Schedule_t *GetSchedule () override;
	MONSTERSTATE GetIdealState () override;

	void DeathSound() override;
	void PainSound() override;
	
	void TalkInit();

	void			Killed( entvars_t *pevAttacker, int iGib ) override;
	void KeyValue(KeyValueData* pkvd) override;

	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];

	int m_Head;
	int m_Clothing;
	int m_Hands;

	CUSTOM_SCHEDULES;

private:	
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

// that's a lot of entities
LINK_ENTITY_TO_CLASS( monster_scientist, CScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_hev, CScientist );
LINK_ENTITY_TO_CLASS(monster_rosenberg, CScientist);

TYPEDESCRIPTION	CScientist::m_SaveData[] = 
{
	DEFINE_FIELD( CScientist, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CScientist, m_fearTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CScientist, CTalkMonster );

//=========================================================
// AI Schedules Specific to this monster
//=========================================================
Task_t	tlFollow[] =
{
	//{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_CANT_FOLLOW },	// If you fail, bail out of follow
	{ TASK_MOVE_TO_TARGET_RANGE,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE },
};

Schedule_t	slFollow[] =
{
	{
		tlFollow,
		ARRAYSIZE ( tlFollow ),
		bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t	tlFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },// If you fail, follow normally
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED,(float)128		},	// Move within 128 of target ent (client)
//	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_FACE_SCARED },
};

Schedule_t	slFollowScared[] =
{
	{
		tlFollowScared,
		ARRAYSIZE ( tlFollowScared ),
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER,
		"FollowScared"
	},
};

Task_t	tlFaceTargetScared[] =
{
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE_SCARED },
};

Schedule_t	slFaceTargetScared[] =
{
	{
		tlFaceTargetScared,
		ARRAYSIZE ( tlFaceTargetScared ),
		bits_COND_HEAR_SOUND |
		bits_COND_NEW_ENEMY,
		bits_SOUND_DANGER,
		"FaceTargetScared"
	},
};

Task_t	tlStopFollowing[] =
{
	{ TASK_CANT_FOLLOW,		(float)0 },
};

Schedule_t	slStopFollowing[] =
{
	{
		tlStopFollowing,
		ARRAYSIZE ( tlStopFollowing ),
		0,
		0,
		"StopFollowing"
	},
};


Task_t	tlHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE,(float)50		},	// Move within 60 of target ent (client)
	{ TASK_SET_FAIL_SCHEDULE,	(float)SCHED_TARGET_CHASE },	// If you fail, catch up with that guy! (change this to put syringe away and then chase)
	{ TASK_FACE_IDEAL,			(float)0		},
	{ TASK_SAY_HEAL,			(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_ARM	},			// Whip out the needle
	{ TASK_HEAL,				(float)0	},	// Put it in the player
	{ TASK_PLAY_SEQUENCE_FACE_TARGET,		(float)ACT_DISARM	},			// Put away the needle
};

Schedule_t	slHeal[] =
{
	{
		tlHeal,
		ARRAYSIZE ( tlHeal ),
		0,	// Don't interrupt or he'll end up running around with a needle all the time
		0,
		"Heal"
	},
};


Task_t	tlFaceTarget[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_TARGET,			(float)0		},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_SET_SCHEDULE,		(float)SCHED_TARGET_CHASE },
};

Schedule_t	slFaceTarget[] =
{
	{
		tlFaceTarget,
		ARRAYSIZE ( tlFaceTarget ),
		bits_COND_CLIENT_PUSH |
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT |
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};


Task_t	tlSciPanic[] =
{
	{ TASK_STOP_MOVING,			(float)0		},
	{ TASK_FACE_ENEMY,			(float)0		},
	{ TASK_SCREAM,				(float)0		},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,		(float)ACT_EXCITED	},	// This is really fear-stricken excitement
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE	},
};

Schedule_t	slSciPanic[] =
{
	{
		tlSciPanic,
		ARRAYSIZE ( tlSciPanic ),
		0,
		0,
		"SciPanic"
	},
};


Task_t	tlIdleSciStand[] =
{
	{ TASK_STOP_MOVING,			0				},
	{ TASK_SET_ACTIVITY,		(float)ACT_IDLE },
	{ TASK_WAIT,				(float)2		}, // repick IDLESTAND every two seconds.
	{ TASK_TLK_HEADRESET,		(float)0		}, // reset head position
};

Schedule_t	slIdleSciStand[] =
{
	{ 
		tlIdleSciStand,
		ARRAYSIZE ( tlIdleSciStand ), 
		bits_COND_NEW_ENEMY		|
		bits_COND_LIGHT_DAMAGE	|
		bits_COND_HEAVY_DAMAGE	|
		bits_COND_HEAR_SOUND	|
		bits_COND_SMELL			|
		bits_COND_CLIENT_PUSH	|
		bits_COND_PROVOKED,

		bits_SOUND_COMBAT		|// sound flags
		//bits_SOUND_PLAYER		|
		//bits_SOUND_WORLD		|
		bits_SOUND_DANGER		|
		bits_SOUND_MEAT			|// scents
		bits_SOUND_CARCASS		|
		bits_SOUND_GARBAGE,
		"IdleSciStand"

	},
};


Task_t	tlScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FIND_COVER_FROM_ENEMY,	(float)0					},
	{ TASK_RUN_PATH_SCARED,			(float)0					},
	{ TASK_TURN_LEFT,				(float)179					},
	{ TASK_SET_SCHEDULE,			(float)SCHED_HIDE			},
};

Schedule_t	slScientistCover[] =
{
	{ 
		tlScientistCover,
		ARRAYSIZE ( tlScientistCover ), 
		bits_COND_NEW_ENEMY,
		0,
		"ScientistCover"
	},
};



Task_t	tlScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE,			(float)ACT_CROUCH			},
	{ TASK_SET_ACTIVITY,			(float)ACT_CROUCHIDLE		},	// FIXME: This looks lame
	{ TASK_WAIT_RANDOM,				(float)10.0					},
};

Schedule_t	slScientistHide[] =
{
	{ 
		tlScientistHide,
		ARRAYSIZE ( tlScientistHide ), 
		bits_COND_NEW_ENEMY |
		bits_COND_HEAR_SOUND |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER,
		"ScientistHide"
	},
};


Task_t	tlScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE,		(float)SCHED_PANIC },		// If you fail, just panic!
	{ TASK_RANDOM_SCREAM,			(float)0.3 },				// Scream 30% of the time
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCH			},
	{ TASK_RANDOM_SCREAM,			(float)0.1 },				// Scream again 10% of the time
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY,			(float)ACT_CROUCHIDLE		},
	{ TASK_WAIT_RANDOM,				(float)1.0					},
};

Schedule_t	slScientistStartle[] =
{
	{ 
		tlScientistStartle,
		ARRAYSIZE ( tlScientistStartle ), 
		bits_COND_NEW_ENEMY |
		bits_COND_SEE_ENEMY |
		bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR |
		bits_COND_SEE_DISLIKE,
		0,
		"ScientistStartle"
	},
};



Task_t	tlFear[] =
{
	{ TASK_STOP_MOVING,				(float)0					},
	{ TASK_FACE_ENEMY,				(float)0					},
	{ TASK_SAY_FEAR,				(float)0					},
//	{ TASK_PLAY_SEQUENCE,			(float)ACT_FEAR_DISPLAY		},
};

Schedule_t	slFear[] =
{
	{ 
		tlFear,
		ARRAYSIZE ( tlFear ), 
		bits_COND_NEW_ENEMY,
		0,
		"Fear"
	},
};


DEFINE_CUSTOM_SCHEDULES( CScientist )
{
	slFollow,
	slFaceTarget,
	slIdleSciStand,
	slFear,
	slScientistCover,
	slScientistHide,
	slScientistStartle,
	slHeal,
	slStopFollowing,
	slSciPanic,
	slFollowScared,
	slFaceTargetScared,
};


IMPLEMENT_CUSTOM_SCHEDULES( CScientist, CTalkMonster );


void CScientist::DeclineFollowing()
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	if (FClassnameIs(pev, "monster_rosenberg"))
		PlaySentence("RO_POK", 2, VOL_NORM, ATTN_NORM);
	else PlaySentence("SC_POK", 2, VOL_NORM, ATTN_NORM);
}


void CScientist :: Scream()
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		if (FClassnameIs(pev, "monster_rosenberg"))
			PlaySentence("RO_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
		else      PlaySentence("SC_SCREAM", RANDOM_FLOAT(3, 6), VOL_NORM, ATTN_NORM);
	}
}


Activity CScientist::GetStoppedActivity()
{ 
	if ( m_hEnemy != NULL ) 
		return ACT_EXCITED;
	return CTalkMonster::GetStoppedActivity();
}


void CScientist :: StartTask( Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_SAY_HEAL:
//		if ( FOkToSpeak() )
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		if (FClassnameIs(pev, "monster_rosenberg"))
			PlaySentence("RO_HEAL", 2, VOL_NORM, ATTN_IDLE);
		if (FClassnameIs(pev, "monster_scientist_female"))
			PlaySentence("FS_HEAL", 2, VOL_NORM, ATTN_IDLE);
		else	PlaySentence("SC_HEAL", 2, VOL_NORM, ATTN_IDLE);
		TaskComplete();
		break;

	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;

	case TASK_RANDOM_SCREAM:
		if ( RANDOM_FLOAT( 0, 1 ) < pTask->flData )
			Scream();
		TaskComplete();
		break;

	case TASK_SAY_FEAR:
		if ( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;
			if ( m_hEnemy->IsPlayer() )
				if (FClassnameIs(pev, "monster_rosenberg"))
				{
					PlaySentence("RO_FEAR", 5, VOL_NORM, ATTN_NORM);
				}
				else
				{
					if (m_hEnemy->IsPlayer())
						PlaySentence("SC_PLFEAR", 5, VOL_NORM, ATTN_NORM);
					else if (FClassnameIs(pev, "monster_janitor"))
							PlaySentence("CL_COMBAT", 5, VOL_NORM, ATTN_NORM);
					else if (FClassnameIs(pev, "monster_scientist_female"))
						PlaySentence("FS_COMBAT", 5, VOL_NORM, ATTN_NORM);
					else
						PlaySentence("SC_FEAR", 5, VOL_NORM, ATTN_NORM);
				}
		}
		TaskComplete();
		break;

	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;

	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( (m_hTargetEnt->pev->origin - pev->origin).Length() < 1 )
				TaskComplete();
			else
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				if ( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
					TaskFail();
			}
		}
		break;

	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CScientist :: RunTask( Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if ( MovementIsComplete() )
			TaskComplete();
		if ( RANDOM_LONG(0,31) < 8 )
			Scream();
		break;

	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		{
			if ( RANDOM_LONG(0,63)< 8 )
				Scream();

			if ( m_hEnemy == NULL )
			{
				TaskFail();
			}
			else
			{
				float distance;

				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				// Re-evaluate when you think your finished, or the target has moved too far
				if ( (distance < pTask->flData) || (m_vecMoveGoal - m_hTargetEnt->pev->origin).Length() > pTask->flData * 0.5 )
				{
					m_vecMoveGoal = m_hTargetEnt->pev->origin;
					distance = ( m_vecMoveGoal - pev->origin ).Length2D();
					FRefreshRoute();
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				if ( distance < pTask->flData )
				{
					TaskComplete();
					RouteClear();		// Stop moving
				}
				else if ( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
					m_movementActivity = ACT_WALK_SCARED;
				else if ( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
					m_movementActivity = ACT_RUN_SCARED;
			}
		}
		break;

	case TASK_HEAL:
		if ( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			if ( TargetDistance() > 90 )
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
			ChangeYaw( pev->yaw_speed );
		}
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CScientist :: Classify ()
{
	return m_iClass ? m_iClass : CLASS_HUMAN_PASSIVE;
}


//=========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
//=========================================================
void CScientist :: SetYawSpeed ()
{
	int ys;

	ys = 90;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	}

	pev->yaw_speed = ys;
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CScientist :: HandleAnimEvent( MonsterEvent_t *pEvent )
{
	switch( pEvent->event )
	{		
	case SCIENTIST_AE_HEAL:		// Heal my target (if within range)
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
		{
		SetBodygroup(SciBodygroup::Hands, SciHands::Needle);//pull the needle
		}
		break;
	case SCIENTIST_AE_NEEDLEOFF:
		{
		SetBodygroup(SciBodygroup::Hands, SciHands::Empty);//hide it back
		}
		break;

	default:
		CTalkMonster::HandleAnimEvent( pEvent );
	}
}

//=========================================================
// Spawn
//=========================================================
void CScientist :: Spawn()
{
	Precache( );

	if (FClassnameIs(pev, "monster_scientist_hev"))
		SET_MODEL(ENT(pev), "models/scientist_hev.mdl");

	else SET_MODEL(ENT(pev), "models/scientist.mdl");
	
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	m_bloodColor		= BLOOD_COLOR_RED;

	if (FClassnameIs(pev, "monster_rosenberg"))
		pev->health = gSkillData.scientistHealth * 2;

	else	pev->health = gSkillData.scientistHealth;
	pev->view_ofs		= Vector ( 0, 0, 50 );// position of the eyes relative to monster's origin.
	m_flFieldOfView		= VIEW_FIELD_WIDE; // NOTE: we need a wide field of view so scientists will notice player and say hello
	m_MonsterState		= MONSTERSTATE_NONE;

	// this block is for backwards compatibility with early HL:E maps and Valve maps
	if (m_Head == 0 && m_Clothing == 0 && m_Hands == 0 && m_SpawnChance == 0)// all these parameters are empty, probably a deprecated scientist
	{
		pev->body = -1;
		pev->skin = -1;
	}
	if (pev->body == -1)
	{
		m_Head = SciHead::Random;
		m_Clothing = SciClothing::Random;
	}


	// picking random sub-models and setting them
	if (m_Head == SciHead::Random)
	{
		m_Head = RANDOM_LONG(0, 16);
	}
	if (m_Clothing == SciClothing::Random && !FClassnameIs(pev, "monster_scientist_hev"))
	{
		m_Clothing = RANDOM_LONG(0, 1);
	}
	if (FClassnameIs(pev, "monster_rosenberg"))
	{
		m_Head = SciHead::RosenHead;
	}
	if (FClassnameIs(pev, "monster_scientist_hev"))
	{
		SetBodygroup(1, m_Head);
		SetBodygroup(SciBodygroup::Hands, m_Hands);
	}
	else
	{
		SetBodygroup(SciBodygroup::Head, m_Head);
		SetBodygroup(SciBodygroup::Clothing, m_Clothing);
		SetBodygroup(SciBodygroup::Hands, m_Hands);
	}

//	m_flDistTooFar		= 256.0;

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;
	
	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, NUM_SCIENTIST_SKINS - 1);// pick a skin, any skin
	}

	MonsterInit();
	SetUse(&CScientist::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CScientist :: Precache()
{
	if(FClassnameIs(pev, "monster_scientist_hev"))
		PRECACHE_MODEL("models/scientist_hev.mdl");
	else PRECACHE_MODEL("models/scientist.mdl");


	PRECACHE_SOUND("scientist/sci_pain1.wav");
	PRECACHE_SOUND("scientist/sci_pain2.wav");
	PRECACHE_SOUND("scientist/sci_pain3.wav");
	PRECACHE_SOUND("scientist/sci_pain4.wav");
	PRECACHE_SOUND("scientist/sci_pain5.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}	

// Init talk data
void CScientist :: TalkInit()
{
	
	CTalkMonster::TalkInit();

	// scientist will try to talk to friends in this order:
	/*m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";
	m_szFriends[3] = "monster_barney_hev";
	m_szFriends[4] = "monster_scientist_female";
	m_szFriends[5] = "monster_rosenberg";
	m_szFriends[6] = "monster_janitor";
	m_szFriends[7] = "monster_worker";
	m_szFriends[8] = "monster_construction";
	m_szFriends[9] = "monster_chef";*/

	// scientists speach group names (group names are in sentences.txt)
	if (FClassnameIs(pev, "monster_rosenberg"))
	{
		m_szGrp[TLK_ANSWER] = "RO_ANSWER";
		m_szGrp[TLK_QUESTION] = "RO_QUESTION";
		m_szGrp[TLK_IDLE] = "RO_IDLE";
		m_szGrp[TLK_STARE] = "RO_STARE";
		m_szGrp[TLK_USE] = "RO_OK";
		m_szGrp[TLK_UNUSE] = "RO_WAIT";
		m_szGrp[TLK_STOP] = "RO_STOP";
		m_szGrp[TLK_NOSHOOT] = "RO_SCARED";
		m_szGrp[TLK_HELLO] = "RO_HELLO";

		m_szGrp[TLK_PLHURT1] = "!RO_CUREA";
		m_szGrp[TLK_PLHURT2] = "!RO_CUREB";
		m_szGrp[TLK_PLHURT3] = "!RO_CUREC";

		m_szGrp[TLK_PHELLO] = "RO_PHELLO";
		m_szGrp[TLK_PIDLE] = "RO_PIDLE";
		m_szGrp[TLK_PQUESTION] = "RO_PQUEST";
		m_szGrp[TLK_SMELL] = "RO_SMELL";

		m_szGrp[TLK_WOUND] = "RO_WOUND";
		m_szGrp[TLK_MORTAL] = "RO_MORTAL";
	}
	else
	{
		m_szGrp[TLK_ANSWER] = "SC_ANSWER";
		m_szGrp[TLK_QUESTION] = "SC_QUESTION";
		m_szGrp[TLK_IDLE] = "SC_IDLE";
		m_szGrp[TLK_STARE] = "SC_STARE";
		m_szGrp[TLK_USE] = "SC_OK";
		m_szGrp[TLK_UNUSE] = "SC_WAIT";
		m_szGrp[TLK_STOP] = "SC_STOP";
		m_szGrp[TLK_NOSHOOT] = "SC_SCARED";
		m_szGrp[TLK_HELLO] = "SC_HELLO";

		m_szGrp[TLK_PLHURT1] = "!SC_CUREA";
		m_szGrp[TLK_PLHURT2] = "!SC_CUREB";
		m_szGrp[TLK_PLHURT3] = "!SC_CUREC";

		m_szGrp[TLK_PHELLO] = "SC_PHELLO";
		m_szGrp[TLK_PIDLE] = "SC_PIDLE";
		m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
		m_szGrp[TLK_SMELL] = "SC_SMELL";

		m_szGrp[TLK_WOUND] = "SC_WOUND";
		m_szGrp[TLK_MORTAL] = "SC_MORTAL";
	}

	// some voice diversity
	m_voicePitch = 93 + RANDOM_LONG(0, 10);
	
}

int CScientist :: TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
{

	if ( pevInflictor && pevInflictor->flags & FL_CLIENT )
	{
		if (!FClassnameIs(pev, "monster_rosenberg"))// Rosy is important, don't let him stop following the player
		{
			Remember(bits_MEMORY_PROVOKED);
			StopFollowing(TRUE);
		}
	}

	// make sure friends talk about it if player hurts scientist...
	return CTalkMonster::TakeDamage(pevInflictor, pevAttacker, flDamage, bitsDamageType);
}


//=========================================================
// ISoundMask - returns a bit mask indicating which types
// of sounds this monster regards. In the base class implementation,
// monsters care about all sounds, but no scents.
//=========================================================
int CScientist :: ISoundMask ()
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
// PainSound
//=========================================================
void CScientist :: PainSound ()
{
	if (gpGlobals->time < m_painTime )
		return;
	
	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);
	if (FClassnameIs(pev, "monster_rosenberg"))
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "rosenberg/ro_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch());
	else
	{
		switch (RANDOM_LONG(0, 4))
		{
		case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
		}
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CScientist :: DeathSound ()
{
	PainSound();
}


void CScientist::Killed( entvars_t *pevAttacker, int iGib )
{
	SetUse( NULL );	
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CScientist::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	if (FClassnameIs(pev, "monster_scientist_hev"))
	{
		if (ptr->iHitgroup != HITGROUP_HEAD && ptr->iHitgroup != HITGROUP_STOMACH)
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
		if (ptr->iHitgroup == HITGROUP_STOMACH)
			flDamage *= 0.6;
	}
	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}


void CScientist :: SetActivity ( Activity newActivity )
{
	int	iSequence;

	iSequence = LookupActivity ( newActivity );

	// Set to the desired anim, or default anim if the desired is not present
	if ( iSequence == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}


Schedule_t* CScientist :: GetScheduleOfType ( int Type )
{
	Schedule_t *psched;

	switch( Type )
	{
	// Hook these to make a looping schedule
	case SCHED_TARGET_FACE:
		// call base class default so that scientist will talk
		// when 'used' 
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slFaceTarget;	// override this for different target face behavior
		else
			return psched;

	case SCHED_TARGET_CHASE:
		return slFollow;
	
	case SCHED_CANT_FOLLOW:
		return slStopFollowing;

	case SCHED_PANIC:
		return slSciPanic;

	case SCHED_TARGET_CHASE_SCARED:
		return slFollowScared;

	case SCHED_TARGET_FACE_SCARED:
		return slFaceTargetScared;

	case SCHED_IDLE_STAND:
		// call base class default so that scientist will talk
		// when standing during idle
		psched = CTalkMonster::GetScheduleOfType(Type);

		if (psched == slIdleStand)
			return slIdleSciStand;
		else
			return psched;

	case SCHED_HIDE:
		return slScientistHide;

	case SCHED_STARTLE:
		return slScientistStartle;

	case SCHED_FEAR:
		return slFear;
	}

	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t *CScientist :: GetSchedule ()
{
	// so we don't keep calling through the EHANDLE stuff
	CBaseEntity *pEnemy = m_hEnemy;

	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound *pSound;
		pSound = PBestSound();

		ASSERT( pSound != NULL );
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	switch( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:	
	case MONSTERSTATE_IDLE:
		if ( pEnemy )
		{
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
			{
				m_hEnemy = NULL;
				pEnemy = NULL;
			}
		}

		if ( HasConditions(bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE))
		{
			// flinch if hurt
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}

		// Cower when you hear something scary
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
		{
			CSound *pSound;
			pSound = PBestSound();

			ASSERT( pSound != NULL );
			if ( pSound )
			{
				if ( pSound->m_iType & (bits_SOUND_DANGER | bits_SOUND_COMBAT) )
				{
					if ( gpGlobals->time - m_fearTime > 3 )	// Only cower every 3 seconds or so
					{
						m_fearTime = gpGlobals->time;		// Update last fear
						return GetScheduleOfType( SCHED_STARTLE );	// This will just duck for a second
					}
				}
			}
		}

		// Behavior for following the player
		if ( IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				// UNDONE: Comment about the recently dead player here?
				StopFollowing( FALSE );
				break;
			}

			int relationship = R_NO;

			// Nothing scary, just me and the player
			if ( pEnemy != NULL )
				relationship = IRelationship( pEnemy );

			// UNDONE: Model fear properly, fix R_FR and add multiple levels of fear
			if ( relationship != R_DL && relationship != R_HT )
			{
				// If I'm already close enough to my target
				if (TargetDistance() <= 128)
				{
					if (CanHeal() == TRUE)	// Heal opportunistically
						return slHeal;
					if (HasConditions(bits_COND_CLIENT_PUSH))	// Player wants me to move
						return GetScheduleOfType(SCHED_MOVE_AWAY_FOLLOW);
				}
				
				
				return GetScheduleOfType( SCHED_TARGET_FACE );	// Just face and follow.
			}
			else if (!FClassnameIs(pev, "monster_rosenberg"))	// UNDONE: When afraid, scientist won't move out of your way.  Keep This?  If not, write move away scared
			{
				if ( HasConditions( bits_COND_NEW_ENEMY ) ) // I just saw something new and scary, react
					return GetScheduleOfType( SCHED_FEAR );					// React to something scary
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );	// face and follow, but I'm scared!
			}
		}

		if ( HasConditions( bits_COND_CLIENT_PUSH ) )	// Player wants me to move
			return GetScheduleOfType( SCHED_MOVE_AWAY );

		// try to say something about smells
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
			return slFear;					// Point and scream!
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return slScientistCover;		// Take Cover
		
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;	// Cower and panic from the scary sound!

		return slScientistCover;			// Run & Cower
		break;
	}
	
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CScientist :: GetIdealState ()
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( IsFollowing() )
			{
				int relationship = IRelationship( m_hEnemy );
				if ( relationship != R_FR || relationship != R_HT && !HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					// Don't go to combat if you're following the player
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
				StopFollowing( TRUE );
			}
		}
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		{
			// Stop following if you take damage
			if ( IsFollowing() )
				StopFollowing( TRUE );
		}
		break;

	case MONSTERSTATE_COMBAT:
		{
			CBaseEntity *pEnemy = m_hEnemy;
			if ( pEnemy != NULL )
			{
				if ( DisregardEnemy( pEnemy ) )		// After 15 seconds of being hidden, return to alert
				{
					// Strip enemy when going to alert
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					m_hEnemy = NULL;
					return m_IdealMonsterState;
				}
				// Follow if only scared a little
				if ( m_hTargetEnt != NULL )
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}

				if ( HasConditions ( bits_COND_SEE_ENEMY ) )
				{
					m_fearTime = gpGlobals->time;
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return m_IdealMonsterState;
				}

			}
		}
		break;
	}

	return CTalkMonster::GetIdealState();
}


BOOL CScientist::CanHeal()
{ 
	if (FClassnameIs(pev, "monster_rosenberg") || FClassnameIs(pev, "monster_scientist") || FClassnameIs(pev, "monster_scientist_female"))
	{
		if ((m_healTime > gpGlobals->time) || (m_hTargetEnt == NULL) || (m_hTargetEnt->pev->health > (m_hTargetEnt->pev->max_health * 0.5)))
			return FALSE;

		return TRUE;
	}
	else return FALSE;
}

void CScientist::Heal()
{
	if ( !CanHeal() )
		return;

	Vector target = m_hTargetEnt->pev->origin - pev->origin;
	if ( target.Length() > 100 )
		return;

	m_hTargetEnt->TakeHealth( gSkillData.scientistHeal, DMG_GENERIC );
	// Don't heal again for 1 minute
	m_healTime = gpGlobals->time + 60;
}

int CScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}

void CScientist::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("clothing", pkvd->szKeyName))
	{
		m_Clothing = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hands", pkvd->szKeyName))
	{
		m_Hands = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}

//=========================================================
// Dead Scientist PROP
//=========================================================
class CDeadScientist : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify () override { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue( KeyValueData *pkvd ) override;

	int m_Head;
	int m_Clothing;
	int m_Hands;

	int	m_iPose;// which sequence to display
	static const char *m_szPoses[12];
};
const char *CDeadScientist::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3", "floater_1", "floater_2", "floater_3", "scientist_deadpose1", "dead_against_wall" };

void CDeadScientist::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("clothing", pkvd->szKeyName))
	{
		m_Clothing = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hands", pkvd->szKeyName))
	{
		m_Hands = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}
LINK_ENTITY_TO_CLASS( monster_scientist_dead, CDeadScientist );
LINK_ENTITY_TO_CLASS( monster_scientist_hev_dead, CDeadScientist );

//
// ********** DeadScientist SPAWN **********
//
void CDeadScientist :: Spawn( )
{
	if (FClassnameIs(pev, "monster_scientist_hev"))
	{
		SET_MODEL(ENT(pev), "models/scientist_hev.mdl");
		PRECACHE_MODEL("models/scientist_hev.mdl");
	}
	else
	{
		PRECACHE_MODEL("models/scientist.mdl");
		SET_MODEL(ENT(pev), "models/scientist.mdl");
	}
	
	
	pev->effects		= 0;
	pev->sequence		= 0;
	// Corpses have less health
	pev->health			= 8;//gSkillData.scientistHealth;
	
	m_bloodColor = BLOOD_COLOR_RED;

	// picking random sub-models and setting them
	if (m_Head == SciHead::Random)
	{
		m_Head = RANDOM_LONG(0, 16);
	}
	if (m_Clothing == SciClothing::Random)
	{
		m_Clothing = RANDOM_LONG(0, 1);
	}
	if (FClassnameIs(pev, "monster_scientist_hev"))
	{
		SetBodygroup(1, m_Head);
		SetBodygroup(SciBodygroup::Hands, m_Hands);
	}
	else
	{
		SetBodygroup(SciBodygroup::Head, m_Head);
		SetBodygroup(SciBodygroup::Clothing, m_Clothing);
		SetBodygroup(SciBodygroup::Hands, m_Hands);
	}

	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, NUM_SCIENTIST_SKINS - 1);// pick a skin, any skin
	}

	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if (pev->sequence == -1)
	{
		ALERT ( at_console, "Dead scientist with bad pose\n" );
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//=========================================================
// Sitting Scientist PROP
//=========================================================

class CSittingScientist : public CScientist // kdb: changed from public CBaseMonster so he can speak
{
public:
	void Spawn() override;
	void  Precache() override;

	void EXPORT SittingThink();
	int	Classify () override;
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	static	TYPEDESCRIPTION m_SaveData[];


	void SetAnswerQuestion( CTalkMonster *pSpeaker ) override;
	int FriendNumber( int arrayNumber ) override;

	int FIdleSpeak ();
	int		m_baseSequence;	
	int		m_headTurn;
	float	m_flResponseDelay;
};

LINK_ENTITY_TO_CLASS( monster_sitting_scientist, CSittingScientist );
TYPEDESCRIPTION	CSittingScientist::m_SaveData[] = 
{
	// Don't need to save/restore m_baseSequence (recalced)
	DEFINE_FIELD( CSittingScientist, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingScientist, m_flResponseDelay, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( CSittingScientist, CScientist );

// animation sequence aliases 
typedef enum
{
SITTING_ANIM_sitlookleft,
SITTING_ANIM_sitlookright,
SITTING_ANIM_sitscared,
SITTING_ANIM_sitting2,
SITTING_ANIM_sitting3
} SITTING_ANIM;


//
// ********** Scientist SPAWN **********
//
void CSittingScientist :: Spawn( )
{
	PRECACHE_MODEL("models/scientist.mdl");
	SET_MODEL(ENT(pev), "models/scientist.mdl");
	Precache();
	InitBoneControllers();

	UTIL_SetSize(pev, Vector(-14, -14, 0), Vector(14, 14, 36));

	pev->solid			= SOLID_SLIDEBOX;
	pev->movetype		= MOVETYPE_STEP;
	pev->effects		= 0;
	pev->health			= 50;
	
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView		= VIEW_FIELD_WIDE; // indicates the width of this monster's forward view cone ( as a dotproduct result )

	m_afCapability		= bits_CAP_HEAR | bits_CAP_TURN_HEAD;

	SetBits(pev->spawnflags, SF_MONSTER_PREDISASTER); // predisaster only!

	if (m_Head == SciHead::Random)
	{
		m_Head = RANDOM_LONG(0, 16);
	}
	if (m_Clothing == SciClothing::Random)
	{
		m_Clothing = RANDOM_LONG(0, 1);
	}
	SetBodygroup(SciBodygroup::Head, m_Head);
	SetBodygroup(SciBodygroup::Clothing, m_Clothing);
	SetBodygroup(SciBodygroup::Hands, m_Hands);

	if (pev->skin == -1)
	{
		pev->skin = RANDOM_LONG(0, NUM_SCIENTIST_SKINS - 1);// pick a skin, any skin
	}

	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG(0,4);
	ResetSequenceInfo( );
	
	SetThink (&CSittingScientist::SittingThink);
	pev->nextthink = gpGlobals->time + 0.1;

	DROP_TO_FLOOR ( ENT(pev) );
}

void CSittingScientist :: Precache()
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

//=========================================================
// ID as a passive human
//=========================================================
int	CSittingScientist :: Classify ()
{
	return	CLASS_HUMAN_PASSIVE;
}


int CSittingScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber < 3 )
		return array[ arrayNumber ];
	return arrayNumber;
}



//=========================================================
// sit, do stuff
//=========================================================
void CSittingScientist :: SittingThink()
{
	CBaseEntity *pent;	

	StudioFrameAdvance( );

	// try to greet player
	if (FIdleHello())
	{
		pent = FindNearestFriend(TRUE);
		if (pent)
		{
			float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

			if (yaw > 180) yaw -= 360;
			if (yaw < -180) yaw += 360;
				
			if (yaw > 0)
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;
		
		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, 0 );
		}
	}
	else if (m_fSequenceFinished)
	{
		int i = RANDOM_LONG(0,99);
		m_headTurn = 0;
		
		if (m_flResponseDelay && gpGlobals->time > m_flResponseDelay)
		{
			// respond to question
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if (i < 30)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	

			// turn towards player or nearest friend and speak

			if (!FBitSet(m_bitsSaid, bit_saidHelloPlayer))
				pent = FindNearestFriend(TRUE);
			else
				pent = FindNearestFriend(FALSE);

			if (!FIdleSpeak() || !pent)
			{	
				m_headTurn = RANDOM_LONG(0,8) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				// only turn head if we spoke
				float yaw = VecToYaw(pent->pev->origin - pev->origin) - pev->angles.y;

				if (yaw > 180) yaw -= 360;
				if (yaw < -180) yaw += 360;
				
				if (yaw > 0)
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;

				//ALERT(at_console, "sitting speak\n");
			}
		}
		else if (i < 60)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;	
			m_headTurn = RANDOM_LONG(0,8) * 10 - 40;
			if (RANDOM_LONG(0,99) < 5)
			{
				//ALERT(at_console, "sitting speak2\n");
				FIdleSpeak();
			}
		}
		else if (i < 80)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		}
		else if (i < 100)
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		}

		ResetSequenceInfo( );
		pev->frame = 0;
		SetBoneController( 0, m_headTurn );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

// prepare sitting scientist to answer a question
void CSittingScientist :: SetAnswerQuestion( CTalkMonster *pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT(3, 4);
	m_hTalkTarget = (CBaseMonster *)pSpeaker;
}


//=========================================================
// FIdleSpeak
// ask question of nearby friend, or make statement
//=========================================================
int CSittingScientist :: FIdleSpeak ()
{ 
	// try to start a conversation, or make statement
	int pitch;
	
	if (!FOkToSpeak())
		return FALSE;

	// set global min delay for next conversation
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);

	pitch = GetVoicePitch();
		
	// if there is a friend nearby to speak to, play sentence, set friend's response time, return

	// try to talk to any standing or sitting scientists nearby
	CBaseEntity *pentFriend = FindNearestFriend(FALSE);

	if (pentFriend && RANDOM_LONG(0,1))
	{
		CTalkMonster *pTalkMonster = GetClassPtr((CTalkMonster *)pentFriend->pev);
		pTalkMonster->SetAnswerQuestion( this );
		
		IdleHeadTurn(pentFriend->pev->origin);
		SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// otherwise, play an idle statement
	if (RANDOM_LONG(0,1))
	{
		SENTENCEG_PlayRndSz( ENT(pev), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch );
		// set global min delay for next conversation
		CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT(4.8, 5.2);
		return TRUE;
	}

	// never spoke
	CTalkMonster::g_talkWaitTime = 0;
	return FALSE;
}


//=========================================
// Worker, based off scientist
//=========================================

class CWorker : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	BOOL CanHeal(void);
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;
	void TalkInit();

	void PainSound();
	void DeathSound();

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;

};

LINK_ENTITY_TO_CLASS(monster_worker, CWorker);

namespace WorkerHead
{
	enum WorkerHead
	{
		Random = -1,
		edwart = 0,
		michael,
		boris,
		marley,
		jonny,
		manuel,
		alfred,
		kyle,
		leonel,
		gus,
		poly,
		tim,
		tommy,
		smbdy
	};
}

namespace WorkerHat
{
	enum WorkerHat
	{
		Random = -1,
		No = 0,
		Yes = 1
		
	};
}
namespace WorkerHands
{
	enum WorkerHands
	{
		Random = -1,
		Empty = 0,
		Notepad,
		Wrench,
	};
}

namespace WorkerBodygroup
{
	enum WorkerBodygroup
	{
		Head = 0,
		Hat = 2,
		Hands,
	};
}

BOOL CWorker::CanHeal()
{
		return FALSE;
};

void CWorker::DeathSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_die1.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_die2.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

void CWorker::PainSound()
{
	switch (RANDOM_LONG(0, 4))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain1.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain2.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain3.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain4.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain5.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CWorker::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CTalkMonster::HandleAnimEvent(pEvent);
}
//=====================================
// Spawn
//=====================================
void CWorker::Spawn(void)
{
	Precache( );

	SET_MODEL(ENT(pev), "models/worker.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;

	// position of the eyes relative to monster's origin.
	pev->view_ofs = Vector(0, 0, 50);

	// NOTE: we need a wide field of view so scientists will notice player and say hello
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;

	//     m_flDistTooFar          = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	if (m_Head == WorkerHead::Random)
		m_Head = RANDOM_LONG(0, 13);
	if (m_Clothing == WorkerHat::Random)
		m_Clothing = RANDOM_LONG(0, 1);
	if (pev->skin == -1)
		pev->skin = RANDOM_LONG(0, 3);

	SetBodygroup(WorkerBodygroup::Head, m_Head);
	SetBodygroup(WorkerBodygroup::Hands, m_Hands);
	SetBodygroup(WorkerBodygroup::Hat, m_Clothing);


	MonsterInit();
	SetUse(&CWorker::FollowerUse);
}

void CWorker::TalkInit()
{
	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "CN_ANSWER";
	m_szGrp[TLK_QUESTION] = "CN_QUESTION";
	m_szGrp[TLK_IDLE] = "CN_IDLE";
	m_szGrp[TLK_STARE] = NULL;
	m_szGrp[TLK_USE] = "CN_OK";
	m_szGrp[TLK_UNUSE] = "CN_WAIT";
	m_szGrp[TLK_STOP] = NULL;
	m_szGrp[TLK_NOSHOOT] = NULL;
	m_szGrp[TLK_HELLO] = NULL;

	m_szGrp[TLK_PLHURT1] = NULL;
	m_szGrp[TLK_PLHURT2] = NULL;
	m_szGrp[TLK_PLHURT3] = NULL;

	m_szGrp[TLK_PHELLO] = NULL;
	m_szGrp[TLK_PIDLE] = NULL;
	m_szGrp[TLK_PQUESTION] = NULL;
	m_szGrp[TLK_SMELL] = NULL;

	m_szGrp[TLK_WOUND] = NULL;
	m_szGrp[TLK_MORTAL] = NULL;


	m_voicePitch = 93 + RANDOM_LONG(0, 13);// some voice diversity
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CWorker::Precache(void)
{
	PRECACHE_MODEL("models/worker.mdl");
	PRECACHE_SOUND("conwork/work_pain1.wav");
	PRECACHE_SOUND("conwork/work_pain2.wav");
	PRECACHE_SOUND("conwork/work_pain3.wav");
	PRECACHE_SOUND("conwork/work_pain4.wav");
	PRECACHE_SOUND("conwork/work_pain5.wav");


	PRECACHE_SOUND("conwork/work_die1.wav");
	PRECACHE_SOUND("conwork/work_die2.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

void CWorker::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case 10:
		if (pev->body >= 5)
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
		else flDamage *= 2;
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

//=========================================
// Chef, based off scientist
//=========================================

class CChef : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void	Heal();
	BOOL CanHeal(void);

	int m_Hat;

	void KeyValue(KeyValueData* pkvd);

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS(monster_chef, CChef);

namespace ChefHead
{
	enum ChefHead
	{
		Random = -1,
		edwart = 0,
		michael,
		boris,
		marley,
		jonny,
		manuel,
		alfred,
		kyle,
		leonel,
		gus,
		poly,
		tim,
		tommy,
		smbdy
	};
}
namespace ChefBodygroup
{
	enum ChefBodygroup
	{
		Head = 0,
		Clothing = 1,
		Hat
	};
}
namespace ChefClothing
{
	enum ChefClothing
	{
		Random = -1,
		White = 0,
		Brown
	};
}
namespace ChefHat
{
	enum ChefHat
	{
		Random = -1,
		No = 0,
		Hat
	};
}

BOOL CChef::CanHeal(void)
{
		return FALSE;
}

void CChef::Heal()
{

	return;

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CChef::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CTalkMonster::HandleAnimEvent(pEvent);
}
//==================================
// Spawn
//=====================================
void CChef::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/chef.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;

	// position of the eyes relative to monster's origin.
	pev->view_ofs = Vector(0, 0, 50);

	// NOTE: we need a wide field of view so scientists will notice player and say hello
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;

	//     m_flDistTooFar          = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	if (m_Head == ChefHead::Random)
		m_Head = RANDOM_LONG(0, 13);
	if (m_Clothing == ChefClothing::Random)
		m_Clothing = RANDOM_LONG(0, 1);
	if (m_Hat == ChefHat::Random)
		m_Hat = RANDOM_LONG(0, 1);

	SetBodygroup(ChefBodygroup::Head, m_Head);
	SetBodygroup(ChefBodygroup::Clothing, m_Clothing);
	SetBodygroup(ChefBodygroup::Hat, m_Hat);

	MonsterInit();
	SetUse(&CChef::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CChef::Precache(void)
{
	PRECACHE_MODEL("models/chef.mdl");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

void CChef::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("clothing", pkvd->szKeyName))
	{
		m_Clothing = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hat", pkvd->szKeyName))
	{
		m_Hat = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}

//=========================================================
// Dead Worker PROP
//=========================================================
class CDeadWorker : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_HUMAN_PASSIVE; }

	int m_Head;
	int m_Hands;
	int m_Clothing;

	void KeyValue(KeyValueData* pkvd) override;
	int	m_iPose;// which sequence to display
	static const char* m_szPoses[7];
};
const char* CDeadWorker::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadWorker::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hands", pkvd->szKeyName))
	{
		m_Hands = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("clothing", pkvd->szKeyName))
	{
		m_Clothing = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}
LINK_ENTITY_TO_CLASS(monster_worker_dead, CDeadWorker);

//
// ********** DeadWorker SPAWN **********
//
void CDeadWorker::Spawn()
{
	PRECACHE_MODEL("models/worker.mdl");
	SET_MODEL(ENT(pev), "models/worker.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;
	pev->skin = 0;//white hands

	m_bloodColor = BLOOD_COLOR_RED;

	if (m_Head == WorkerHead::Random)
		m_Head = RANDOM_LONG(0, 13);
	if (m_Clothing == WorkerHat::Random)
		m_Clothing = RANDOM_LONG(0, 1);
	if (pev->skin == -1)
		pev->skin = RANDOM_LONG(0, 3);

	SetBodygroup(WorkerBodygroup::Head, m_Head);
	SetBodygroup(WorkerBodygroup::Hands, m_Hands);
	SetBodygroup(WorkerBodygroup::Hat, m_Clothing);

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead worker with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}

//=========================================================
// Dead Chef PROP
//=========================================================
class CDeadChef : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_HUMAN_PASSIVE; }

	int m_Head;
	int m_Clothing;
	int m_Hat;

	void KeyValue(KeyValueData* pkvd) override;
	int	m_iPose;// which sequence to display
	static const char* m_szPoses[7];
};
const char* CDeadChef::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadChef::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hat", pkvd->szKeyName))
	{
		m_Hat = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("clothing", pkvd->szKeyName))
	{
		m_Clothing = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}
LINK_ENTITY_TO_CLASS(monster_chef_dead, CDeadChef);

//
// ********** DeadChef SPAWN **********
//
void CDeadChef::Spawn()
{
	PRECACHE_MODEL("models/chef.mdl");
	SET_MODEL(ENT(pev), "models/chef.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;

	m_bloodColor = BLOOD_COLOR_RED;

	if (m_Head == ChefHead::Random)
		m_Head = RANDOM_LONG(0, 13);
	if (m_Clothing == ChefClothing::Random)
		m_Clothing = RANDOM_LONG(0, 1);
	if (m_Hat == ChefHat::Random)
		m_Hat = RANDOM_LONG(0, 1);

	SetBodygroup(ChefBodygroup::Head, m_Head);
	SetBodygroup(ChefBodygroup::Clothing, m_Clothing);
	SetBodygroup(ChefBodygroup::Hat, m_Hat);

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead chef with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}

//=========================================
// Janitor, based off scientist
//=========================================

class CJanitor : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void	Heal();
	BOOL CanHeal(void);

	void DeclineFollowing() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit();

	void KeyValue(KeyValueData* pkvd) override;

	int iHead;
	int iArms;

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS(monster_janitor, CJanitor);

#define		NUM_JAN_BODIES		6// five body variations available for cleaner model

namespace JanBodygroup
{
	enum JanBodygroup
	{
		Head = 1,
		Arms
	};
}

namespace JanHead
{
	enum JanHead
	{
		Random = -1,
		Otis = 0,
		Maxwell,
		Cletus,
		Bob,
		Gus,
		Nigel,
		Richard,
		Paky
	};
}
namespace JanArms
{
	enum JanArms
	{
		Random = -1,
		Default = 0,
		RolledUp
	};
}

void CJanitor::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		iHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("arms", pkvd->szKeyName))
	{
		iArms = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}

BOOL CJanitor::CanHeal(void)
{
		return FALSE;
}

void CJanitor::Heal()
{

	return;

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CJanitor::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CTalkMonster::HandleAnimEvent(pEvent);
}
//==================================
// Spawn
//=====================================
void CJanitor::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/janitor.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;

	// position of the eyes relative to monster's origin.
	pev->view_ofs = Vector(0, 0, 50);

	// NOTE: we need a wide field of view so scientists will notice player and say hello
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->body = 0;
	pev->skin = 0;


	//     m_flDistTooFar          = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	if (iHead == JanHead::Random)
	{
		iHead = RANDOM_LONG(0, 7);
	}
	if (iArms == JanArms::Random)
	{
		iArms = RANDOM_LONG(0, 1);
	}

	if (iHead > JanHead::Bob)
	{
		pev->skin = 1;
	}


	SetBodygroup(JanBodygroup::Head, iHead);
	SetBodygroup(JanBodygroup::Arms, iArms);
	

	MonsterInit();
	SetUse(&CJanitor::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CJanitor::Precache(void)
{
	PRECACHE_MODEL("models/janitor.mdl");

	PRECACHE_SOUND("cleaner/jan_pain1.wav");
	PRECACHE_SOUND("cleaner/jan_pain2.wav");
	PRECACHE_SOUND("cleaner/jan_pain3.wav");
	PRECACHE_SOUND("cleaner/jan_pain4.wav");
	PRECACHE_SOUND("cleaner/jan_pain5.wav");


	PRECACHE_SOUND("cleaner/jan_die1.wav");
	PRECACHE_SOUND("cleaner/jan_die2.wav");
	PRECACHE_SOUND("cleaner/jan_die3.wav");
	PRECACHE_SOUND("cleaner/jan_die4.wav");
	PRECACHE_SOUND("cleaner/jan_die5.wav");
	PRECACHE_SOUND("cleaner/jan_die6.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

// Init talk data
void CJanitor::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "CL_ANSWER";
	m_szGrp[TLK_QUESTION] = "CL_QUESTION";
	m_szGrp[TLK_IDLE] = NULL;
	m_szGrp[TLK_STARE] = NULL;
	m_szGrp[TLK_USE] = "CL_OK";
	m_szGrp[TLK_UNUSE] = "CL_WAIT";
	m_szGrp[TLK_STOP] = NULL;

	m_szGrp[TLK_NOSHOOT] = "CL_COMBAT";
	m_szGrp[TLK_HELLO] = NULL;

	m_szGrp[TLK_PLHURT1] = NULL;
	m_szGrp[TLK_PLHURT2] = NULL;
	m_szGrp[TLK_PLHURT3] = NULL;

	m_szGrp[TLK_PHELLO] = NULL;	//"OT_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;	//"OT_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = NULL;		// UNDONE

	m_szGrp[TLK_SMELL] = NULL;

	m_szGrp[TLK_WOUND] = NULL;
	m_szGrp[TLK_MORTAL] = NULL;

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
	if (iHead > JanHead::Bob)
	{
		m_voicePitch = 96;
	}
}

void CJanitor::DeclineFollowing()
{
	PlaySentence("CL_POK", 3, VOL_NORM, ATTN_NORM);
}

//=========================================================
// PainSound
//=========================================================
void CJanitor::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 4))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CJanitor::DeathSound()
{
	switch (RANDOM_LONG(0, 5))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_die4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_die5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 5: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "cleaner/jan_die6.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// Dead Janitor PROP
//=========================================================
class CDeadJanitor : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_HUMAN_PASSIVE; }

	int iHead;
	int iArms;

	void KeyValue(KeyValueData* pkvd) override;
	int	m_iPose;// which sequence to display
	static const char* m_szPoses[5];
};
const char* CDeadJanitor::m_szPoses[] = { "lying_on_back", "lying_on_side", "lying_on_stomach", "stuffed_in_vent", "laseridle" };

void CDeadJanitor::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		iHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("arms", pkvd->szKeyName))
	{
		iArms = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}
LINK_ENTITY_TO_CLASS(monster_janitor_dead, CDeadJanitor);

//
// ********** DeadJanitor SPAWN **********
//
void CDeadJanitor::Spawn()
{
	PRECACHE_MODEL("models/janitor.mdl");
	SET_MODEL(ENT(pev), "models/janitor.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;
	pev->skin = 0;//white hands

	m_bloodColor = BLOOD_COLOR_RED;

	if (iHead == JanHead::Random)
	{
		iHead = RANDOM_LONG(0, 7);
	}
	if (iArms == JanArms::Random)
	{
		iArms = RANDOM_LONG(0, 1);
	}

	if (iHead > JanHead::Bob)
	{
		pev->skin = 1;
	}

	SetBodygroup(JanBodygroup::Head, iHead);
	SetBodygroup(JanBodygroup::Arms, iArms);

	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead Janitor with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}

//=========================================
// Construction, based off scientist
//=========================================

class CConstruction : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	BOOL CanHeal(void);
	void Heal();
	void KeyValue(KeyValueData* pkvd) override;
	void TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType) override;

	void TalkInit();
	void DeclineFollowing();

	void DeathSound();
	void PainSound();

	int m_Helm;
	int m_Gloves;

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;

};

LINK_ENTITY_TO_CLASS(monster_construction, CConstruction);

namespace ConBodygroup
{
	enum ConBodygroup
	{
		Head = 1,
		Hands,
		Helm,
		Gloves
	};
}
namespace ConHead
{
	enum ConHead
	{
		Random = -1,
		Slick	= 0,
		Luhter,//black
		Edwart,
		Michael,
		Boris,
		Marley,
		Jonny,
		Manuel,//black
		Alfred,
		Kyle,
		Leonel,//black
		Gus,//black
		Poly,
		Tim,//black
		Tommy,
	};
}
namespace ConHelm
{
	enum ConHelm
	{
		Random = -1,
		No = 0,
		Helm
	};
}
namespace ConGloves
{
	enum ConGloves
	{
		Random = -1,
		No = 0,
		Gloves
	};
}
namespace ConHands
{
	enum ConHands
	{
		Random = -1,
		Empty = 0,
		Notepad,
		Wrench
	};
}

BOOL CConstruction::CanHeal()
{
	return FALSE;
};

void CConstruction::Heal()
{
	return;
};

void CConstruction::DeathSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_die1.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_die2.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

void CConstruction::PainSound()
{
	switch (RANDOM_LONG(0, 4))
	{
	case 0:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain1.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 1:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain2.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 2:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain3.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 3:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain4.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	case 4:
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "conwork/work_pain5.wav", VOL_NORM, ATTN_NORM, 0, GetVoicePitch());
		break;
	}
}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CConstruction::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CTalkMonster::HandleAnimEvent(pEvent);
}
//=====================================
// Spawn
//=====================================
void CConstruction::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/construction.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;


	// position of the eyes relative to monster's origin.
	pev->view_ofs = Vector(0, 0, 50);

	// NOTE: we need a wide field of view so scientists will notice player and say hello
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;

	//     m_flDistTooFar          = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	// White hands
	pev->skin = 0;

	if (m_Head == ConHead::Random)
	{
		m_Head = RANDOM_LONG(0, 14);
	}
	if (m_Helm == ConHelm::Random)
	{
		m_Helm = RANDOM_LONG(0, 1);
	}
	if (m_Hands == ConHands::Random)
	{
		m_Hands = RANDOM_LONG(0, 2);
	}
	if (m_Gloves == ConGloves::Random)
	{
		m_Gloves = RANDOM_LONG(0, 2);
	}

	SetBodygroup(ConBodygroup::Head, m_Head);
	SetBodygroup(ConBodygroup::Hands, m_Hands);
	SetBodygroup(ConBodygroup::Gloves, m_Gloves);
	SetBodygroup(ConBodygroup::Helm, m_Helm);

	if (m_Head == ConHead::Luhter || m_Head == ConHead::Manuel || m_Head == ConHead::Leonel || m_Head == ConHead::Gus || m_Head == ConHead::Tim)
	{
		pev->skin = 1;
	}

	MonsterInit();
	SetUse(&CConstruction::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CConstruction::Precache(void)
{

	PRECACHE_MODEL("models/construction.mdl");
	PRECACHE_SOUND("conwork/work_pain1.wav");
	PRECACHE_SOUND("conwork/work_pain2.wav");
	PRECACHE_SOUND("conwork/work_pain3.wav");
	PRECACHE_SOUND("conwork/work_pain4.wav");
	PRECACHE_SOUND("conwork/work_pain5.wav");


	PRECACHE_SOUND("conwork/work_die1.wav");
	PRECACHE_SOUND("conwork/work_die2.wav");
	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

void CConstruction::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)
	
	m_szGrp[TLK_ANSWER] = "CN_ANSWER";
	m_szGrp[TLK_QUESTION] = "CN_QUESTION";
	m_szGrp[TLK_IDLE] = "CN_IDLE";
	m_szGrp[TLK_STARE] = NULL;
	m_szGrp[TLK_USE] = "CN_OK";
	m_szGrp[TLK_UNUSE] = "CN_WAIT";
	m_szGrp[TLK_STOP] = NULL;
	m_szGrp[TLK_NOSHOOT] = NULL;
	m_szGrp[TLK_HELLO] = NULL;

	m_szGrp[TLK_PLHURT1] = NULL;
	m_szGrp[TLK_PLHURT2] = NULL;
	m_szGrp[TLK_PLHURT3] = NULL;

	m_szGrp[TLK_PHELLO] = NULL;
	m_szGrp[TLK_PIDLE] = NULL;
	m_szGrp[TLK_PQUESTION] = NULL;
	m_szGrp[TLK_SMELL] = NULL;

	m_szGrp[TLK_WOUND] = NULL;
	m_szGrp[TLK_MORTAL] = NULL;
	

	m_voicePitch = 93 + RANDOM_LONG(0, 13);// some voice diversity
}

void CConstruction::DeclineFollowing()
{
	PlaySentence("CN_POK", 3, VOL_NORM, ATTN_NORM);
}

void CConstruction::TraceAttack(entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType)
{
	switch (ptr->iHitgroup)
	{
	case 10:
		if (m_Helm == ConHelm::Helm)
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
		else flDamage *= 2;
		// always a head shot
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}

	CTalkMonster::TraceAttack(pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
}

void CConstruction::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hands", pkvd->szKeyName))
	{
		m_Hands = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("gloves", pkvd->szKeyName))
	{
		m_Gloves = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("helmet", pkvd->szKeyName))
	{
		m_Helm = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}

//=========================================================
// Dead Construction PROP
//=========================================================
class CDeadConstruction : public CBaseMonster
{
public:
	void Spawn() override;
	int	Classify() override { return	CLASS_HUMAN_PASSIVE; }

	void KeyValue(KeyValueData* pkvd) override;
	int	m_iPose;// which sequence to display
	static const char* m_szPoses[7];
	int m_Head;
	int m_Hands;
	int m_Helm;
	int m_Gloves;
};
const char* CDeadConstruction::m_szPoses[] = { "lying_on_back", "lying_on_stomach", "dead_sitting", "dead_hang", "dead_table1", "dead_table2", "dead_table3" };

void CDeadConstruction::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "pose"))
	{
		m_iPose = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	if (FStrEq("head", pkvd->szKeyName))
	{
		m_Head = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("helmet", pkvd->szKeyName))
	{
		m_Helm = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("hands", pkvd->szKeyName))
	{
		m_Hands = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("gloves", pkvd->szKeyName))
	{
		m_Gloves = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
		CBaseMonster::KeyValue(pkvd);
}
LINK_ENTITY_TO_CLASS(monster_construction_dead, CDeadConstruction);

//
// ********** DeadConstruction SPAWN **********
//
void CDeadConstruction::Spawn()
{
	PRECACHE_MODEL("models/construction.mdl");
	SET_MODEL(ENT(pev), "models/construction.mdl");

	pev->effects = 0;
	pev->sequence = 0;
	// Corpses have less health
	pev->health = 8;//gSkillData.scientistHealth;

	// White hands
	pev->skin = 0;

	if (m_Head == ConHead::Random)
	{
		m_Head = RANDOM_LONG(0, 14);
	}
	if (m_Helm == ConHelm::Random)
	{
		m_Helm = RANDOM_LONG(0, 1);
	}
	if (m_Hands == ConHands::Random)
	{
		m_Hands = RANDOM_LONG(0, 2);
	}
	if (m_Gloves == ConGloves::Random)
	{
		m_Gloves = RANDOM_LONG(0, 2);
	}

	SetBodygroup(ConBodygroup::Head, m_Head);
	SetBodygroup(ConBodygroup::Hands, m_Hands);
	SetBodygroup(ConBodygroup::Gloves, m_Gloves);
	SetBodygroup(ConBodygroup::Helm, m_Helm);

	if (m_Head == ConHead::Luhter || m_Head == ConHead::Manuel || m_Head == ConHead::Leonel || m_Head == ConHead::Gus || m_Head == ConHead::Tim)
	{
		pev->skin = 1;
	}


	pev->sequence = LookupSequence(m_szPoses[m_iPose]);
	if (pev->sequence == -1)
	{
		ALERT(at_console, "Dead construction with bad pose\n");
	}

	//	pev->skin += 2; // use bloody skin -- UNDONE: Turn this back on when we have a bloody skin again!
	MonsterInitDead();
}


//= ========================================
// Janitor, based off scientist
//=========================================

class CFemSci : public CScientist
{
public:
	void Spawn(void);
	void Precache(void);
	void HandleAnimEvent(MonsterEvent_t* pEvent);
	void	Heal();
	BOOL CanHeal(void);

	void DeclineFollowing() override;

	void DeathSound() override;
	void PainSound() override;

	void TalkInit();

	void KeyValue(KeyValueData* pkvd) override;

	int iHead;
	int iArms;

private:
	float m_painTime;
	float m_healTime;
	float m_fearTime;
};

LINK_ENTITY_TO_CLASS(monster_scientist_female, CFemSci);

namespace FemSciBodygroup
{
	enum FemSciBodygroup
	{
		Head = 1,
		Arms
	};
}

namespace FemSciHead
{
	enum FemSciHead
	{
		Random = -1,
		Yelene = 0,
		Ana,
		Lorena,
		Martha,
		Silvia,
		Maria,
		Colette,
		Gina
	};
}
namespace FemSciArms
{
	enum FemSciArms
	{
		Random = -1,
		Default = 0,
		Needle,
		Notebook,
		Stick
	};
}

void CFemSci::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq("head", pkvd->szKeyName))
	{
		iHead = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	if (FStrEq("arms", pkvd->szKeyName))
	{
		iArms = atoi(pkvd->szValue);
		pkvd->fHandled = true;
	}
	else
	{
		CBaseMonster::KeyValue(pkvd);
	}
}

BOOL CFemSci::CanHeal(void)
{
	return FALSE;
}

void CFemSci::Heal()
{

	return;

}

//=========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played.
//=========================================================
void CFemSci::HandleAnimEvent(MonsterEvent_t* pEvent)
{
	CTalkMonster::HandleAnimEvent(pEvent);
}
//==================================
// Spawn
//=====================================
void CFemSci::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/scientist_female.mdl");
	UTIL_SetSize(pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;

	// position of the eyes relative to monster's origin.
	pev->view_ofs = Vector(0, 0, 50);

	// NOTE: we need a wide field of view so scientists will notice player and say hello
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->body = 0;
	if (pev->skin == -1)
		pev->skin = RANDOM_LONG(0, 7);


	//     m_flDistTooFar          = 256.0;

	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;

	if (iHead == FemSciHead::Random)
	{
		iHead = RANDOM_LONG(0, 7);
	}
	if (iArms == FemSciArms::Random)
	{
		iArms = RANDOM_LONG(0, 3);
	}

	SetBodygroup(FemSciBodygroup::Head, iHead);
	SetBodygroup(FemSciBodygroup::Arms, iArms);

	MonsterInit();
	SetUse(&CFemSci::FollowerUse);
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CFemSci::Precache(void)
{
	PRECACHE_MODEL("models/scientist_female.mdl");

	PRECACHE_SOUND("femsci/femsci_pain1.wav");
	PRECACHE_SOUND("femsci/femsci_pain2.wav");
	PRECACHE_SOUND("femsci/femsci_pain3.wav");
	PRECACHE_SOUND("femsci/femsci_pain4.wav");
	PRECACHE_SOUND("femsci/femsci_pain5.wav");


	PRECACHE_SOUND("femsci/femsci_die1.wav");
	PRECACHE_SOUND("femsci/femsci_die2.wav");

	// every new scientist must call this, otherwise
	// when a level is loaded, nobody will talk (time is reset to 0)
	TalkInit();

	CTalkMonster::Precache();
}

// Init talk data
void CFemSci::TalkInit()
{

	CTalkMonster::TalkInit();

	// scientists speach group names (group names are in sentences.txt)

	m_szGrp[TLK_ANSWER] = "FS_ANSWER";
	m_szGrp[TLK_QUESTION] = "FS_QUESTION";
	m_szGrp[TLK_IDLE] = NULL;
	m_szGrp[TLK_STARE] = NULL;
	m_szGrp[TLK_USE] = "FS_OK";
	m_szGrp[TLK_UNUSE] = NULL;
	m_szGrp[TLK_STOP] = NULL;

	m_szGrp[TLK_NOSHOOT] = "FS_COMBAT";
	m_szGrp[TLK_HELLO] = NULL;

	m_szGrp[TLK_PLHURT1] = NULL;
	m_szGrp[TLK_PLHURT2] = NULL;
	m_szGrp[TLK_PLHURT3] = NULL;

	m_szGrp[TLK_PHELLO] = NULL;	//"OT_PHELLO";		// UNDONE
	m_szGrp[TLK_PIDLE] = NULL;	//"OT_PIDLE";			// UNDONE
	m_szGrp[TLK_PQUESTION] = NULL;		// UNDONE

	m_szGrp[TLK_SMELL] = NULL;

	m_szGrp[TLK_WOUND] = NULL;
	m_szGrp[TLK_MORTAL] = NULL;

	// get voice for head - just one otis voice for now
	m_voicePitch = 100;
}

void CFemSci::DeclineFollowing()
{
	PlaySentence("FS_POK", 3, VOL_NORM, ATTN_NORM);
}

//=========================================================
// PainSound
//=========================================================
void CFemSci::PainSound()
{
	if (gpGlobals->time < m_painTime)
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT(0.5, 0.75);

	switch (RANDOM_LONG(0, 4))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 2: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 3: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 4: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}

//=========================================================
// DeathSound 
//=========================================================
void CFemSci::DeathSound()
{
	switch (RANDOM_LONG(0, 1))
	{
	case 0: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	case 1: EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "femsci/femsci_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch()); break;
	}
}