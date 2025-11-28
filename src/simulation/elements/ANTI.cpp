#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_ANTI()
{
	Identifier = "DEFAULT_PT_ANTI";
	Name = "ANTI";
	Colour = 0x000033_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.0f;  // Floats eerily
	Diffusion = 0.10f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Energy level
	HeatConduct = 0;
	Description = "Antimatter. Annihilates on contact with regular matter in massive explosions!";

	Properties = TYPE_PART | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Float eerily
	if (sim->rng.chance(1, 10))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-10, 10) * 0.02f;
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

				// Don't annihilate with other antimatter or special elements
				if (rt == PT_ANTI || rt == PT_DMND || rt == PT_CLNE || rt == PT_VOID ||
				    rt == PT_INSL || rt == PT_PHOT || rt == PT_NEUT || rt == PT_PROT)
					continue;

				// ANNIHILATION! Matter + Antimatter = BOOM
				// Create massive explosion
				sim->pv[y/CELL][x/CELL] += 50.0f;

				// Create explosion particles
				for (int j = 0; j < 10; j++)
				{
					int px = x + sim->rng.between(-3, 3);
					int py = y + sim->rng.between(-3, 3);

					// Create plasma
					int np = sim->create_part(-1, px, py, PT_PLSM);
					if (np >= 0)
					{
						parts[np].temp = 10000.0f;
						parts[np].life = 50;
						parts[np].vx = sim->rng.between(-20, 20);
						parts[np].vy = sim->rng.between(-20, 20);
					}
				}

				// Create photons (gamma rays)
				for (int j = 0; j < 5; j++)
				{
					int np = sim->create_part(-1, x, y, PT_PHOT);
					if (np >= 0)
					{
						parts[np].vx = sim->rng.between(-30, 30);
						parts[np].vy = sim->rng.between(-30, 30);
						parts[np].ctype = 0xFFFFFF;
					}
				}

				// Destroy the matter particle
				sim->kill_part(rID);

				// Destroy the antimatter
				sim->kill_part(i);
				return 1;
			}
		}
	}

	// Slowly decay
	if (sim->rng.chance(1, 5000))
	{
		sim->kill_part(i);
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Eerie dark blue with inverse glow effect
	*colr = 0;
	*colg = 0;
	*colb = 80;

	// Pulsing dark aura
	int pulse = (cpart->tmp % 20);
	*firea = 50 + pulse * 2;
	*firer = 20;
	*fireg = 0;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
