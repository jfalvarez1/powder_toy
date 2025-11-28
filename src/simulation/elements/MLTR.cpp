#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MLTR()
{
	Identifier = "DEFAULT_PT_MLTR";
	Name = "MLTR";
	Colour = 0xFF4400_rgb;
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
	HotAir = 0.010f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Weight = 100;

	DefaultProperties.temp = 3000.0f;  // VERY HOT
	DefaultProperties.tmp = 100;       // Fuel/power
	HeatConduct = 255;  // Conducts heat extremely well
	Description = "Melter. Extremely hot! Melts almost anything it touches.";

	Properties = TYPE_SOLID | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 500.0f;  // Cools and dies
	LowTemperatureTransition = PT_METL;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Maintain extreme heat while powered
	if (parts[i].tmp > 0)
	{
		if (parts[i].temp < 3000.0f)
		{
			parts[i].temp += 50.0f;
		}
		parts[i].tmp--;
	}

	// Melt everything nearby!
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

				// Transfer massive heat
				if (parts[rID].temp < parts[i].temp - 100)
				{
					parts[rID].temp += 200.0f;
					parts[i].temp -= 10.0f;  // Cool slightly
				}

				// Force melt things that resist
				switch (rt)
				{
				case PT_DMND:
					// Even diamond eventually yields
					if (sim->rng.chance(1, 1000) && parts[i].temp > 4000.0f)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_COAL);
					}
					break;
				case PT_TTAN:
					// Titanium melts
					if (parts[rID].temp > 2000.0f)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LAVA);
						parts[rID].ctype = PT_TTAN;
					}
					break;
				case PT_GOLD:
					// Gold melts easily
					if (parts[rID].temp > 1337.0f)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LAVA);
						parts[rID].ctype = PT_GOLD;
					}
					break;
				case PT_STNE:
				case PT_ROCK:
				case PT_BRCK:
				case PT_CNCT:
					// Stone melts
					if (parts[rID].temp > 1500.0f)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LAVA);
						parts[rID].ctype = rt;
					}
					break;
				case PT_METL:
				case PT_IRON:
				case PT_BMTL:
					// Metals melt
					if (parts[rID].temp > 1500.0f)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LAVA);
						parts[rID].ctype = rt;
					}
					break;
				case PT_GLAS:
				case PT_QRTZ:
					// Glass melts
					if (parts[rID].temp > 1700.0f)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LAVA);
						parts[rID].ctype = rt;
					}
					break;
				case PT_ICEI:
				case PT_SNOW:
					// Instant evaporate
					sim->part_change_type(rID, x+rx, y+ry, PT_WTRV);
					parts[rID].temp = 500.0f;
					break;
				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
					// Instant boil
					sim->part_change_type(rID, x+rx, y+ry, PT_WTRV);
					parts[rID].temp = 500.0f;
					sim->pv[y/CELL][x/CELL] += 2.0f;
					break;
				case PT_WOOD:
				case PT_PLNT:
				case PT_VINE:
					// Instant burn
					sim->part_change_type(rID, x+rx, y+ry, PT_FIRE);
					parts[rID].life = 100;
					break;
				default:
					break;
				}
			}
		}
	}

	// Spark recharges
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			auto r = pmap[y+ry][x+rx];
			if (r && TYP(r) == PT_SPRK)
			{
				parts[i].tmp = std::min(parts[i].tmp + 20, 200);
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int fuel = cpart->tmp;
	float temp = cpart->temp;

	// White-hot to red depending on temperature
	if (temp > 4000.0f)
	{
		*colr = 255;
		*colg = 255;
		*colb = 255;
	}
	else if (temp > 3000.0f)
	{
		*colr = 255;
		*colg = 200;
		*colb = 100;
	}
	else if (temp > 2000.0f)
	{
		*colr = 255;
		*colg = 100;
		*colb = 0;
	}
	else
	{
		*colr = 255;
		*colg = 68;
		*colb = 0;
	}

	// Intense glow
	int glowIntensity = (int)((temp - 500) / 20);
	if (glowIntensity > 255) glowIntensity = 255;

	*firea = glowIntensity;
	*firer = 255;
	*fireg = 150;
	*fireb = 50;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
