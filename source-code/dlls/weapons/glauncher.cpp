/*
  =====================================================
                Half-Life: Extended

              --- Grenade launcher ---

  by tear
  =====================================================
*/
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

LINK_ENTITY_TO_CLASS(weapon_grenadelauncher, CGLauncher);
LINK_ENTITY_TO_CLASS(weapon_m79, CGLauncher);

void CGLauncher::Precache()
{
	PRECACHE_MODEL("models/v_glauncher.mdl");
	PRECACHE_MODEL("models/w_glauncher.mdl");
	PRECACHE_MODEL("models/p_glauncher.mdl");

//	m_iShell = PRECACHE_MODEL("models/saw_shell.mdl");
//	m_iLink = PRECACHE_MODEL("models/saw_link.mdl");
	m_iSmoke = PRECACHE_MODEL("sprites/wep_smoke_01.spr");
	m_iFire = PRECACHE_MODEL("sprites/xfire.spr");

	PRECACHE_SOUND("weapons/m79_close.wav");
	PRECACHE_SOUND("weapons/m79_draw.wav");
	PRECACHE_SOUND("weapons/gl_fire.wav");
	PRECACHE_SOUND("weapons/bounce.wav");

	m_usFireGLauncher = PRECACHE_EVENT(1, "events/m79.sc");
}

void CGLauncher::Spawn()
{
	pev->classname = MAKE_STRING("weapon_m79");

	Precache();

	m_iId = WEAPON_M79;

 SET_MODEL(edict(), "models/w_glauncher.mdl");

	m_iDefaultAmmo = M79_DEFAULT_GIVE;

	FallInit(); // get ready to fall down.
}

BOOL CGLauncher::AddToPlayer(CBasePlayer* pPlayer)
{
	if (CBasePlayerWeapon::AddToPlayer(pPlayer))
	{
		MESSAGE_BEGIN(MSG_ONE, gmsgWeapPickup, nullptr, pPlayer->edict());
		WRITE_BYTE(m_iId);
		MESSAGE_END();

		return true;
	}

	return false;
}

void CGLauncher::Reload()
{
	int iResult;

	if (m_iClip <= 0)
		iResult = DefaultReload(M79_MAX_CLIP, GL_RELOAD_EMPTY, 2.6);
	else
		iResult = DefaultReload(M79_MAX_CLIP, GL_RELOAD, 2);

	if (iResult)
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15);
	}
}

BOOL CGLauncher::Deploy()
{
	return DefaultDeploy("models/v_glauncher.mdl", "models/p_glauncher.mdl", GL_DRAW, "m79");
}

void CGLauncher::Holster(int skiplocal)
{
	SetThink(nullptr);
	// why does this exist?
}

void CGLauncher::WeaponIdle()
{

	//Update auto-aim
	// don't, m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);
	
	ResetEmptySound();

	m_pPlayer->GetAutoaimVector(AUTOAIM_5DEGREES);

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	switch (RANDOM_LONG(0, 2))
	{
	case 0:
		iAnim = GL_IDLE2;
		break;
	case 1:
		iAnim = GL_IDLEFIDGET;
		break;

	default:
	case 2:
		iAnim = GL_IDLE1;
		break;
	}

	SendWeaponAnim(iAnim);

	m_flTimeWeaponIdle = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 10, 15); // how long till we do this again.
}

void CGLauncher::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound();
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if(m_iClip <= 0)
	{
		PlayEmptySound();
		m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 2;
		return;
	}


	m_pPlayer->m_iWeaponVolume = NORMAL_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_pPlayer->m_iExtraSoundTypes = bits_SOUND_DANGER;
	m_pPlayer->m_flStopExtraSoundTime = UTIL_WeaponTimeBase() + 0.2;

	// player "shoot" animation
	m_pPlayer->SetAnimation(PLAYER_ATTACK1);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

	// we don't add in player velocity anymore.
	float sec = 1;
	CGrenade::ShootBouncy(m_pPlayer->pev,
		m_pPlayer->pev->origin + m_pPlayer->pev->view_ofs + gpGlobals->v_forward * 16,
		gpGlobals->v_forward * 700, sec);

	m_iClip--;

	int flags;
#if defined( CLIENT_WEAPONS )
	flags = UTIL_DefaultPlaybackFlags();
#else
	flags = 0;
#endif

	PLAYBACK_EVENT(flags, m_pPlayer->edict(), m_usFireGLauncher);

	m_flNextPrimaryAttack = GetNextAttackDelay(0.75);
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 5;// idle pretty soon after shooting.

	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		// HEV suit - indicate out of ammo condition
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
}

void CGLauncher::SecondaryAttack()
{
	// empty
}

int CGLauncher::GetItemInfo(ItemInfo* p)
{
	p->pszAmmo1 = "ARgrenades";
	p->iMaxAmmo1 = M79_MAX_CARRY;
	p->pszName = STRING(pev->classname);
	p->pszAmmo2 = nullptr;
	p->iMaxAmmo2 = WEAPON_NOCLIP;
	p->iMaxClip = M79_MAX_CLIP;
	p->iSlot = 5;
	p->iPosition = 3;
	p->iFlags = 0;
	p->iId = m_iId = WEAPON_M79;
	p->iWeight = M79_WEIGHT;

	return true;
}

void CGLauncher::IncrementAmmo(CBasePlayer* pPlayer)
{
	if (pPlayer->GiveAmmo(1, "556", M79_MAX_CARRY))
	{
		EMIT_SOUND(pPlayer->edict(), CHAN_STATIC, "ctf/pow_backpack.wav", 0.5, ATTN_NORM);
	}
}

