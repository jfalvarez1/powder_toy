#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PHOS()
{
	Identifier = "DEFAULT_PT_PHOS";
	Name = "PHOS";
	Colour = 0xFFFF99_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 100;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 0;  // Glow timer
	HeatConduct = 30;
	Description = "Phosphorus. Glows in the dark, spontaneously ignites in oxygen. Very reactive.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 317.0f;  // Melts at 44C
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Glow cycle
	if (parts[i].life > 0)
		parts[i].life--;
	else
		parts[i].life = sim->rng.between(50, 100);

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

				switch (rt)
				{
				case PT_O2:
					// Spontaneous combustion in oxygen!
					if (sim->rng.chance(1, 20))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = sim->rng.between(100, 200);
						parts[i].temp = 2000.0f + 273.15f;
						sim->pv[y/CELL][x/CELL] += 2.0f;
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Reacts violently with water
					if (sim->rng.chance(1, 50))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = sim->rng.between(50, 100);
						sim->pv[y/CELL][x/CELL] += 1.0f;
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Catches fire easily
					if (sim->rng.chance(1, 5))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = sim->rng.between(100, 200);
						parts[i].temp = 2000.0f + 273.15f;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Emit faint glow particles
	if (sim->rng.chance(1, 200))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_PHOT);
		if (np >= 0)
		{
			parts[np].ctype = 0x00FFFF00;  // Yellow-green
			parts[np].vx = sim->rng.between(-2, 2) * 0.5f;
			parts[np].vy = sim->rng.between(-2, 2) * 0.5f;
			parts[np].life = 20;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Glowing effect
	int glow = 50 + (cpart->life % 50);

	*colr = 255;
	*colg = 255;
	*colb = 100 + glow;

	*firea = glow;
	*firer = 200;
	*fireg = 255;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
