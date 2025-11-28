#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_XTLG()
{
	Identifier = "DEFAULT_PT_XTLG";
	Name = "XTLG";
	Colour = 0xFF00FF_rgb;
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
	Meltable = 1;
	Hardness = 5;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Growth stage
	HeatConduct = 150;
	Description = "Living Crystal. Grows by consuming minerals and glass. Changes color as it grows.";

	Properties = TYPE_SOLID | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 50.0f;  // Shatters under high pressure
	HighPressureTransition = PT_PQRT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2500.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// tmp stores growth level (0-100)
	// tmp2 stores color hue (cycles through spectrum)

	// Slowly cycle color
	if (sim->rng.chance(1, 50))
	{
		parts[i].tmp2 = (parts[i].tmp2 + 1) % 360;
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
					// Grow into empty space if we have enough energy
					if (parts[i].tmp >= 50 && sim->rng.chance(1, 500))
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_XTLG);
						if (np >= 0)
						{
							parts[np].tmp = 0;  // New crystal starts small
							parts[np].tmp2 = parts[i].tmp2;  // Inherit color
							parts[np].temp = parts[i].temp;
							parts[i].tmp -= 25;  // Cost energy to grow
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				// Consume minerals to grow
				switch (rt)
				{
				case PT_SAND:
				case PT_STNE:
				case PT_BRCK:
				case PT_CNCT:
					// Basic minerals - slow growth
					if (sim->rng.chance(1, 200))
					{
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 5, 100);
					}
					break;
				case PT_GLAS:
				case PT_BGLA:
					// Glass - medium growth
					if (sim->rng.chance(1, 100))
					{
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 10, 100);
					}
					break;
				case PT_QRTZ:
				case PT_PQRT:
					// Quartz - fast growth
					if (sim->rng.chance(1, 50))
					{
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 20, 100);
					}
					break;
				case PT_DMND:
					// Diamond - super growth!
					if (sim->rng.chance(1, 25))
					{
						sim->kill_part(rID);
						parts[i].tmp = 100;
						// Also spread color influence
						parts[i].tmp2 = (parts[i].tmp2 + 60) % 360;
					}
					break;
				case PT_CLST:
					// Clay stone
					if (sim->rng.chance(1, 150))
					{
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 8, 100);
					}
					break;
				case PT_SALT:
					// Salt crystals help growth
					if (sim->rng.chance(1, 75))
					{
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 15, 100);
					}
					break;
				case PT_XTLG:
					// Transfer growth between crystals
					if (parts[i].tmp > parts[rID].tmp + 20)
					{
						if (sim->rng.chance(1, 50))
						{
							int transfer = (parts[i].tmp - parts[rID].tmp) / 4;
							parts[i].tmp -= transfer;
							parts[rID].tmp += transfer;
						}
					}
					// Harmonize colors slowly
					if (sim->rng.chance(1, 100))
					{
						int diff = parts[rID].tmp2 - parts[i].tmp2;
						if (diff > 180) diff -= 360;
						if (diff < -180) diff += 360;
						parts[i].tmp2 = (parts[i].tmp2 + diff / 10 + 360) % 360;
					}
					break;
				case PT_SPRK:
					// Electricity makes crystal glow brighter
					parts[i].tmp2 = (parts[i].tmp2 + 10) % 360;
					break;
				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
					// Water slowly dissolves crystal
					if (sim->rng.chance(1, 2000))
					{
						parts[i].tmp = std::max(parts[i].tmp - 1, 0);
						if (parts[i].tmp <= 0 && sim->rng.chance(1, 10))
						{
							sim->kill_part(i);
							return 1;
						}
					}
					break;
				case PT_ACID:
					// Acid destroys crystal
					if (sim->rng.chance(1, 50))
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

	// Mature crystals emit faint light
	if (parts[i].tmp >= 80 && sim->rng.chance(1, 200))
	{
		int np = sim->create_part(-1, x, y, PT_PHOT);
		if (np >= 0)
		{
			// Color based on tmp2 (hue)
			int hue = parts[i].tmp2;
			int ctype = 0;
			if (hue < 60)
				ctype = 0x00FF0000;  // Red
			else if (hue < 120)
				ctype = 0x00FFFF00;  // Yellow
			else if (hue < 180)
				ctype = 0x0000FF00;  // Green
			else if (hue < 240)
				ctype = 0x0000FFFF;  // Cyan
			else if (hue < 300)
				ctype = 0x000000FF;  // Blue
			else
				ctype = 0x00FF00FF;  // Magenta

			parts[np].ctype = ctype;
			parts[np].vx = sim->rng.between(-3, 3) * 0.5f;
			parts[np].vy = sim->rng.between(-3, 3) * 0.5f;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Color based on hue (tmp2) and growth level (tmp)
	int hue = cpart->tmp2;
	int growth = cpart->tmp;

	// Convert HSV to RGB (simplified)
	float h = hue / 60.0f;
	int hi = (int)h % 6;
	float f = h - (int)h;

	int v = 150 + growth;  // Value increases with growth
	if (v > 255) v = 255;

	int p = v * 0.3f;
	int q = v * (1 - f * 0.7f);
	int t = v * (0.3f + f * 0.7f);

	switch (hi)
	{
	case 0: *colr = v; *colg = t; *colb = p; break;
	case 1: *colr = q; *colg = v; *colb = p; break;
	case 2: *colr = p; *colg = v; *colb = t; break;
	case 3: *colr = p; *colg = q; *colb = v; break;
	case 4: *colr = t; *colg = p; *colb = v; break;
	case 5: *colr = v; *colg = p; *colb = q; break;
	}

	// Glow effect for grown crystals
	if (growth >= 50)
	{
		*firea = growth / 2;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;
	}

	*pixel_mode |= PMODE_GLOW;

	return 0;
}
