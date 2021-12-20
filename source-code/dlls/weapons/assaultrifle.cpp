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
#include "soundent.h"
#include "gamerules.h"
#include "UserMessages.h"

LINK_ENTITY_TO_CLASS(weapon_assaultrifle, C556AR);


//=========================================================
//=========================================================
void C556AR::Spawn()
{
	pev->classname = MAKE_STRING("weapon_assaultrifle"); // hack to allow for old names
	Precache();
	SET_MODEL(ENT(pev), "models/w_556ar.mdl");
	m_iId = WEAPON_AR;

	m_iDefaultAmmo = AR_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}


void C556AR::Precache()
{
	PRECACHE_MODEL("models/v_556ar.mdl");
	PRECACHE_MODEL("models/w_556ar.mdl");
	PRECACHE_MODEL("models/p_556ar.mdl");

	m_iShell = PRECACHE_MODEL("models/shell.mdl");// brass shellTE_MODEL

	PRECACHE_MODEL("models/w_556arclip.mdl");
	PRECACHE_SOUND("items/9mmclip2.wav");

	PRECACHE_SOUND("weapons/556ar_boltpull.wav");
	PRECACHE_SOUND("weapons/556ar_clipmiss.wav");
	PRECACHE_SOUND("weapons/556ar_clipout.wav");
	PRECACHE_SOUND("weapons/556ar_cliptap.wav");
	PRECACHE_SOUND("weapons/556ar_draw.wav");
	PRECACHE_SOUND("weapons/556ar_shoot.wav");

	PRECACHE_SOUND("weapons/sniper_zoom.wav");

	PRECACHE_SOUND("weapons/357_cock1.wav");

	m_usAR1 = PRECACHE_EVENT(1, "events/ar1.sc");
}

int C556AR::GetItemInfo(ItemInfo* p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "556";
	p->iMaxAmmo1 = M249_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = AR_MAX_CLIP;
	p->iSlot = 2;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_AR;
	p->iWeight = AR_WEIGHT;

	return 1;
}

void C556AR::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "556", M249_MAX_CARRY))
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

int C556AR::AddToPlayer(CBasePlayer* pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev);
		WRITE_BYTE(m_iId);
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

BOOL C556AR::Deploy()
{

	return DefaultDeploy("models/v_556ar.mdl", "models/p_556ar.mdl", AR_DEPLOY, "ar");
}

void C556AR::Holster(int skiplocal)
{
	m_fInReload = false;// cancel any reload in progress.

	if (m_pPlayer->m_iFOV != 0)
	{
		SecondaryAttack();
	}

	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.25;
}

void C556AR::ARFire(float flSpread, float flCycleTime, BOOL fUseAutoAim, int iVolume)
{

	m_pPlayer->m_iWeaponVolume = iVolume;

	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	m_pPlayer->m_iWeaponFlash = NORMAL_GUN_FLASH;

	m_iClip--;


	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	Vector vecDir;


	// single player spread
	vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, Vector(flSpread, flSpread, flSpread), 8192, BULLET_PLAYER_556, 2, 0, m_pPlayer->pev, m_pPlayer->random_seed);
	

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usAR1, 0.0, (float*)&g_vecZero, (float*)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0);

	m_flNextPrimaryAttack = m_flNextSecondaryAttack = GetNextAttackDelay(flCycleTime);

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
}

void C556AR::PrimaryAttack()
{
	if (m_iBurstShots >= 3)
	{
		ARFire(0.01745, 0.55, FALSE, NORMAL_GUN_VOLUME);
		m_iBurstShots = 0;
	}
	else
	{
		ARFire(0.02500, 0.11, FALSE, NORMAL_GUN_VOLUME);
		m_iBurstShots++;
	}
}


void C556AR::SecondaryAttack()
{
	EMIT_SOUND_DYN(m_pPlayer->edict(), CHAN_ITEM, "weapons/sniper_zoom.wav", VOL_NORM, ATTN_NORM, 0, 94);

	if (m_pPlayer->m_iFOV == 0)
	{
		m_pPlayer->m_iFOV = 20;
		SendWeaponAnim(AR_SCOPE);
	}
	else
	{
		m_pPlayer->m_iFOV = 0;
		SendWeaponAnim(AR_DEPLOY);
	}

	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

void C556AR::Reload()
{
	if (m_pPlayer->m_iFOV != 0)
		SecondaryAttack();

	if (m_iClip < 1)
		DefaultReload(AR_MAX_CLIP, AR_RELOAD_EMPTY, 1.5);
	else
		DefaultReload(AR_MAX_CLIP, AR_RELOAD, 1.5);
}


void C556AR::WeaponIdle()
{
	ResetEmptySound();

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		iAnim = AR_IDLE1;
		break;
	case 1:
		iAnim = AR_IDLEFIDGET;
		break;

	default:
	case 2:
		iAnim = AR_LONGIDLE;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

class C556ARclip : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache();
		SET_MODEL(ENT(pev), "models/w_556arclip.mdl");
		CBasePlayerAmmo::Spawn();
	}
	void Precache() override
	{
		PRECACHE_MODEL("models/w_556arclip.mdl");
		PRECACHE_SOUND("items/9mmclip2.wav");
	}
	BOOL AddAmmo(CBaseEntity* pOther) override
	{
		int bResult = (pOther->GiveAmmo(AMMO_ARCLIP_GIVE, "556", M249_MAX_CARRY) != -1);
		if (bResult)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip2.wav", 1, ATTN_NORM);
		}
		return bResult;
	}
};
LINK_ENTITY_TO_CLASS(ammo_556AR, C556ARclip);

















