#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_ALCH()
{
	Identifier = "DEFAULT_PT_ALCH";
	Name = "ALCH";
	Colour = 0xFFD700_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
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

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 100;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Transmutation charges
	HeatConduct = 200;
	Description = "Philosopher's Stone. Transmutes base metals to gold! Limited uses.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;  // Indestructible by heat
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].tmp <= 0)
	{
		// Exhausted - turns to regular stone
		sim->part_change_type(i, x, y, PT_STNE);
		return 0;
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

				// Transmutation table!
				int newType = 0;
				int cost = 1;

				switch (rt)
				{
				// Base metals to gold
				case PT_IRON:
				case PT_METL:
				case PT_BMTL:
					newType = PT_GOLD;
					cost = 5;
					break;
				case PT_BRMT:
					newType = PT_GOLD;
					cost = 3;
					break;
				// Stone to metal
				case PT_STNE:
				case PT_ROCK:
					newType = PT_IRON;
					cost = 2;
					break;
				// Sand to glass
				case PT_SAND:
					newType = PT_GLAS;
					cost = 1;
					break;
				// Coal to diamond!
				case PT_COAL:
				case PT_BCOL:
					newType = PT_DMND;
					cost = 20;
					break;
				// Water to healing
				case PT_WATR:
					newType = PT_DSTW;  // Distilled/pure
					cost = 1;
					break;
				// Cure diseases
				case PT_VIRS:
				case PT_VRSS:
				case PT_VRSG:
					sim->kill_part(rID);
					cost = 3;
					continue;
				// Resurrect dead plants
				case PT_DUST:
					if (sim->rng.chance(1, 10))
					{
						newType = PT_PLNT;
						cost = 5;
					}
					break;
				// Acid to water
				case PT_ACID:
					newType = PT_WATR;
					cost = 2;
					break;
				default:
					break;
				}

				if (newType > 0 && parts[i].tmp >= cost)
				{
					if (sim->rng.chance(1, 20))  // Not instant
					{
						sim->part_change_type(rID, x+rx, y+ry, newType);
						parts[i].tmp -= cost;

						// Transmutation sparkle
						int np = sim->create_part(-1, x+rx, y+ry-1, PT_EMBR);
						if (np >= 0)
						{
							parts[np].life = 10;
							parts[np].ctype = 0xFFD700;
							parts[np].vy = -1;
						}
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int charges = cpart->tmp;

	// Golden color
	*colr = 255;
	*colg = 215;
	*colb = 0;

	// Dims as charges deplete
	if (charges < 50)
	{
		float fade = charges / 50.0f;
		*colr = (int)(255 * fade);
		*colg = (int)(215 * fade);
	}

	// Magical glow
	*firea = 50 + charges;
	*firer = 255;
	*fireg = 215;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
