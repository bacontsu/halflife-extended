#pragma once



#define DRIPSPEED	900		// drips fall speed (pixels/sec.)
#define SNOWSPEED	200		// snoflakes fall speed (pixels/sec.)
#define SNOWFADEDIST	80

#define MAXDRIPS	2000	// drips limit
#define MAXFX		3000	// additional SFX limit (water splashes)

#define DRIP_SPRITE_HALFHEIGHT	46
#define DRIP_SPRITE_HALFWIDTH	8
#define SNOW_SPRITE_HALFSIZE	3

// water splash radius after a second of existing
#define MAXRINGHALFSIZE		25


typedef struct
{
	int			dripsPerSecond;
	float		distFromPlayer;
	float		windX, windY;
	float		randX, randY;
	int			weatherMode;	// 0 - snow, 1 - rain
	float		globalHeight;
	
} rain_properties;

typedef struct cl_drip
{
	
	float		birthTime;
	float		minHeight;	// the drip will be destroyed at this height
	Vector		origin;
	float		alpha;
	
	float		xDelta;		// side speed
	float		yDelta;
	int			landInWater;
	

	
	cl_drip * p_Next;		// next drip in chain
	cl_drip * p_Prev;		// previous drip in chain
	
} cl_drip_t;

typedef struct cl_rainfx
{
	float		birthTime;
	float		life;
	Vector		origin;
	float		alpha;
	
	cl_rainfx * p_Next;		// next fx in chain
	cl_rainfx * p_Prev;		// previous fx in chain
	
} cl_rainfx_t;

void ProcessRain(void);
void ProcessFXObjects(void);
void ResetRain(void);
void InitRain(void);