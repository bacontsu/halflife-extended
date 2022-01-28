//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"
#include "fog.h"
#include "rain.h"
#include "com_model.h"
#include "studio_util.h"

extern IParticleMan *g_pParticleMan;

// rain tutorial
void SetPoint(float x, float y, float z, float(*matrix)[4])
{
 
   Vector point, result;
 
   point[0] = x;
 
   point[1] = y;
 
   point[2] = z;
 
   VectorTransform(point, matrix, result);
 
   gEngfuncs.pTriAPI->Vertex3f(result[0], result[1], result[2]);
 
}



/* rain tutorial
=================================
DrawRain
Draws drips or snowflakes
=================================
*/
extern cl_drip FirstChainDrip;

extern rain_properties Rain;

void DrawRain(void)
{
    
    if (FirstChainDrip.p_Next == NULL)
        return; // no drips to draw
    
    HSPRITE hsprTexture;
    const model_s * pTexture;
    float visibleHeight = Rain.globalHeight - SNOWFADEDIST;
    

    if (Rain.weatherMode == 0)
        hsprTexture = LoadSprite("sprites/effects/hi_rain.spr"); // load rain sprite
    else
        hsprTexture = LoadSprite("sprites/effects/snowflake.spr"); // load snow sprite
    
    // usual triapi stuff
    pTexture = gEngfuncs.GetSpritePointer(hsprTexture);
    
    gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)pTexture, 0);
    
    gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
    
    gEngfuncs.pTriAPI->CullFace(TRI_NONE);
    
    // go through drips list
    cl_drip * Drip = FirstChainDrip.p_Next;
    
    cl_entity_t * player = gEngfuncs.GetLocalPlayer();
    
    if (Rain.weatherMode == 0)
    { // draw rain
            while (Drip != NULL)
             
            {
             
               cl_drip * nextdDrip = Drip->p_Next;
             
               Vector2D toPlayer;
             
               toPlayer.x = player->origin[0] - Drip->origin[0];
             
               toPlayer.y = player->origin[1] - Drip->origin[1];
             
               toPlayer = toPlayer.Normalize();
             

               toPlayer.x *= DRIP_SPRITE_HALFWIDTH;
             
               toPlayer.y *= DRIP_SPRITE_HALFWIDTH;

             
               float shiftX = (Drip->xDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;
               float shiftY = (Drip->yDelta / DRIPSPEED) * DRIP_SPRITE_HALFHEIGHT;
             
               // --- draw triangle --------------------------
               gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, Drip->alpha);
               gEngfuncs.pTriAPI->Begin(TRI_TRIANGLES);
             
               gEngfuncs.pTriAPI->TexCoord2f(0, 0);
             
               gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] - toPlayer.y - shiftX, Drip->origin[1] + toPlayer.x - shiftY,
                   Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT);
             
               gEngfuncs.pTriAPI->TexCoord2f(0.5, 1);
             
               gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] + shiftX, Drip->origin[1] + shiftY,
                   Drip->origin[2] - DRIP_SPRITE_HALFHEIGHT);
             
               gEngfuncs.pTriAPI->TexCoord2f(1, 0);
             
               gEngfuncs.pTriAPI->Vertex3f(Drip->origin[0] + toPlayer.y - shiftX, Drip->origin[1] - toPlayer.x - shiftY,
                   Drip->origin[2] + DRIP_SPRITE_HALFHEIGHT);

               gEngfuncs.pTriAPI->End();
             
               // --- draw triangle end ----------------------
               Drip = nextdDrip;
             
            }
        
    }
    else
    { // draw snow
      
       Vector normal;
      
       gEngfuncs.GetViewAngles((float*)normal);
      
       float matrix[3][4];
       AngleMatrix(normal, matrix);	// calc view matrix
      
       while (Drip != NULL)
         
       {
         
           cl_drip * nextdDrip = Drip->p_Next;
         
           matrix[0][3] = Drip->origin[0]; // write origin to matrix
         
           matrix[1][3] = Drip->origin[1];
         
           matrix[2][3] = Drip->origin[2];
         
           // apply start fading effect
           float alpha = (Drip->origin[2] <= visibleHeight) ? Drip->alpha :
           ((Rain.globalHeight - Drip->origin[2]) / (float)SNOWFADEDIST) * Drip->alpha;
         

           // --- draw quad --------------------------
           gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, alpha);
           gEngfuncs.pTriAPI->Begin(TRI_QUADS);
         
           gEngfuncs.pTriAPI->TexCoord2f(0, 0);
           SetPoint(0, SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);
         
           gEngfuncs.pTriAPI->TexCoord2f(0, 1);
           SetPoint(0, SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);
         
           gEngfuncs.pTriAPI->TexCoord2f(1, 1);
           SetPoint(0, -SNOW_SPRITE_HALFSIZE, -SNOW_SPRITE_HALFSIZE, matrix);
         
           gEngfuncs.pTriAPI->TexCoord2f(1, 0);
           SetPoint(0, -SNOW_SPRITE_HALFSIZE, SNOW_SPRITE_HALFSIZE, matrix);
         
           gEngfuncs.pTriAPI->End();
           // --- draw quad end ----------------------

           Drip = nextdDrip;
         
       }
      
    }
}

/* rain tutorial
=================================
DrawFXObjects
Draws water rings
=================================
*/
extern cl_rainfx FirstChainFX;

void DrawFXObjects(void)
{
  
    if (FirstChainFX.p_Next == NULL)
        return; // no objects to draw
  
    float curtime = gEngfuncs.GetClientTime();

    // usual triapi stuff
    HSPRITE hsprTexture;
  
    const model_s * pTexture;
  
    hsprTexture = LoadSprite("sprites/effects/waterring.spr"); // load water ring sprite
  
    pTexture = gEngfuncs.GetSpritePointer(hsprTexture);
  
    gEngfuncs.pTriAPI->SpriteTexture((struct model_s*)pTexture, 0);
  
    gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
  
    gEngfuncs.pTriAPI->CullFace(TRI_NONE);
  

    // go through objects list
    cl_rainfx * curFX = FirstChainFX.p_Next;
  
    while (curFX != NULL)    {
       
         cl_rainfx * nextFX = curFX->p_Next;
       
         // fadeout
         float alpha = ((curFX->birthTime + curFX->life - curtime) / curFX->life) * curFX->alpha;
       
         float size = (curtime - curFX->birthTime) * MAXRINGHALFSIZE;
       
         // --- draw quad --------------------------
         gEngfuncs.pTriAPI->Color4f(1.0, 1.0, 1.0, alpha);
         gEngfuncs.pTriAPI->Begin(TRI_QUADS);
       
         gEngfuncs.pTriAPI->TexCoord2f(0, 0);
         gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] - size, curFX->origin[1] - size, curFX->origin[2]);
       
         gEngfuncs.pTriAPI->TexCoord2f(0, 1);
         gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] - size, curFX->origin[1] + size, curFX->origin[2]);
       
         gEngfuncs.pTriAPI->TexCoord2f(1, 1);
         gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] + size, curFX->origin[1] + size, curFX->origin[2]);
       
         gEngfuncs.pTriAPI->TexCoord2f(1, 0);
         gEngfuncs.pTriAPI->Vertex3f(curFX->origin[0] + size, curFX->origin[1] - size, curFX->origin[2]);
       
         gEngfuncs.pTriAPI->End();
       
         // --- draw quad end ----------------------

         curFX = nextFX;
       
    }
  
}




/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
//	RecClDrawNormalTriangles();

	gHUD.m_Spectator.DrawOverview();

	gFog.HUD_DrawNormalTriangles();
}

#if defined( _TFC )
void RunEventList();
#endif

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
//	RecClDrawTransparentTriangles();

#if defined( _TFC )
	RunEventList();
#endif

	if ( g_pParticleMan )
		 g_pParticleMan->Update();
	gFog.HUD_DrawTransparentTriangles();

    ProcessFXObjects();

    ProcessRain();
    DrawRain();
    DrawFXObjects();

}
