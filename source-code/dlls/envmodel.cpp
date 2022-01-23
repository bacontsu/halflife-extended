//This code were made for HL:E
//env_model made by bacontsu, pretty basic

#include "extdll.h"
#include "util.h"
#include "cbase.h"

class EnvModel : public CBaseEntity
{
public:
	void Spawn() override;
	void Use(CBaseEntity* pActivator, CBaseEntity* pInflictor, USE_TYPE useType, float value) override;
};


LINK_ENTITY_TO_CLASS( env_model , EnvModel );

void EnvModel::Spawn()
{
	if (!pev->model)
	{
		ALERT(at_console, "env_model doesn't have a model set up!");
		UTIL_Remove(this); //remove to reduce entity count
		return;
	}
	PRECACHE_MODEL( STRING (pev->model ) );
	SET_MODEL( edict(), STRING( pev->model ) );

	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;

}

void EnvModel::Use(CBaseEntity* pActivator, CBaseEntity* pInflictor, USE_TYPE useType, float value)
{
	pev->effects ^= EF_NODRAW;
}