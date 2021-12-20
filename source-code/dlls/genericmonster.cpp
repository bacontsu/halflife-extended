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

// For holograms, make them not solid so the player can walk through them
#define	SF_GENERICMONSTER_NOTSOLID					4 
#define	SF_HEAD_CONTROLLER						8 
#define SF_CAP_DOORS 1024
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
		CBaseMonster::HandleAnimEvent( pEvent );
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

	if ( pev->spawnflags & SF_GENERICMONSTER_NOTSOLID )
	{
		pev->solid = SOLID_NOT;
		pev->takedamage = DAMAGE_NO;
	}
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

//none, we're stoopid