/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
//
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_efx.h"

#include "studio.h"
#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

extern CGameStudioModelRenderer g_StudioRenderer;

#include "vgui_TeamFortressViewport.h"
#include "vgui_ScorePanel.h"



#include "particleman.h"
extern IParticleMan* g_pParticleMan;

extern int giTeamplay;

#define MAX_CLIENTS 32

extern void EV_HLDM_WaterSplash(float x, float y, float z);
extern void EV_HLDM_Particles(vec_t Pos_X, vec_t Pos_Y, vec_t Pos_Z, float PosNorm_X, float PosNorm_Y, float PosNorm_Z, int DoPuff, int Material);

int CHud::MsgFunc_WaterSplash(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	float X, Y, Z;
	X = READ_COORD();
	Y = READ_COORD();
	Z = READ_COORD();

	EV_HLDM_WaterSplash(X, Y, Z);
	return 1;


}


extern void EV_HLDM_DropMag(float x, float y, float z, int body);

int CHud::MsgFunc_DropMag(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	float X, Y, Z;
	int body;
	X = READ_COORD();
	Y = READ_COORD();
	Z = READ_COORD();
	body = READ_BYTE();

	EV_HLDM_DropMag(X, Y, Z, body);
	return 1;
}

int CHud::MsgFunc_Impact(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	
	int MatType = READ_SHORT();
	
	int DoPuffSpr = READ_BYTE();
	
	vec_t Pos_X, Pos_Y, Pos_Z;
	
	float PosNorm_X, PosNorm_Y, PosNorm_Z;

	Pos_X = READ_COORD();
	
	Pos_Y = READ_COORD();
	
	Pos_Z = READ_COORD();
	
	PosNorm_X = READ_COORD();
	
	PosNorm_Y = READ_COORD();
	
	PosNorm_Z = READ_COORD();
	
	EV_HLDM_Particles(Pos_X, Pos_Y, Pos_Z, PosNorm_X, PosNorm_Y, PosNorm_Z, DoPuffSpr, MatType);
	
	return 1;

}


#if !defined( _TFC )
extern BEAM* pBeam;
extern BEAM* pBeam2;
extern TEMPENTITY* pFlare;	// Vit_amiN
#endif 

#if defined( _TFC )
void ClearEventList();
#endif

/// USER-DEFINED SERVER MESSAGE HANDLERS




int CHud::MsgFunc_ResetHUD(const char* pszName, int iSize, void* pbuf)
{
	ASSERT(iSize == 0);

	// clear all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->Reset();
		pList = pList->pNext;
	}

	// reset sensitivity
	m_flMouseSensitivity = 0;

	// reset concussion effect
	m_iConcussionEffect = 0;

	return 1;
}

void CAM_ToFirstPerson();

void CHud::MsgFunc_ViewMode(const char* pszName, int iSize, void* pbuf)
{
	CAM_ToFirstPerson();
}

void CHud::MsgFunc_InitHUD(const char* pszName, int iSize, void* pbuf)
{
	m_iSkyMode = SKY_OFF; //LRC

	// prepare all hud data
	HUDLIST* pList = m_pHudList;

	while (pList)
	{
		if (pList->p)
			pList->p->InitHUDData();
		pList = pList->pNext;
	}

#if defined( _TFC )
	ClearEventList();

	// catch up on any building events that are going on
	gEngfuncs.pfnServerCmd("sendevents");
#endif

	if (g_pParticleMan)
		g_pParticleMan->ResetParticles();

#if !defined( _TFC )
	//Probably not a good place to put this.
	pBeam = pBeam2 = NULL;
	pFlare = NULL;	// Vit_amiN: clear egon's beam flare
#endif
}


int CHud::MsgFunc_GameMode(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_Teamplay = giTeamplay = READ_BYTE();

	if (gViewPort && !gViewPort->m_pScoreBoard)
	{
		gViewPort->CreateScoreBoard();
		gViewPort->m_pScoreBoard->Initialize();

		if (!gHUD.m_iIntermission)
		{
			gViewPort->HideScoreBoard();
		}
	}

	return 1;
}


//LRC
void CHud::MsgFunc_SetSky(const char* pszName, int iSize, void* pbuf)
{
	//	CONPRINT("MSG:SetSky");
	BEGIN_READ(pbuf, iSize);

	m_iSkyMode = READ_BYTE();
	m_vecSkyPos.x = READ_COORD();
	m_vecSkyPos.y = READ_COORD();
	m_vecSkyPos.z = READ_COORD();
}

int CHud::MsgFunc_Damage(const char* pszName, int iSize, void* pbuf)
{
	int		armor, blood;
	Vector	from;
	int		i;
	float	count;

	BEGIN_READ(pbuf, iSize);
	armor = READ_BYTE();
	blood = READ_BYTE();

	for (i = 0; i < 3; i++)
		from[i] = READ_COORD();

	count = (blood * 0.5) + (armor * 0.5);

	if (count < 10)
		count = 10;

	// TODO: kick viewangles,  show damage visually

	return 1;
}

int CHud::MsgFunc_Concuss(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	m_iConcussionEffect = READ_BYTE();
	if (m_iConcussionEffect)
	{
		this->m_StatusIcons.EnableIcon("dmg_concuss", giR, giG, giB);
	}
	else
		this->m_StatusIcons.DisableIcon("dmg_concuss");
	return 1;
}



void CHud::MsgFunc_SetSkin(const char* pszName, int iSize, void* pbuf)
{

	BEGIN_READ(pbuf, iSize);//begin reading message
	m_iSkin = READ_BYTE();//set iSkin value to the one we got from the message	
	cl_entity_s* view = gEngfuncs.GetViewModel();//get our gun's "name"
	view->curstate.skin = m_iSkin;//change skin
}

//add by bacontsu
int CHud::MsgFunc_WpnSkn(const char* pszName, int iSize, void* pbuf)
{
	BEGIN_READ(pbuf, iSize);
	g_iViewmodelSkin = READ_SHORT();

	return 1;
}
