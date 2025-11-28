#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_BUBL()
{
	Identifier = "DEFAULT_PT_BUBL";
	Name = "BUBL";
	Colour = 0xAADDFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.1f;  // Float up
	Diffusion = 0.20f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 200;  // Lifetime
	DefaultProperties.tmp = 10;    // Size
	HeatConduct = 5;
	Description = "Bubbles. Float upward and pop! Can trap small particles inside.";

	Properties = TYPE_GAS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 2.0f;  // Pop under pressure
	HighPressureTransition = PT_NONE;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.between(100, 300);
	sim->parts[i].tmp = sim->rng.between(5, 15);  // Random size
}

static int update(UPDATE_FUNC_ARGS)
{
	// Pop when life runs out
	if (parts[i].life <= 0)
	{
		// Pop effect - tiny splash
		for (int j = 0; j < 3; j++)
		{
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_WTRV);
			if (np >= 0)
			{
				parts[np].temp = parts[i].temp;
			}
		}
		sim->kill_part(i);
		return 1;
	}

	// Wobble around
	if (sim->rng.chance(1, 3))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-10, 5) * 0.02f;  // Bias upward
	}

	// Grow slightly over time
	if (parts[i].tmp < 20 && sim->rng.chance(1, 100))
	{
		parts[i].tmp++;
	}

	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Sharp things pop bubbles
				if (rt == PT_BRCK || rt == PT_STNE || rt == PT_METL || rt == PT_IRON ||
				    rt == PT_GLAS || rt == PT_DMND)
				{
					if (sim->rng.chance(1, 10))
					{
						parts[i].life = 0;
						return 0;
					}
				}

				// Fire pops bubbles instantly
				if (rt == PT_FIRE || rt == PT_PLSM || rt == PT_LAVA)
				{
					parts[i].life = 0;
					return 0;
				}

				// Bubbles merge
				if (rt == PT_BUBL && sim->rng.chance(1, 50))
				{
					parts[i].tmp = std::min(parts[i].tmp + parts[rID].tmp / 2, 30);
					parts[i].life = std::max(parts[i].life, parts[rID].life);
					sim->kill_part(rID);
				}

				// Trap small particles (carry them along)
				if (rt == PT_DUST || rt == PT_SPOR || rt == PT_PXLD || rt == PT_GLTR)
				{
					parts[rID].vx = parts[i].vx;
					parts[rID].vy = parts[i].vy;
				}
			}
		}
	}

	// Pop under high pressure
	if (sim->pv[y/CELL][x/CELL] > 3.0f)
	{
		parts[i].life = 0;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int size = cpart->tmp;
	int life = cpart->life;

	// Translucent blue
	*colr = 170;
	*colg = 220;
	*colb = 255;

	// Transparency based on size
	*pixel_mode |= PMODE_BLEND;
	*cola = 50 + size * 3;

	// Shimmery glow
	*firea = 20 + size;
	*firer = 200;
	*fireg = 230;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD;

	// Flash before popping
	if (life < 20)
	{
		*colr = 255;
		*colg = 255;
		*colb = 255;
	}

	return 0;
}
