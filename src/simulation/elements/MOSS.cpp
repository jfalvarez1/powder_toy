#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MOSS()
{
	Identifier = "DEFAULT_PT_MOSS";
	Name = "MOSS";
	Colour = 0x2D5A27_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
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

	Flammable = 30;
	Explosive = 0;
	Meltable = 0;
	Hardness = 10;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Growth level
	HeatConduct = 30;
	Description = "Moss. Grows slowly on stone in damp conditions. Produces oxygen.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 260.0f;  // Dies if frozen
	LowTemperatureTransition = PT_DUST;
	HighTemperature = 373.0f;  // Dries out and dies
	HighTemperatureTransition = PT_DUST;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	bool hasWater = false;
	bool hasStone = false;

	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
				{
					// Try to spread to empty spaces on stone
					if (parts[i].tmp >= 50 && hasStone && hasWater && sim->rng.chance(1, 200))
					{
						// Check if there's stone adjacent to the empty space
						for (int dx = -1; dx <= 1; dx++)
						{
							for (int dy = -1; dy <= 1; dy++)
							{
								auto r2 = pmap[y+ry+dy][x+rx+dx];
								if (r2 && (TYP(r2) == PT_STNE || TYP(r2) == PT_ROCK || TYP(r2) == PT_BRCK))
								{
									int np = sim->create_part(-1, x+rx, y+ry, PT_MOSS);
									if (np >= 0)
									{
										parts[np].tmp = 0;
										parts[i].tmp -= 25;
									}
									goto done_spread;
								}
							}
						}
					}
					done_spread:
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
					hasWater = true;
					// Absorb water to grow
					if (sim->rng.chance(1, 500))
					{
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 10, 100);
					}
					break;
				case PT_STNE:
				case PT_ROCK:
				case PT_BRCK:
				case PT_CNCT:
					hasStone = true;
					break;
				case PT_CO2:
					// Absorb CO2, produce O2 (photosynthesis!)
					if (sim->rng.chance(1, 100))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_O2);
						parts[i].tmp = std::min(parts[i].tmp + 5, 100);
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
					sim->part_change_type(i, x, y, PT_FIRE);
					parts[i].life = sim->rng.between(10, 30);
					return 0;
				case PT_ACID:
					if (sim->rng.chance(1, 20))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Slowly produce O2 if grown
	if (parts[i].tmp >= 30 && sim->rng.chance(1, 500))
	{
		int np = sim->create_part(-1, x, y - 1, PT_O2);
		if (np >= 0)
		{
			parts[np].temp = parts[i].temp;
		}
	}

	// Slowly wither without water
	if (!hasWater && sim->rng.chance(1, 2000))
	{
		parts[i].tmp = std::max(parts[i].tmp - 1, 0);
		if (parts[i].tmp <= 0)
		{
			sim->part_change_type(i, x, y, PT_DUST);
			return 0;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int growth = cpart->tmp;

	// Greener when more grown
	*colr = 45 - growth / 5;
	*colg = 90 + growth / 2;
	*colb = 39 - growth / 5;

	if (*colr < 20) *colr = 20;
	if (*colg > 150) *colg = 150;
	if (*colb < 20) *colb = 20;

	return 0;
}
