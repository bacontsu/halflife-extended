
#include "extdll.h"
#include "util_debugdraw.h"
#include "util_shared.h"
#include "cbase.h"
#include "util.h"
#include "vector.h"
#include "util_printout.h"
#include "player.h"
#include "nodes.h"
#include "trains.h"


EASY_CVAR_EXTERN_DEBUGONLY(drawDebugBloodTrace)
EASY_CVAR_EXTERN_DEBUGONLY(drawNodeAlternateTime)
EASY_CVAR_EXTERN_DEBUGONLY(drawNodeSpecial)
EASY_CVAR_EXTERN_DEBUGONLY(drawNodeAll)
EASY_CVAR_EXTERN_DEBUGONLY(drawNodeConnections)
EASY_CVAR_EXTERN_DEBUGONLY(drawDebugCine)


// or "always the first time"
int DebugLine_drawTime = -1;

static int nextDebugLineID = 0;
static int nextDebugPathTrackDrawID = 0;

DebugDrawable aryDebugLines[DEBUG_LINES_MAX];
DebugDrawable_pathTrack aryDebugLines_pathTrack[DEBUG_PATHTRACK_DRAW_MAX];


void DebugLine::checkDrawLine(void){
	if(canDraw){
		UTIL_drawLineFrame( v1x, v1y, v1z, v2x, v2y, v2z, width, r, g, b);
		//UTIL_drawLineFrame( roundToNearest(v1x), roundToNearest(v1y), roundToNearest(v1z), roundToNearest(v2x), roundToNearest(v2y), roundToNearest(v2z), width, r, g, b);
	}
}
void DebugLine::setup(BOOL argCanDraw, int argR, int argG, int argB, int argWidth, float argV1x, float argV1y, float argV1z, float argV2x, float argV2y, float argV2z){
	canDraw = argCanDraw;
	r = argR;
	g = argG;
	b = argB;
	width = argWidth;
	v1x = argV1x;
	v1y = argV1y;
	v1z = argV1z;
	v2x = argV2x;
	v2y = argV2y;
	v2z = argV2z;
}

void DebugDrawable::checkDrawLines(void){
	l1.checkDrawLine();
	l2.checkDrawLine();
}

void DebugLine_ClearLine(int argID){
	aryDebugLines[argID].l1.canDraw = FALSE;
	aryDebugLines[argID].l2.canDraw = FALSE;
}

void DebugLine_ClearAll(){
	int i;
	nextDebugLineID = 0;
	nextDebugPathTrackDrawID = 0;
	for(i = 0; i < DEBUG_LINES_MAX; i++){
		aryDebugLines[i].l1.canDraw = FALSE;
		aryDebugLines[i].l2.canDraw = FALSE;
	}
	for(i = 0; i < DEBUG_PATHTRACK_DRAW_MAX; i++){
		aryDebugLines_pathTrack[i].canDraw = FALSE;
	}
}
void DebugLine_RenderAll(){
	int i = 0;
	//return;
	/*
	float fracto = fabs(cos(gpGlobals->time/3));
	easyForcePrintLine("WHAT? %.2f : %.2f", gpGlobals->time, fracto);
	DebugLine_Setup(2, Vector(66.78, 805.35, 36.36), Vector(66.78, 805.35, 36.36) + Vector(-40, 200, 40), fracto );
	
	*/

	if (CVAR_GET_FLOAT("developer") >= 1) {
		// proceed
	}else{
		// no!
		return;
	}


	
	for(i = 0; i < DEBUG_PATHTRACK_DRAW_MAX; i++){
		aryDebugLines_pathTrack[i].checkDrawLines();
	}
	for(i = 0; i < DEBUG_LINES_MAX; i++){
		aryDebugLines[i].checkDrawLines();
	}
}//END OF DebugLine_RenderAll



void DebugDrawable_pathTrack::checkDrawLines(){



	if(canDraw){
		if(m_hSafeRef == NULL){
			//the path referred to disappeared? STOP!
			canDraw = FALSE;
			m_pPathTrackRef = NULL;
			return;
		}
		if(m_pPathTrackRef->pev->spawnflags & SF_PATH_DISABLED){
			//color it red.
			UTIL_drawLineFrame( m_pPathTrackRef->pev->origin + Vector(0, 0, -30), m_pPathTrackRef->pev->origin + Vector(0, 0, 30), DEBUG_LINE_WIDTH, 255, 0, 0);
		}else{
			//color it green.
			UTIL_drawLineFrame( m_pPathTrackRef->pev->origin + Vector(0, 0, -30), m_pPathTrackRef->pev->origin + Vector(0, 0, 30), DEBUG_LINE_WIDTH, 0, 255, 0);
		}

		if(m_pPathTrackRef->m_pprevious != NULL){
			//line to previous is yellow.
			UTIL_drawLineFrame( m_pPathTrackRef->pev->origin, m_pPathTrackRef->m_pprevious->pev->origin, DEBUG_LINE_WIDTH, 255, 255, 0);
		}
		if(m_pPathTrackRef->m_pnext != NULL){
			//line to next is blue.
			UTIL_drawLineFrame( m_pPathTrackRef->pev->origin, m_pPathTrackRef->m_pnext->pev->origin, DEBUG_LINE_WIDTH, 0, 0, 255);
		}
		if(m_pPathTrackRef->m_paltpath != NULL){
			//line to alternate is white. Rarely exists most likely.
			UTIL_drawLineFrame( m_pPathTrackRef->pev->origin, m_pPathTrackRef->m_paltpath->pev->origin, DEBUG_LINE_WIDTH, 255, 255, 255);
		}
	}
}//END OF DebugDrawable_pathTrack's checkDrawLines



void DebugLine_Setup(Vector vecStart, Vector vecEnd, int r, int g, int b){
	DebugLine_Setup(-1, vecStart, vecEnd, r, g, b);
}
void DebugLine_Setup(int argID, Vector vecStart, Vector vecEnd, int r, int g, int b){

	if(argID == -1){
		if(nextDebugLineID >= DEBUG_LINES_MAX){
			//easyForcePrintLine("ERROR: Too many lines or invalid ID given! Tried to create debug line of ID %d, max is %d", argID, DEBUG_LINES_MAX);
			// start over at 0.  jeez.
			//return; //too many lines, don't.

			nextDebugLineID = 0;
		}
		argID = nextDebugLineID;
		nextDebugLineID++;
	}
	DebugDrawable& thisLine = aryDebugLines[argID];
	thisLine.l1.setup(TRUE, r, g, b, DEBUG_LINE_WIDTH, vecStart.x, vecStart.y, vecStart.z, vecEnd.x, vecEnd.y, vecEnd.z);
	
	thisLine.l2.canDraw = FALSE;  //only one line here.
}//END OF CreateDebugLine



void DebugLine_SetupPoint(Vector vecPoint, int r, int g, int b){
	DebugLine_SetupPoint(-1, vecPoint, r, g, b);
}
void DebugLine_SetupPoint(int argID, Vector vecPoint, int r, int g, int b){
	DebugLine_Setup(argID, vecPoint + Vector(0, 0, -30), vecPoint + Vector(0,0, 30), r, g, b);
}

void DebugLine_SetupPathTrack(CPathTrack* argPathTrack){
	DebugLine_SetupPathTrack(-1, argPathTrack);
}
void DebugLine_SetupPathTrack(int argID, CPathTrack* argPathTrack){

	if(argID == -1){
		if(nextDebugPathTrackDrawID >= DEBUG_PATHTRACK_DRAW_MAX){
			easyForcePrintLine("ERROR: Too many path_tracks or invalid ID given! Tried to create debug line of ID %d, max is %d", argID, DEBUG_PATHTRACK_DRAW_MAX);
			return; //too many lines, don't.
		}
		argID = nextDebugPathTrackDrawID;
		nextDebugPathTrackDrawID++;
	}
	DebugDrawable_pathTrack& thisDebugPathTrack = aryDebugLines_pathTrack[argID];
	thisDebugPathTrack.canDraw = TRUE;

	thisDebugPathTrack.m_hSafeRef = argPathTrack;
	//thisDebugPathTrack.m_hSafeRef.Set(argPathTrack->edict());
	thisDebugPathTrack.m_pPathTrackRef = argPathTrack;
}

void DebugLine_Setup(Vector vecStart, Vector vecEnd, float fraction, int successR, int successG, int successB, int failR, int failG, int failB){
	DebugLine_Setup(-1, vecStart, vecEnd, fraction, successR, successG, successB, failR, failG, failB);
}
//From here to there, draw a line to show that up to this distance has passed.
void DebugLine_Setup(int argID, Vector vecStart, Vector vecEnd, float fraction, int successR, int successG, int successB, int failR, int failG, int failB){
	
	if(argID == -1){
		if(nextDebugLineID + 1 >= DEBUG_LINES_MAX){
			easyForcePrintLine("ERROR: Too many lines! Tried to create debug line of ID %d, max is %d", argID, DEBUG_LINES_MAX);
			return; //too many lines, don't.
		}
		argID = nextDebugLineID;
		nextDebugLineID++;
	}

	fraction = UTIL_clamp(fraction, 0, 1); //just in case.

	Vector vecDeltaFract = (vecEnd - vecStart) * fraction;

	float dist = (vecEnd - vecStart).Length();

	Vector vecDelta = (vecEnd - vecStart);
	Vector vecDir = vecDelta.Normalize();
	/*
	Vector vecDeltaFract = vecDir * dist * fraction;
	*/
	//Vector vecNewEnd = vecStart + vecDeltaFract;
	Vector vecNewEnd = vecStart + vecDelta * fraction;

	Vector& vecAltStart = vecNewEnd;
	//Vector& vecAltEnd = vecEnd;
	//Vector vecAltEnd = vecAltStart + vecDir * dist * (1 - fraction);
	Vector vecAltEnd = vecAltStart + vecDelta * (1 - fraction);
	
	DebugDrawable& thisLine = aryDebugLines[argID];

	if(fraction <= 0){
		//all fail, just draw the 2nd line only.
		thisLine.l1.setup(TRUE, failR, failG, failB, DEBUG_LINE_WIDTH, vecAltStart.x, vecAltStart.y, vecAltStart.z, vecAltEnd.x, vecAltEnd.y, vecAltEnd.z);
		thisLine.l2.canDraw = FALSE;
	}else if(fraction >= 1){
		//all success.
		thisLine.l1.setup(TRUE, successR, successG, successB, DEBUG_LINE_WIDTH, vecStart.x, vecStart.y, vecStart.z, vecNewEnd.x, vecNewEnd.y, vecNewEnd.z);
		thisLine.l2.canDraw = FALSE;
	}else{
		//inbetween.
		thisLine.l1.setup(TRUE, successR, successG, successB, DEBUG_LINE_WIDTH, vecStart.x, vecStart.y, vecStart.z, vecNewEnd.x, vecNewEnd.y, vecNewEnd.z);
		thisLine.l2.setup(TRUE, failR, failG, failB, DEBUG_LINE_WIDTH, vecAltStart.x, vecAltStart.y, vecAltStart.z, vecAltEnd.x, vecAltEnd.y, vecAltEnd.z);
	}
}//END OF CreateDebugLine

void DebugLine_Color(int argID, int r, int g, int b){
	DebugDrawable& thisLine = aryDebugLines[argID];
	thisLine.l1.r = r;
	thisLine.l1.g = g;
	thisLine.l1.b = b;
	thisLine.l2.r = r;
	thisLine.l2.g = g;
	thisLine.l2.b = b;
}

void DebugLine_ColorSuccess(int argID){
	DebugLine_Color(argID, 0, 255, 0);
}
void DebugLine_ColorFail(int argID){
	DebugLine_Color(argID, 255, 0, 0);
}

//color a line green (success) or red (fail) based on the provided BOOL.
void DebugLine_ColorBool(int argID, BOOL argPass){
	if(argPass){
		DebugLine_ColorSuccess(argID);
	}else{
		DebugLine_ColorFail(argID);
	}
}






//MODDD - new.  uses calls to "UTIL_drawLineFrame" to draw a line this frame for easier updates.
// Was a new method for CGraph but moved here to be easier to copy/paste to other projects
void ShowNodeConnectionsFrame(int iNode) {

	int i = 0;
	Vector	vecSpot;
	CNode* pNode;
	CNode* pLinkNode;
	if (iNode < 0 || iNode >= WorldGraph.m_cNodes)
	{
		//out of bounds!
		return;
	}

	pNode = &WorldGraph.m_pNodes[iNode];

	for (i = 0; i < pNode->m_cNumLinks; i++)
	{
		pLinkNode = &WorldGraph.Node(WorldGraph.NodeLink(iNode, i).m_iDestNode);
		vecSpot = pLinkNode->m_vecOrigin;


		UTIL_drawLineFrame(WorldGraph.m_pNodes[iNode].m_vecOrigin.x,
			WorldGraph.m_pNodes[iNode].m_vecOrigin.y,
			WorldGraph.m_pNodes[iNode].m_vecOrigin.z + node_linktest_height,
			vecSpot.x,
			vecSpot.y,
			vecSpot.z + node_linktest_height, 7, 255, 0, 100);

	}
}//ShowNodeConnectionsFrame










// DEBUG STUFF.  A lot should be phased out at some point.
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////


BOOL debugPoint1Given;
BOOL debugPoint2Given;
BOOL debugPoint3Given;
Vector debugPoint1;
Vector debugPoint2;
Vector debugPoint3;

Vector debugDrawVect;
Vector debugDrawVectB;
Vector debugDrawVect2;
Vector debugDrawVect3;
Vector debugDrawVect4;
Vector debugDrawVect5;

Vector debugDrawVectRecentGive1;
Vector debugDrawVectRecentGive2;

// these might be ok though
int nextSpecialNode;
float nextSpecialNodeAlternateTime;
///////////////////////////////////////////////////////////////////////////



void DebugCall1(CBasePlayer* somePlayer) {
	debugPoint1 = somePlayer->pev->origin;
	debugPoint1.z += 2;
	debugPoint1Given = TRUE;
	debugPoint3Given = FALSE;
}
void DebugCall2(CBasePlayer* somePlayer) {
	debugPoint2 = somePlayer->pev->origin;
	debugPoint2.z += 2;
	debugPoint2Given = TRUE;
	debugPoint3Given = FALSE;
}
void DebugCall3(CBasePlayer* somePlayer) {
	//try a local move.

	debugPoint3Given = FALSE;


	float distReg;

	UTIL_MakeVectors(somePlayer->pev->v_angle + somePlayer->pev->punchangle);

	Vector vecStart = debugPoint1;
	Vector vecEnd = debugPoint2;

	//BOOL success = this->CheckLocalMove(vecStart, vecStart + gpGlobals->v_forward * dist, NULL, &distReg);
	BOOL success = (somePlayer->CheckLocalMove(vecStart, vecEnd, NULL, TRUE, &distReg) == LOCALMOVE_VALID);
	//UTIL_TraceLine(tempplayer->pev->origin + tempplayer->pev->view_ofs + gpGlobals->v_forward * 5,pMe->pev->origin + pMe->pev->view_ofs + gpGlobals->v_forward * 2048,dont_ignore_monsters, pMe->edict(), &tr );

	if (success) {
		easyForcePrintLine("SUCCESS!  CLEAR!");
		debugPoint3 = vecEnd;
		debugPoint3Given = TRUE;
	}
	else {

		float totalDist = (vecEnd - vecStart).Length();
		easyForcePrintLine("Stopped this far: %.2f OUT OF %.2f", distReg, totalDist);
		debugPoint3Given = TRUE;
		Vector tempDir = (vecEnd - vecStart).Normalize();

		//debugPoint3 = vecStart + (tempDir * (distReg ));
		//debugPoint3 = vecStart + (vecEnd - vecStart) * (distReg / totalDist);
		float fracto = distReg / totalDist;
		debugPoint3 = vecStart * (1 + -fracto) + vecEnd * fracto;

		//UTIL_printVector("vecStart", vecStart);
		//UTIL_printVector("tempDir", tempDir);
		//UTIL_printVector("debugPoint3", debugPoint3);
		//UTIL_printVector("vecEnd", vecEnd);
	}
}//END OF DebugCall3()




void initOldDebugStuff(void) {
	// called at game startup for the first time since opening the game, not new maps since then.

	nextSpecialNode = -1;
	nextSpecialNodeAlternateTime = -1;


	debugPoint1Given = FALSE;
	debugPoint2Given = FALSE;
	debugPoint3Given = FALSE;

}



//MODDD - NEW.  For organization.   Should be able to be drawn independently of any player.  Except the first.
// Because laziness. Most features tied to the player like that aren't used anymore though.
void drawOldDebugStuff(void) {
	float drawDebugBloodTrace_val = EASY_CVAR_GET_DEBUGONLY(drawDebugBloodTrace);
	float drawNodeAlternateTime_val = EASY_CVAR_GET_DEBUGONLY(drawNodeAlternateTime);
	float drawNodeSpecial_val = EASY_CVAR_GET_DEBUGONLY(drawNodeSpecial);
	float drawNodeAll_val = EASY_CVAR_GET_DEBUGONLY(drawNodeAll);
	float drawNodeConnections_val = EASY_CVAR_GET_DEBUGONLY(drawNodeConnections);
	float drawDebugCine_val = EASY_CVAR_GET_DEBUGONLY(drawDebugCine);



	edict_t* thePlayerEd = g_engfuncs.pfnPEntityOfEntIndex(1);
	CBasePlayer* tempplayer;

	if (!FNullEnt(thePlayerEd) && !thePlayerEd->free) {

		//entvars_t* someEntvarsTest = &thePlayerEd->v;
		//tempplayer = GetClassPtr((CBasePlayer*)someEntvarsTest);

		// how about this instead
		CBaseEntity* entTest = CBaseEntity::Instance(thePlayerEd);
		if (entTest != NULL) {
			tempplayer = static_cast<CBasePlayer*>(entTest);
		}

	}
	else {
		// ??????
		tempplayer = NULL;
	}


	if (debugPoint1Given) {
		UTIL_drawLineFrame(debugPoint1 - Vector(0, 0, 3), debugPoint1 + Vector(0, 0, 3), 9, 0, 255, 0);
	}
	if (debugPoint2Given) {
		UTIL_drawLineFrame(debugPoint2 - Vector(0, 0, 3), debugPoint2 + Vector(0, 0, 3), 9, 55, 225, 70);
	}
	if (debugPoint3Given) {
		UTIL_drawLineFrame(debugPoint1, debugPoint3, 15, 0, 255, 0);
		UTIL_drawLineFrame(debugPoint3, debugPoint2, 15, 255, 0, 0);
	}
	else if (debugPoint1Given && debugPoint2Given) {
		UTIL_drawLineFrame(debugPoint1, debugPoint2, 12, 255, 255, 255);
	}

	//MODDD
	if (drawDebugBloodTrace_val == 1) {
		UTIL_drawLineFrameBoxAround(debugDrawVect, 9, 30, 255, 0, 0);
		//UTIL_drawLineFrameBoxAround(debugDrawVectB, 9, 30, 255, 255, 255);

		UTIL_drawLineFrame(debugDrawVect2, debugDrawVect3, 9, 0, 255, 0);
		UTIL_drawLineFrame(debugDrawVect4, debugDrawVect5, 9, 0, 0, 255);

		UTIL_drawLineFrame(debugDrawVectRecentGive1, debugDrawVectRecentGive2, 9, 255, 255, 255);

	}


	int specialNode = -1;
	if (drawNodeAlternateTime_val > 0) {
		if (gpGlobals->time > 3 && gpGlobals->time > nextSpecialNodeAlternateTime) {
			//easyPrintLine("IM A DURR %d", nextSpecialNode);
			nextSpecialNode++;
			if (nextSpecialNode >= WorldGraph.m_cNodes) {
				nextSpecialNode = 0;
			}
			nextSpecialNodeAlternateTime = gpGlobals->time + drawNodeAlternateTime_val;
			easyForcePrintLine("SPECIAL NODE: %d", nextSpecialNode);
		}
		specialNode = nextSpecialNode;
	}
	else {
		nextSpecialNode = -1;
		if (drawNodeSpecial_val >= 0 && drawNodeSpecial_val < WorldGraph.m_cNodes) {
			specialNode = (int)drawNodeSpecial_val;
		}
	}


	if (drawNodeAll_val >= 1 || drawNodeConnections_val >= 2) {
		//easyPrintLine("NODEZ: %d", WorldGraph.m_cNodes);

		for (int i = 0; i < WorldGraph.m_cNodes; i++) {
			CNode thisNode = WorldGraph.m_pNodes[i];
			//thisNode.m_afNodeInfo
			//UTIL_drawLineFrameBoxAround(thisNode.m_vecOrigin, 9, 30, 255, 255, 0);

			if (specialNode != i) {


				if (tempplayer != NULL) {
					float nodeDist = (WorldGraph.m_pNodes[i].m_vecOrigin - tempplayer->pev->origin).Length();

					if (drawNodeAll_val == 1) {
						UTIL_drawLineFrame(thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z + 20, thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z - 20, 9, 255, 0, 0);
						if (drawNodeConnections_val == 2) {
							//special node connections!
							ShowNodeConnectionsFrame(i);
						}
					}
					else if (drawNodeAll_val >= 10) {
						if (nodeDist < drawNodeAll_val) {
							UTIL_drawLineFrame(thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z + 20, thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z - 20, 9, 255, 0, 0);
							if (drawNodeConnections_val == 2) {
								//special node connections!
								ShowNodeConnectionsFrame(i);
							}
						};
					}
					/*
					if(drawNodeConnections_val == 2){
						//special node connections!
						ShowNodeConnectionsFrame(i);
					}else if(drawNodeConnections_val >= 10){
						//require the node itself to be close enough to the player first, to limit drawing.

						if( nodeDist < drawNodeConnections_val){
							ShowNodeConnectionsFrame(i);
						};
					}
					*/
				}//END OF tempplayer null check

			}
			else {
				UTIL_drawLineFrame(thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z + 20, thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z - 20, 9, 255, 255, 255);
				if (drawNodeConnections_val >= 1) {
					//special node connections!
					ShowNodeConnectionsFrame(specialNode);
				}
			}

			//easyPrintLine("WHERE BE I?? %.2f %.2f %.2f", thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z);
		}
		//WorldGraph.m_pNodes[i].m_vecOriginPeek.z = 
		//	WorldGraph.m_pNodes[i].m_vecOrigin.z = tr.vecEndPos.z + NODE_HEIGHT

	}
	else {
		if (specialNode != -1) {
			CNode thisNode = WorldGraph.m_pNodes[specialNode];
			//still a special node to draw at least.
			UTIL_drawLineFrame(thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z + 15, thisNode.m_vecOrigin.x, thisNode.m_vecOrigin.y, thisNode.m_vecOrigin.z - 15, 7, 255, 255, 255);
			if (drawNodeConnections_val >= 1) {
				//special node connections!
				ShowNodeConnectionsFrame(specialNode);
			}
		}
	}

	//NOTE:::it appears that the end of retail ends the game by making the client's "pev->flags" add "FL_FROZEN".
	//easyPrintLine("MY DETAILZ %d %d %d %d %s", pev->deadflag, pev->effects, pev->flags, pev->renderfx, m_pActiveItem!=NULL?STRING(m_pActiveItem->pev->classname):"NULL");


	//bla bla bla, draw scene stuff.
	if (drawDebugCine_val == 1)
	{
		edict_t* pEdicttt;
		CBaseEntity* pEntityyy;

		//ENGINE_SET_PVS(Vector(2639.43, -743.85, -251.64));

		pEdicttt = g_engfuncs.pfnPEntityOfEntIndex(1);
		pEntityyy;
		if (pEdicttt) {
			for (int i = 1; i < gpGlobals->maxEntities; i++, pEdicttt++) {
				if (pEdicttt->free)	// Not in use
					continue;

				pEntityyy = CBaseEntity::Instance(pEdicttt);
				if (!pEntityyy)
					continue;

				//pEntityyy->alreadySaved = FALSE;


				CBaseMonster* tempmon = NULL;
				//if(  (tempmon = pEntityyy->GetMonsterPointer()) != NULL && tempmon->m_pCine != NULL){
					////tempmon->m_pCine->pev->origin = tempmon->pev->origin + Vector(0,0, 6);
				//}

				if (FClassnameIs(pEntityyy->pev, "scripted_sequence") || FClassnameIs(pEntityyy->pev, "aiscripted_sequence") || FClassnameIs(pEntityyy->pev, "scripted_sentence")) {
					UTIL_drawLineFrame(pEntityyy->pev->origin.x, pEntityyy->pev->origin.y, pEntityyy->pev->origin.z + 15, pEntityyy->pev->origin.x, pEntityyy->pev->origin.y, pEntityyy->pev->origin.z - 15, 7, 0, 255, 12);

				}

			}//END OF loop
		}//END OF if(pEdicttt (first)  )

	}//END OF draw cine's check


}//drawExtraDebugStuff




//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////





