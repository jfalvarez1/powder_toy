#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_CAKE()
{
	Identifier = "DEFAULT_PT_CAKE";
	Name = "CAKE";
	Colour = 0xFFE4C4_rgb;
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
	Hardness = 5;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 500;  // Freshness
	DefaultProperties.tmp = 0;     // Frosting color
	HeatConduct = 30;
	Description = "Cake. Delicious! Attracts creatures. Goes stale over time.";

	Properties = TYPE_SOLID | PROP_LIFE_DEC | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 10.0f;  // Squishes
	HighPressureTransition = PT_DUST;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 450.0f;  // Burns
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Goes stale
	if (parts[i].life <= 0)
	{
		// Moldy!
		if (sim->rng.chance(1, 10))
		{
			sim->part_change_type(i, x, y, PT_DUST);
			return 0;
		}
	}

	// Attract creatures from a distance
	int attractRange = 10;
	for (int rx = -attractRange; rx <= attractRange; rx++)
	{
		for (int ry = -attractRange; ry <= attractRange; ry++)
		{
			if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
				continue;

			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			auto rt = TYP(r);
			auto rID = ID(r);

			// Living things are attracted to cake!
			if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH ||
			    rt == PT_SWRM || rt == PT_BLOB)
			{
				// Pull toward cake
				float dist = sqrtf(rx*rx + ry*ry);
				if (dist > 2)
				{
					parts[rID].vx -= rx * 0.02f;
					parts[rID].vy -= ry * 0.02f;
				}
			}
		}
	}

	// Nearby interactions
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
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Stickmen eat cake and heal!
					if (sim->rng.chance(1, 30))
					{
						parts[rID].life = std::min(parts[rID].life + 10, 100);
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_SWRM:
				case PT_BLOB:
					// Swarms/blobs eat cake
					if (sim->rng.chance(1, 20))
					{
						parts[rID].life = std::min(parts[rID].life + 50, 1000);
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Water makes cake soggy (faster decay)
					parts[i].life -= 5;
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Burns!
					sim->part_change_type(i, x, y, PT_FIRE);
					parts[i].life = 40;
					return 0;
				case PT_YEST:
					// Yeast makes cake rise (expand)
					if (sim->rng.chance(1, 100))
					{
						for (int dx = -1; dx <= 1; dx++)
						{
							for (int dy = -1; dy <= 1; dy++)
							{
								if (!pmap[y+dy][x+dx])
								{
									sim->create_part(-1, x+dx, y+dy, PT_CAKE);
									goto done_rise;
								}
							}
						}
						done_rise:;
					}
					break;
				case PT_HONY:
					// Honey preserves cake
					parts[i].life = std::min(parts[i].life + 10, 500);
					break;
				default:
					break;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int fresh = cpart->life;
	int frosting = cpart->tmp;

	// Base cake color (biscuit/sponge)
	*colr = 255;
	*colg = 228;
	*colb = 196;

	// Gets darker (stale/moldy) as it ages
	if (fresh < 200)
	{
		int staleness = (200 - fresh) / 4;
		*colr -= staleness;
		*colg -= staleness * 2;
		*colb -= staleness;

		// Green mold spots when very stale
		if (fresh < 50)
		{
			*colr = 100;
			*colg = 150;
			*colb = 100;
		}
	}

	// Fresh cake looks appetizing
	if (fresh > 400)
	{
		*firea = 20;
		*firer = 255;
		*fireg = 230;
		*fireb = 200;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
