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
* -----------------------------------------
* Half-Life:Extended code by trvps (blsha)
* -----------------------------------------
****/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

#ifndef CLIENT_DLL

//========================================================
// Chumtoad Weapon
//========================================================
#endif
LINK_ENTITY_TO_CLASS(weapon_chummy, CChub)

void CChub::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING("weapon_chummy");
	
	m_iId = WEAPON_CHUMMY;
	SET_MODEL(ENT(pev), "models/w_chummynest.mdl");


	m_iDefaultAmmo = CHUMMY_DEFAULT_GIVE;
	FallInit();
}

void CChub::Precache()
{
	// Precache models
	PRECACHE_MODEL("models/v_chub.mdl");
	PRECACHE_MODEL("models/w_chummynest.mdl");
	PRECACHE_MODEL("models/p_chub.mdl");
	PRECACHE_MODEL("models/chumtoad.mdl");

	// Precache sounds
	PRECACHE_SOUND("chumtoad/chub_draw.wav");
	PRECACHE_SOUND("chumtoad/cht_croak_short.wav");
	PRECACHE_SOUND("chumtoad/cht_croak_medium.wav"); 
	PRECACHE_SOUND("chumtoad/cht_croak_long.wav");
	m_usToadFire = PRECACHE_EVENT(1, "events/chumtoadfire.sc");
}

void CChub::SecondaryAttack()
{
	//amogus
}

void CChub::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		UTIL_MakeVectors(m_pPlayer->pev->v_angle);
		TraceResult tr;
		Vector trace_origin;

		// HACK HACK:  Ugly hacks to handle change in origin based on new physics code for players
		// Move origin up if crouched and start trace a bit outside of body ( 20 units instead of 16 )
		trace_origin = m_pPlayer->pev->origin;
		if (m_pPlayer->pev->flags & FL_DUCKING)
		{
			trace_origin = trace_origin - (VEC_HULL_MIN - VEC_DUCK_HULL_MIN);
		}

		// find place to toss monster
		UTIL_TraceLine(trace_origin + gpGlobals->v_forward * 20, trace_origin + gpGlobals->v_forward * 64, dont_ignore_monsters, NULL, &tr);

		int flags;
#ifdef CLIENT_WEAPONS
		flags = UTIL_DefaultPlaybackFlags();
#else
		flags = 0;
#endif

		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usToadFire, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, 0.0, 0.0, 0, 0, 0, 0);

		if (tr.fAllSolid == 0 && tr.fStartSolid == 0 && tr.flFraction > 0.25)
		{
			// player "shoot" animation
			m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
			CBaseEntity* pChummy = CBaseEntity::Create("monster_chumtoad", tr.vecEndPos, m_pPlayer->pev->angles, NULL);
			pChummy->pev->velocity = gpGlobals->v_forward * 200 + m_pPlayer->pev->velocity;
#endif

			// play hunt sound
			float flRndSound = RANDOM_FLOAT(0, 1);

			if (flRndSound <= 0.5)
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad/cht_croak_medium.wav", 1, ATTN_NORM, 0, 105);
			else
				EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad/cht_croak_short.wav", 1, ATTN_NORM, 0, 105);

			m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

			m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

			m_fJustThrown = 1;

			m_flNextPrimaryAttack = GetNextAttackDelay(0.3);
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.0;
		}
	}
}

BOOL CChub::Deploy()
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT(0, 1);

	if (flRndSound <= 0.5)
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad/chub_draw.wav", 1, ATTN_NORM, 0, 100);
	else
		EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "chumtoad/cht_croak_short.wav", 1, ATTN_NORM, 0, 100);

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	const BOOL result = DefaultDeploy("models/v_chub.mdl", "models/p_chub.mdl", VCHUB_UP, "chumtoad");

	if (result)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 1.7;
	}

	return result;
}

void CChub::Holster(int skiplocal)
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if (!m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType])
	{
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_CHUMMY);
		SetThink(&CChub::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	SendWeaponAnim(VCHUB_DOWN);
	EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM);
}

void CChub::WeaponIdle()
{
	//cycle between skins ~ bacontsu
	while (nextBlinkUpdate <= gpGlobals->time)
	{

		switch (mBody)
		{
		case 0:
		{
			SetWeaponSkin(0);
			nextBlinkUpdate = gpGlobals->time + 0.1f;
			break;
		}
		case 1:
		{
			SetWeaponSkin(1);
			nextBlinkUpdate = gpGlobals->time + 0.1f;
			break;
		}
		case 2:
		{
			SetWeaponSkin(2);
			nextBlinkUpdate = gpGlobals->time + 0.1f;
			break;
		}
		case 3:
		{
			SetWeaponSkin(1);
			nextBlinkUpdate = gpGlobals->time + 0.1f;
			break;
		}
		case 4:
		{
			SetWeaponSkin(0);
			nextBlinkUpdate = gpGlobals->time + 0.1f;
			break;
		}

		default:
		{
			nextBlinkUpdate = gpGlobals->time + 5;
			mBody = 0;
			break;
		}

		}
		mBody++;
		//ALERT(at_console, "body type %d", mBody);

	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	if (pev->body == 0 && RANDOM_LONG(0, 45) == 0)
	{
		pev->body = 2;
	}
	if (pev->body != 0)
	{
		pev->body--;
	}

	if (m_fJustThrown)
	{
		m_fJustThrown = 0;

		if (!m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()])
		{
			RetireWeapon();
			return;
		}

		SendWeaponAnim(VCHUB_UP);
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);
	if (flRand <= 0.60)
	{
		iAnim = VCHUB_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.80)
	{
		iAnim = VCHUB_IDLELICK;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 16.0;
	}
	else
	{
		iAnim = VCHUB_IDLECROAK;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
	}
	SendWeaponAnim(iAnim);
}

int CChub::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "Toads";
	p->iMaxAmmo1 = CHUMMY_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 6;
	p->iPosition = 2;
	p->iId = m_iId = WEAPON_CHUMMY;
	p->iWeight = CHUMMY_WEIGHT;
	return TRUE;
}

/////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


