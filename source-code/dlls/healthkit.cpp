/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"
#include "UserMessages.h"

#define SF_OFF (1024)
#define SF_FAST (2048)

class CHealthKit : public CItem
{
	void Spawn() override;
	void Precache() override;
	BOOL MyTouch( CBasePlayer *pPlayer ) override;

/*
	int		Save( CSave &save ) override; 
	int		Restore( CRestore &restore ) override;
	
	static	TYPEDESCRIPTION m_SaveData[];
*/

};


LINK_ENTITY_TO_CLASS( item_healthkit, CHealthKit );
LINK_ENTITY_TO_CLASS( item_bandaid, CHealthKit );
LINK_ENTITY_TO_CLASS( item_bigaid, CHealthKit );

/*
TYPEDESCRIPTION	CHealthKit::m_SaveData[] = 
{

};


IMPLEMENT_SAVERESTORE( CHealthKit, CItem);
*/

void CHealthKit :: Spawn()
{
	Precache( );
	SET_MODEL(ENT(pev), "models/w_medkit.mdl");

	CItem::Spawn();
}

void CHealthKit::Precache()
{
	PRECACHE_MODEL("models/w_medkit.mdl");
	PRECACHE_SOUND("items/smallmedkit1.wav");
}

BOOL CHealthKit::MyTouch( CBasePlayer *pPlayer )
{
	float HP;
	HP = gSkillData.healthkitCapacity;
	if (pev->body == 2)
	{
		HP = gSkillData.healthkitCapacity - 5;
	}
	if (pev->body == 3)
	{
		HP = gSkillData.healthkitCapacity + 10;
	}

	if ( pPlayer->pev->deadflag != DEAD_NO )
	{
		return FALSE;
	}

	if ( pPlayer->TakeHealth(HP, DMG_GENERIC ) )
	{
		
		if (pev->body == 2)
		{
			MAKE_STRING("item_bandaid");
			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING("item_bandaid");
			MESSAGE_END();
		}
		else if (pev->body == 3)
		{
			MAKE_STRING("item_bigaid");
			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING("item_bigaid");
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN(MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING(STRING(pev->classname));
			MESSAGE_END();
		}

		EMIT_SOUND(ENT(pPlayer->pev), CHAN_ITEM, "items/smallmedkit1.wav", 1, ATTN_NORM);

		if ( g_pGameRules->ItemShouldRespawn( this ) )
		{
			Respawn();
		}
		else
		{
			UTIL_Remove(this);	
		}

		return TRUE;
	}

	return FALSE;
}



//-------------------------------------------------------------
// Wall mounted health kit
//-------------------------------------------------------------
class CWallHealth : public CBaseToggle
{
public:
	void Spawn( ) override;
	void Precache() override;
	void EXPORT Off();
	void EXPORT Recharge();
	void KeyValue( KeyValueData *pkvd ) override;
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
	int	ObjectCaps() override { return (CBaseToggle :: ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }
	int		Save( CSave &save ) override;
	int		Restore( CRestore &restore ) override;
	void UseTargetsOnEmpty();
	static	TYPEDESCRIPTION m_SaveData[];

	float m_flNextCharge; 
	int		m_iReactivate ; // DeathMatch Delay until reactvated
	int		m_iJuice;
	int		m_iOn;			// 0 = off, 1 = startup, 2 = going
	float   m_flSoundTime;
	bool bOnce;
};

TYPEDESCRIPTION CWallHealth::m_SaveData[] =
{
	DEFINE_FIELD( CWallHealth, m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( CWallHealth, m_iReactivate, FIELD_INTEGER),
	DEFINE_FIELD( CWallHealth, m_iJuice, FIELD_INTEGER),
	DEFINE_FIELD( CWallHealth, m_iOn, FIELD_INTEGER),
	DEFINE_FIELD( CWallHealth, m_flSoundTime, FIELD_TIME),
};

IMPLEMENT_SAVERESTORE( CWallHealth, CBaseEntity );

LINK_ENTITY_TO_CLASS(func_healthcharger, CWallHealth);


void CWallHealth::KeyValue( KeyValueData *pkvd )
{
	if (	FStrEq(pkvd->szKeyName, "style") ||
				FStrEq(pkvd->szKeyName, "height") ||
				FStrEq(pkvd->szKeyName, "value1") ||
				FStrEq(pkvd->szKeyName, "value2") ||
				FStrEq(pkvd->szKeyName, "value3"))
	{
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "dmdelay"))
	{
		m_iReactivate = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void CWallHealth::Spawn()
{
	Precache( );

	pev->solid		= SOLID_BSP;
	pev->movetype	= MOVETYPE_PUSH;

	UTIL_SetOrigin(pev, pev->origin);		// set size and link into world
	UTIL_SetSize(pev, pev->mins, pev->maxs);
	SET_MODEL(ENT(pev), STRING(pev->model) );
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->frame = 0;		
	bOnce = true;

}

void CWallHealth::Precache()
{
	PRECACHE_SOUND("items/medshot4.wav");
	PRECACHE_SOUND("items/medshotno1.wav");
	PRECACHE_SOUND("items/medcharge4.wav");
}


void CWallHealth::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ 
	// Make sure that we have a caller
	if (!pActivator)
		return;
	// if it's not a player, ignore
	if ( !pActivator->IsPlayer() )
		return;

	// if there is no juice left, turn it off
	if (m_iJuice <= 0)
	{
		pev->frame = 1;
		Off();
		
	}

	// if the player doesn't have the suit, or there is no juice left, or we start off, make the deny noise
	if ((m_iJuice <= 0) || (!(pActivator->pev->weapons & (1<<WEAPON_SUIT))) || pev->spawnflags & SF_OFF)
	{
		if (m_flSoundTime <= gpGlobals->time)
		{
			m_flSoundTime = gpGlobals->time + 0.62;
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshotno1.wav", 1.0, ATTN_NORM );
		}
		return;
	}

	pev->nextthink = pev->ltime + 0.25;
	SetThink(&CWallHealth::Off);

	// Time to recharge yet?

	if (m_flNextCharge >= gpGlobals->time)
		return;

	// Play the on sound or the looping charging sound
	if (!m_iOn)
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
		m_flSoundTime = 0.56 + gpGlobals->time;
	}
	if ((m_iOn == 1) && (m_flSoundTime <= gpGlobals->time))
	{
		m_iOn++;
		EMIT_SOUND(ENT(pev), CHAN_STATIC, "items/medcharge4.wav", 1.0, ATTN_NORM );
	}


	// charge the player
	if (pev->spawnflags & SF_FAST)
	{
		if (pActivator->TakeHealth(2, DMG_GENERIC))
		{
			m_iJuice -= 2;
		}
	}
	else if ( pActivator->TakeHealth( 1, DMG_GENERIC ) )
	{
		m_iJuice--;
	}
	

	// govern the rate of charge
	m_flNextCharge = gpGlobals->time + 0.1;
}

void CWallHealth::UseTargetsOnEmpty()
{
	SUB_UseTargets(this, USE_TOGGLE, 0);
}

void CWallHealth::Recharge()
{
		EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/medshot4.wav", 1.0, ATTN_NORM );
	m_iJuice = gSkillData.healthchargerCapacity;
	pev->frame = 0;			
	SetThink( &CWallHealth::SUB_DoNothing );
}

void CWallHealth::Off()
{
	// Stop looping sound.
	if (m_iOn > 1)
		STOP_SOUND( ENT(pev), CHAN_STATIC, "items/medcharge4.wav" );

	m_iOn = 0;

	if ((!m_iJuice) &&  ( ( m_iReactivate = g_pGameRules->FlHealthChargerRechargeTime() ) > 0) )
	{
		pev->nextthink = pev->ltime + m_iReactivate;
		SetThink(&CWallHealth::Recharge);
	}
	else
		SetThink( &CWallHealth::SUB_DoNothing );
	if (bOnce == true && m_iJuice <= 0)
	{
		UseTargetsOnEmpty();
		bOnce = false;
	}
}
