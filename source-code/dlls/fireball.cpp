/*
* 
* 
here we have the code for unique projectiles alien grunt fires (just the fireball for now)
"plasma" and toxic projecile are not present since they're actually shocktrooper projectiles and they already exist

* -----------------------------------------------------------------------------------------------------------------
* Half-Life Extended code by tear (blsha), Copyright 2021. Feel free to use/modify this as long as credit provided
* -----------------------------------------------------------------------------------------------------------------

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"
#include "UserMessages.h"

//=========================================================
// Fireball
//=========================================================

LINK_ENTITY_TO_CLASS(fireball, CFireball);

//=========================================================
//=========================================================
CFireball* CFireball::CreateFireball(Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner, CRpg* pLauncher)
{
	CFireball* pFireball = GetClassPtr((CFireball*)NULL);

	UTIL_SetOrigin(pFireball->pev, vecOrigin);
	pFireball->pev->angles = vecAngles;
	pFireball->Spawn();
	pFireball->SetTouch(&CFireball::FireballTouch);
	//pFireball->m_pFireLauncher = pLauncher;// remember what RPG fired me.     <- unused lol
	//pLauncher->m_cActiveRockets++;// register this missile as active for the launcher
	pFireball->pev->owner = pOwner->edict();

	return pFireball;
}

//=========================================================
//=========================================================
void CFireball::Spawn()
{
	Precache();
	// motor
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;

	SET_MODEL(ENT(pev), "sprites/glow_red.spr");
	UTIL_SetSize(pev, Vector(0, 0, 0), Vector(0, 0, 0));
	UTIL_SetOrigin(pev, pev->origin);

	pev->classname = MAKE_STRING("fireball");

	SetThink(&CFireball::IgniteThink);
	SetTouch(&CFireball::ExplodeTouch);

	pev->angles.x -= 30;
	UTIL_MakeVectors(pev->angles);
	pev->angles.x = -(pev->angles.x + 30);

	pev->velocity = gpGlobals->v_forward * 250;
	pev->gravity = 0.5;

	pev->nextthink = gpGlobals->time + 0.4;

	pev->dmg = 25;

	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
}

//=========================================================
//=========================================================
void CFireball::FireballTouch(CBaseEntity* pOther)
{
	if (m_pFireLauncher)
	{
		// my launcher is still around, tell it I'm dead.
		//static_cast<CRpg*>(static_cast<CBaseEntity*>(m_pFireLauncher))->m_cActiveRockets--;
	}

	STOP_SOUND(edict(), CHAN_VOICE, "agrunt/inferno_travel.wav");
	EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "agrunt/inferno_impact.wav", 1, 0.5, 0, 100);
	ExplodeTouch(pOther);
}

//=========================================================
//=========================================================
void CFireball::Precache()
{
	PRECACHE_MODEL("sprites/glow_red.spr");
	m_iTrail = PRECACHE_MODEL("sprites/smoke.spr");
	PRECACHE_SOUND("agrunt/inferno_travel.wav");
	PRECACHE_SOUND("agrunt/inferno_impact.wav");
}


void CFireball::IgniteThink()
{
	// pev->movetype = MOVETYPE_TOSS;

	pev->movetype = MOVETYPE_FLY;
	//pev->effects |= EF_LIGHT;

	// make rocket sound
	EMIT_SOUND(ENT(pev), CHAN_BODY, "agrunt/inferno_travel.wav", 1, 0.5);

	 //rocket trail
	MESSAGE_BEGIN(MSG_BROADCAST, SVC_TEMPENTITY);

	WRITE_BYTE(TE_BEAMFOLLOW);
	WRITE_SHORT(entindex());	// entity
	WRITE_SHORT(m_iTrail);	// model
	WRITE_BYTE(40); // life
	WRITE_BYTE(5);  // width
	WRITE_BYTE(224);   // r, g, b
	WRITE_BYTE(75);   // r, g, b
	WRITE_BYTE(75);   // r, g, b
	WRITE_BYTE(190);	// brightness

	MESSAGE_END();  // move PHS/PVS data sending into here (SEND_ALL, SEND_PVS, SEND_PHS)

	m_flIgniteTime = gpGlobals->time;

	// set to follow laser spot
	SetThink(&CFireball::FollowThink);
	pev->nextthink = gpGlobals->time + 0.1;
}


void CFireball::FollowThink()
{
	CBaseEntity* pOther = NULL;
	Vector vecTarget;
	Vector vecDir;
	float flDist, flMax, flDot;
	TraceResult tr;

	UTIL_MakeAimVectors(pev->angles);

	vecTarget = gpGlobals->v_forward;
	flMax = 4096;

	// Examine all entities within a reasonable radius
	while ((pOther = UTIL_FindEntityByClassname(pOther, "player")) != NULL)
	{
		UTIL_TraceLine(pev->origin, pOther->pev->origin, dont_ignore_monsters, ENT(pev), &tr);
		// ALERT( at_console, "%f\n", tr.flFraction );
		if (tr.flFraction >= 0.90)
		{
			vecDir = pOther->pev->origin - pev->origin;
			flDist = vecDir.Length();
			vecDir = vecDir.Normalize();
			flDot = DotProduct(gpGlobals->v_forward, vecDir);
			if ((flDot > 0) && (flDist * (1 - flDot) < flMax))
			{
				flMax = flDist * (1 - flDot);
				vecTarget = vecDir;
			}
		}
	}

	pev->angles = UTIL_VecToAngles(vecTarget);

	// this acceleration and turning math is totally wrong, but it seems to respond well so don't change it.
	float flSpeed = pev->velocity.Length();
	if (gpGlobals->time - m_flIgniteTime < 1.0)
	{
		pev->velocity = pev->velocity * 0.2 + vecTarget * (flSpeed * 0.7 + 200);
		if (pev->waterlevel == 3)
		{
			// go slow underwater
			if (pev->velocity.Length() > 300)
			{
				pev->velocity = pev->velocity.Normalize() * 300;
			}
			UTIL_BubbleTrail(pev->origin - pev->velocity * 0.1, pev->origin, 4);
		}
		else
		{
			if (pev->velocity.Length() > 2000)
			{
				pev->velocity = pev->velocity.Normalize() * 2000;
			}
		}
	}
	else
	{
		if (pev->effects & EF_LIGHT)
		{
			pev->effects = 0;
			STOP_SOUND(ENT(pev), CHAN_BODY, "weapons/rocket1.wav");
		}
		pev->velocity = pev->velocity * 0.2 + vecTarget * flSpeed * 0.798;
		if (pev->waterlevel == 0 && pev->velocity.Length() < 1500)
		{
			Detonate();
		}
	}
	// ALERT( at_console, "%.0f\n", flSpeed );

	pev->nextthink = gpGlobals->time + 0.1;
}
