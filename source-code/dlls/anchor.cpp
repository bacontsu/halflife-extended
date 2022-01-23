//This code was made for HL:E ~ Bacontsu
//trigger_anchor, made to anchor entities to other entities

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "UserMessages.h"

class TriggerAnchor : public CBaseEntity
{
public:

	int	Save(CSave& save) override;
	int	Restore(CRestore& restore) override;
	static TYPEDESCRIPTION m_SaveData[];

	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pInflictor, USE_TYPE useType, float value) override;
	void Think() override;
	void KeyValue(KeyValueData* pkvd) override;

	bool enabled;
	bool enabledPosition;
	bool enabledRotation;
	bool relativeToAnchor;
	int xOffset = 0;
	int yOffset = 0;
	int zOffset = 0;
	int rollOffset = 0;
	int pitchOffset = 0;
	int yawOffset = 0;
	EHANDLE pEntity;
	EHANDLE pAnchor;


};


LINK_ENTITY_TO_CLASS(trigger_anchor, TriggerAnchor);

TYPEDESCRIPTION	TriggerAnchor::m_SaveData[] =
{
	DEFINE_FIELD(TriggerAnchor, enabled, FIELD_BOOLEAN),
	DEFINE_FIELD(TriggerAnchor, enabledPosition, FIELD_BOOLEAN),
	DEFINE_FIELD(TriggerAnchor, xOffset, FIELD_INTEGER),
	DEFINE_FIELD(TriggerAnchor, yOffset, FIELD_INTEGER),
	DEFINE_FIELD(TriggerAnchor, zOffset, FIELD_INTEGER),
	DEFINE_FIELD(TriggerAnchor, rollOffset, FIELD_INTEGER),
	DEFINE_FIELD(TriggerAnchor, pitchOffset, FIELD_INTEGER),
	DEFINE_FIELD(TriggerAnchor, yawOffset, FIELD_INTEGER),
	DEFINE_FIELD(TriggerAnchor, enabledRotation, FIELD_BOOLEAN),
	DEFINE_FIELD(TriggerAnchor, relativeToAnchor, FIELD_BOOLEAN),
	//entities
	DEFINE_FIELD(TriggerAnchor, pEntity, FIELD_EHANDLE),
	DEFINE_FIELD(TriggerAnchor, pAnchor, FIELD_EHANDLE),

};

IMPLEMENT_SAVERESTORE(TriggerAnchor, CBaseEntity);

void TriggerAnchor::KeyValue(KeyValueData* pkvd)
{
	if (FStrEq(pkvd->szKeyName, "triggerstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			enabled = false;
			break;
		case 1:
			enabled = true;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "positionstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			enabledPosition = false;
			break;
		case 1:
			enabledPosition = true;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "xOffset"))
	{
		int offset = atoi(pkvd->szValue);
		xOffset = offset;
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "yOffset"))
	{
		int offset = atoi(pkvd->szValue);
		yOffset = offset;
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "zOffset"))
	{
		int offset = atoi(pkvd->szValue);
		zOffset = offset;
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "rotationstate"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			enabledRotation = false;
			break;
		case 1:
			enabledRotation = true;
			break;
		}
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rollOffset"))
	{
		int offset = atoi(pkvd->szValue);
		rollOffset = offset;
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "pitchOffset"))
	{
		int offset = atoi(pkvd->szValue);
		pitchOffset = offset;
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "yawOffset"))
	{
		int offset = atoi(pkvd->szValue);
		yawOffset = offset;
		pkvd->fHandled = TRUE;

	}
	else if (FStrEq(pkvd->szKeyName, "OverrideOffset"))
	{
		int type = atoi(pkvd->szValue);
		switch (type)
		{
		case 0:
			relativeToAnchor = false;
			break;
		case 1:
			relativeToAnchor = true;
			break;
		}

	}
	else
		CBaseEntity::KeyValue(pkvd);




}

void TriggerAnchor::Spawn()
{

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->nextthink = gpGlobals->time + 0.005f;



}

void TriggerAnchor::Think()
{

	if (enabled)
	{

		pEntity = UTIL_FindEntityByTargetname(pEntity, STRING(pev->target));

		if (pev->message)
		{
			pAnchor = UTIL_FindEntityByTargetname(pAnchor, STRING(pev->message));
		}
		else
		{
			pAnchor = this;
		}

		if (!FNullEnt(pEntity) && !FNullEnt(pAnchor))
		{
			if (pEntity->IsBSPModel())
			{
				pEntity->pev->movetype = MOVETYPE_PUSH; //brushes are mad whenever i move them without this
			}
			else
			{
				pEntity->pev->movetype = MOVETYPE_FLY;
			}

			if (relativeToAnchor && xOffset == 0 && yOffset == 0 && zOffset == 0)
			{
				xOffset = (pEntity->pev->origin.x - pAnchor->pev->origin.x);
				yOffset = (pEntity->pev->origin.y - pAnchor->pev->origin.y);
				zOffset = (pEntity->pev->origin.z - pAnchor->pev->origin.z);
			}

			pEntity->pev->velocity.z = 0; //no gravity didnt really works for some reason

			if (enabledPosition)
			{
				MAKE_VECTORS(pEntity->pev->angles);


				//ALERT(at_console, "%d %d %d", xOffset, yOffset, zOffset);

				Vector forwardOffset = gpGlobals->v_forward * xOffset;
				Vector rightOffset = gpGlobals->v_right * yOffset * -1;
				Vector upOffset = gpGlobals->v_up * zOffset;

				const Vector vecNewOrigin = pAnchor->pev->origin + forwardOffset + rightOffset + upOffset;
				UTIL_SetOrigin(pEntity->pev, vecNewOrigin);
			}

			if (enabledRotation)
			{
				Vector AnglesOffset;
				AnglesOffset.x = rollOffset;
				AnglesOffset.y = pitchOffset;
				AnglesOffset.z = yawOffset;

				if (!FNullEnt(pEntity))
					pEntity->pev->angles = pAnchor->pev->angles;
			}

		}

	}
	pev->nextthink = gpGlobals->time + 0.005f;
}

void TriggerAnchor::Use(CBaseEntity* pActivator, CBaseEntity* pInflictor, USE_TYPE useType, float value)
{
	enabled = !enabled;
}