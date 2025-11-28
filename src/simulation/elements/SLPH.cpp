#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SLPH()
{
	Identifier = "DEFAULT_PT_SLPH";
	Name = "SLPH";
	Colour = 0xFFFF00_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.95f;
	Collision = -0.1f;
	Gravity = 0.2f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 50;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;

	Weight = 35;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Burn state
	HeatConduct = 50;
	Description = "Sulfur. Burns with blue flame, produces toxic fumes. Used in explosives.";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 388.0f;  // Melts at 115C
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	bool nearFire = false;
	bool nearOxygen = false;

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

				switch (rt)
				{
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
					nearFire = true;
					break;
				case PT_O2:
					nearOxygen = true;
					// Sulfur + O2 = SO2 (toxic gas)
					if (parts[i].tmp > 0 && sim->rng.chance(1, 20))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_SMKE);
						parts[rID].life = 100;
						parts[rID].temp = parts[i].temp + 200.0f;
					}
					break;
				case PT_NITR:
				case PT_GBMB:
					// Makes explosives more powerful!
					if (sim->rng.chance(1, 100))
					{
						parts[rID].tmp = std::min(parts[rID].tmp + 10, 100);
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Sulfur + water = sulfuric acid!
					if (parts[i].temp > 373.0f && sim->rng.chance(1, 200))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_ACID);
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Auto-ignite if hot enough
	if (parts[i].temp > 505.0f)  // 232C ignition point
	{
		parts[i].tmp = 1;
	}

	// Burning state
	if (parts[i].tmp > 0)
	{
		parts[i].temp += 5.0f;
		parts[i].tmp++;

		// Create blue fire effect
		if (sim->rng.chance(1, 5))
		{
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y - 1, PT_FIRE);
			if (np >= 0)
			{
				parts[np].life = 10;
				parts[np].temp = parts[i].temp;
				parts[np].ctype = 0x0000FF;  // Blue fire!
			}
		}

		// Burn out
		if (parts[i].tmp > 100)
		{
			sim->kill_part(i);
			return 1;
		}
	}

	// Ignite from fire/heat
	if (nearFire && nearOxygen && parts[i].tmp == 0)
	{
		parts[i].tmp = 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int burn = cpart->tmp;

	if (burn > 0)
	{
		// Blue flame when burning!
		*colr = 50;
		*colg = 100 + burn;
		*colb = 200 + burn / 2;
		if (*colg > 200) *colg = 200;
		if (*colb > 255) *colb = 255;

		*firea = burn * 2;
		*firer = 50;
		*fireg = 150;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD | PMODE_GLOW;
	}
	else
	{
		// Yellow sulfur
		*colr = 255;
		*colg = 255;
		*colb = 0;
	}

	return 0;
}
