#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_GFLY()
{
	Identifier = "DEFAULT_PT_GFLY";
	Name = "GFLY";
	Colour = 0xAAFF00_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIFE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = 0.0f;
	Gravity = -0.01f;  // Slight float
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 1000;  // Lifespan
	DefaultProperties.tmp = 0;      // Glow cycle
	DefaultProperties.tmp2 = 100;   // Energy
	HeatConduct = 10;
	Description = "Glowfly. Bioluminescent insects that glow and zap when provoked!";

	Properties = TYPE_PART | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_DUST;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Die when life runs out
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Glow cycle
	parts[i].tmp = (parts[i].tmp + 1) % 40;

	// Recharge energy
	if (parts[i].tmp2 < 100 && sim->rng.chance(1, 20))
	{
		parts[i].tmp2++;
	}

	// Fly around randomly
	if (sim->rng.chance(1, 3))
	{
		parts[i].vx = sim->rng.between(-20, 20) * 0.1f;
		parts[i].vy = sim->rng.between(-20, 20) * 0.1f;
	}

	// Find other glowflies - swarm behavior
	for (int rx = -5; rx <= 5; rx++)
	{
		for (int ry = -5; ry <= 5; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Swarm toward other glowflies
				if (rt == PT_GFLY)
				{
					float dist = sqrtf(rx*rx + ry*ry);
					if (dist > 3)
					{
						parts[i].vx += rx * 0.02f;
						parts[i].vy += ry * 0.02f;
					}
				}

				// ZAP threats!
				if (rt == PT_FIRE || rt == PT_PLSM || rt == PT_LAVA || rt == PT_ACID)
				{
					// Flee!
					parts[i].vx -= rx * 0.5f;
					parts[i].vy -= ry * 0.5f;
				}

				// Zap predators
				if (rt == PT_SWRM)
				{
					if (parts[i].tmp2 > 30 && sim->rng.chance(1, 10))
					{
						// Electric zap!
						int np = sim->create_part(-1, x + rx, y + ry, PT_SPRK);
						if (np >= 0)
						{
							parts[np].life = 4;
							parts[np].ctype = PT_METL;
						}
						parts[i].tmp2 -= 20;
						// Hurt the swarm
						parts[rID].life -= 20;
					}
				}

				// Attracted to light sources
				if (rt == PT_GLOW || rt == PT_PHOT)
				{
					parts[i].vx += rx * 0.03f;
					parts[i].vy += ry * 0.03f;
				}
			}
		}
	}

	// Reproduce when well-fed and enough energy
	if (parts[i].life > 800 && parts[i].tmp2 > 80 && sim->rng.chance(1, 500))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_GFLY);
		if (np >= 0)
		{
			parts[np].life = 500;
			parts[np].tmp2 = 50;
			parts[i].life -= 200;
			parts[i].tmp2 -= 30;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int glow = cpart->tmp;
	int energy = cpart->tmp2;

	// Base green-yellow color
	*colr = 170;
	*colg = 255;
	*colb = 0;

	// Pulsing glow effect
	int pulse = (glow < 20) ? glow : 40 - glow;
	int glowIntensity = 50 + pulse * 5 + energy / 2;

	*firea = glowIntensity;
	*firer = 180;
	*fireg = 255;
	*fireb = 50;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	// Brighter when glow peaks
	if (glow < 5)
	{
		*colr = 255;
		*colg = 255;
		*colb = 100;
	}

	return 0;
}
