#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SPOR()
{
	Identifier = "DEFAULT_PT_SPOR";
	Name = "SPOR";
	Colour = 0x8B7355_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIFE;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.90f;
	Collision = 0.0f;
	Gravity = 0.02f;  // Very light, floats
	Diffusion = 0.30f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 40;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 3;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 500;  // Viability timer
	DefaultProperties.tmp = 0;     // Infection progress
	HeatConduct = 20;
	Description = "Spores. Fungal spores that infect and convert organic matter.";

	Properties = TYPE_PART | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 373.0f;  // Dies when boiled
	HighTemperatureTransition = PT_DUST;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Die if lifespan runs out
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Float on air currents
	float airX = sim->vx[y/CELL][x/CELL];
	float airY = sim->vy[y/CELL][x/CELL];
	parts[i].vx += airX * 0.1f;
	parts[i].vy += airY * 0.1f;

	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
				{
					// Release more spores if mature
					if (parts[i].tmp >= 100 && sim->rng.chance(1, 50))
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_SPOR);
						if (np >= 0)
						{
							parts[np].life = 500;
							parts[np].tmp = 0;
							parts[i].tmp -= 20;
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				switch (rt)
				{
				case PT_PLNT:
				case PT_VINE:
				case PT_WOOD:
					// Infect plants!
					if (sim->rng.chance(1, 30))
					{
						// Convert to fungal matter
						sim->part_change_type(rID, x+rx, y+ry, PT_SPOR);
						parts[rID].life = 1000;
						parts[rID].tmp = 50;
						parts[i].tmp += 20;
					}
					break;
				case PT_YEST:
					// Combine with yeast - super infection
					if (sim->rng.chance(1, 20))
					{
						parts[i].tmp += 30;
						parts[i].life += 100;
						sim->kill_part(rID);
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Spores love moisture
					parts[i].life = std::min(parts[i].life + 5, 1000);
					break;
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
					// Fire kills spores
					sim->kill_part(i);
					return 1;
				case PT_SOAP:
				case PT_ACID:
					// Soap and acid kill spores
					if (sim->rng.chance(1, 10))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_SPOR:
					// Spores clump together
					parts[i].vx += rx * 0.02f;
					parts[i].vy += ry * 0.02f;
					// Share nutrients
					if (parts[rID].tmp < parts[i].tmp - 10)
					{
						parts[i].tmp -= 5;
						parts[rID].tmp += 5;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Mature over time
	if (parts[i].tmp < 100 && sim->rng.chance(1, 100))
	{
		parts[i].tmp++;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int maturity = cpart->tmp;

	// Brown -> greenish as they mature
	*colr = 139 - maturity / 3;
	*colg = 115 + maturity / 2;
	*colb = 85 - maturity / 3;

	if (*colr < 80) *colr = 80;
	if (*colg > 170) *colg = 170;
	if (*colb < 40) *colb = 40;

	// Glow when mature
	if (maturity > 50)
	{
		*firea = maturity / 3;
		*firer = 100;
		*fireg = 150;
		*fireb = 80;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
