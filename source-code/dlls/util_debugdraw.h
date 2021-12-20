


// New file to contain script related to drawing debug lines in a more organized way.
// That is, specifing coords for a line to be drawn at (DebugLine_Setup, etc.) so that
// a line can be continually drawn at those coords from then on for easy viewing later.
// Has more control than the TE_LINE (or whatever it was) call that draws dotted lines
// for some fixed interval.  These can be immediately replaced when needed to avoid 
// irrelevant/outdated spam.




#ifndef UTIL_DEBUGDRAW_H
#define UTIL_DEBUGDRAW_H

#include "util_shared.h"  //includes util_printout.h
#include "cbase.h"

class CPathTrack;

#define DEBUG_LINES_MAX 16
#define DEBUG_PATHTRACK_DRAW_MAX 2

//For anywhere else that wants to know this?
#define DEBUG_LINE_WIDTH 8



class DebugLine{
public:
	BOOL canDraw;
	int r;
	int g;
	int b;
	int width;
	float v1x;
	float v1y;
	float v1z;
	float v2x;
	float v2y;
	float v2z;
	void checkDrawLine(void);
	void setup(BOOL argCanDraw, int argR, int argG, int argB, int argWidth, float argV1x, float argV1y, float argV1z, float argV2x, float argV2y, float argV2z);

};


class DebugDrawable{
public:
	DebugLine l1;
	DebugLine l2;
	void checkDrawLines(void);

};


class DebugDrawable_pathTrack{
public:
	/*
	DebugLine l_prev;
	DebugLine l_next;
	DebugLine l_origin;
	DebugLine l_alt;
	*/
	BOOL canDraw;
	EHANDLE	m_hSafeRef;
	CPathTrack* m_pPathTrackRef;
	void checkDrawLines(void);

};




extern int DebugLine_drawTime;


extern DebugDrawable aryDebugLines[];


extern void DebugLine_ClearLine(int argID);

extern void DebugLine_ClearAll(void);
extern void DebugLine_RenderAll(void);

extern void DebugLine_Setup(Vector vecStart, Vector vecEnd, float fraction, int successR=0, int successG=255, int successB=0, int failR=255, int failG=0, int failB=0);
extern void DebugLine_Setup(int argID, Vector vecStart, Vector vecEnd, float fraction, int successR=0, int successG=255, int successB=0, int failR=255, int failG=0, int failB=0);

extern void DebugLine_Setup(Vector vecStart, Vector vecEnd, int r, int g, int b);
extern void DebugLine_Setup(int argID, Vector vecStart, Vector vecEnd, int r, int g, int b);

extern void DebugLine_SetupPoint(Vector vecPoint, int r, int g, int b);
extern void DebugLine_SetupPoint(int argID, Vector vecPoint, int r, int g, int b);

extern void DebugLine_SetupPathTrack(CPathTrack* argPathTrack);
extern void DebugLine_SetupPathTrack(int argID, CPathTrack* argPathTrack);




extern void DebugLine_Color(int argID, int r, int g, int b);
extern void DebugLine_ColorSuccess(int argID);
extern void DebugLine_ColorFail(int argID);
extern void DebugLine_ColorBool(int argID, BOOL argPass);

extern void ShowNodeConnectionsFrame(int iNode);




// OLD DEBUG STUFF, phase out at some point
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
extern void DebugCall1(CBasePlayer* somePlayer);
extern void DebugCall2(CBasePlayer* somePlayer);
extern void DebugCall3(CBasePlayer* somePlayer);

extern void initOldDebugStuff(void);
extern void drawOldDebugStuff(void);


extern BOOL debugPoint1Given;
extern BOOL debugPoint2Given;
extern BOOL debugPoint3Given;
extern Vector debugPoint1;
extern Vector debugPoint2;
extern Vector debugPoint3;

extern Vector debugDrawVect;
extern Vector debugDrawVectB;
extern Vector debugDrawVect2;
extern Vector debugDrawVect3;
extern Vector debugDrawVect4;
extern Vector debugDrawVect5;

extern Vector debugDrawVectRecentGive1;
extern Vector debugDrawVectRecentGive2;

// these might be ok though
extern int nextSpecialNode;
extern float nextSpecialNodeAlternateTime;
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////



#endif //END OF UTIL_DEBUGDRAW_H















