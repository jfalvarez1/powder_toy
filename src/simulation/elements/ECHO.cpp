#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_ECHO()
{
	Identifier = "DEFAULT_PT_ECHO";
	Name = "ECHO";
	Colour = 0xCCFFFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.05f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 5;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 100;   // Duration
	DefaultProperties.tmp = 0;      // Charge
	DefaultProperties.ctype = 0;    // Element to copy
	HeatConduct = 0;
	Description = "Echo Matter. Copies any particle it touches and creates duplicates!";

	Properties = TYPE_PART | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 500.0f;
	HighTemperatureTransition = PT_NONE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Fade when life runs out
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Float around
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-10, 10) * 0.02f;
	}

	// Recharge
	if (parts[i].tmp < 50 && sim->rng.chance(1, 10))
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
				{
					// Create echo (copy) if charged
					if (parts[i].ctype > 0 && parts[i].tmp >= 20 && sim->rng.chance(1, 10))
					{
						int np = sim->create_part(-1, x+rx, y+ry, parts[i].ctype);
						if (np >= 0)
						{
							parts[np].temp = parts[i].temp;
							// Copies are temporary
							parts[np].life = 50;
							parts[i].tmp -= 20;
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				// Don't copy special elements
				if (rt == PT_ECHO || rt == PT_CLNE || rt == PT_VOID || rt == PT_DMND ||
				    rt == PT_PCLN || rt == PT_BCLN || rt == PT_CONV)
					continue;

				// Copy the touched element
				if (parts[i].ctype == 0 || sim->rng.chance(1, 50))
				{
					parts[i].ctype = rt;
					parts[i].tmp = std::min(parts[i].tmp + 10, 50);  // Gain charge
				}

				// Echo duplicates everything it passes through
				if (parts[i].ctype == rt && parts[i].tmp >= 30 && sim->rng.chance(1, 20))
				{
					// Find empty spot
					for (int dx = -2; dx <= 2; dx++)
					{
						for (int dy = -2; dy <= 2; dy++)
						{
							if (!pmap[y+dy][x+dx] && sim->rng.chance(1, 3))
							{
								int np = sim->create_part(-1, x+dx, y+dy, rt);
								if (np >= 0)
								{
									parts[np].temp = parts[rID].temp;
									parts[np].vx = parts[rID].vx;
									parts[np].vy = parts[rID].vy;
									parts[i].tmp -= 10;
								}
								goto done_echo;
							}
						}
					}
					done_echo:;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int charge = cpart->tmp;
	int copyType = cpart->ctype;

	// Base ghostly white-blue
	*colr = 200;
	*colg = 255;
	*colb = 255;

	// Tint based on what it's copying
	if (copyType > 0)
	{
		// Mix in color of copied element
		charge = std::min(charge, 50);
		*colr = 200 - charge * 2;
		*colg = 255;
		*colb = 255 - charge;
	}

	// Ethereal glow
	*firea = 50 + charge * 2;
	*firer = 180;
	*fireg = 230;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	// Transparent
	*pixel_mode |= PMODE_BLEND;
	*cola = 100 + charge * 2;

	return 0;
}
