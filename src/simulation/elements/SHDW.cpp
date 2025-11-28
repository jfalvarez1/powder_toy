#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SHDW()
{
	Identifier = "DEFAULT_PT_SHDW";
	Name = "SHDW";
	Colour = 0x101010_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.05f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 0;

	DefaultProperties.temp = R_TEMP - 50.0f + 273.15f;  // Cold
	DefaultProperties.tmp = 100;  // Darkness strength
	HeatConduct = 0;  // No heat transfer
	Description = "Shadow. Living darkness that consumes light and spreads in dark areas.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1000.0f;
	HighTemperatureTransition = PT_NONE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	bool nearLight = false;
	int lightCount = 0;
	int darkCount = 0;

	// Check surroundings for light/darkness
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				auto r = pmap[y+ry][x+rx];

				if (!r)
				{
					darkCount++;  // Empty space = dark
					continue;
				}

				auto rt = TYP(r);
				auto rID = ID(r);

				// Light sources damage shadow
				switch (rt)
				{
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
				case PT_SPRK:
				case PT_LIGH:
				case PT_PHOT:
				case PT_NEUT:
					lightCount++;
					nearLight = true;
					parts[i].tmp -= 5;

					// Try to extinguish nearby light
					if (parts[i].tmp > 50 && sim->rng.chance(1, 20))
					{
						if (rt == PT_FIRE || rt == PT_PLSM)
						{
							sim->kill_part(rID);
							parts[i].tmp -= 10;
						}
					}
					break;
				case PT_GLOW:
					// Glow is deadly to shadow
					parts[i].tmp -= 20;
					nearLight = true;
					lightCount += 5;
					break;
				case PT_SHDW:
					// Shadows strengthen each other
					darkCount += 2;
					if (parts[rID].tmp < parts[i].tmp - 20)
					{
						parts[i].tmp -= 10;
						parts[rID].tmp += 10;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Shadow weakens in light, strengthens in darkness
	if (nearLight && lightCount > 3)
	{
		parts[i].tmp -= lightCount;
	}
	else if (darkCount > 10)
	{
		// Grow stronger in darkness
		parts[i].tmp = std::min(parts[i].tmp + 1, 200);
	}

	// Die if too weak
	if (parts[i].tmp <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Spread to dark areas
	if (parts[i].tmp > 80 && !nearLight)
	{
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if ((rx || ry) && sim->rng.chance(1, 100))
				{
					if (!pmap[y+ry][x+rx])
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_SHDW);
						if (np >= 0)
						{
							parts[np].tmp = 30;
							parts[i].tmp -= 20;
						}
						goto done_spread;
					}
				}
			}
		}
		done_spread:;
	}

	// Move toward darkness, away from light
	if (nearLight)
	{
		// Find darkest direction and move there
		int bestX = 0, bestY = 0;
		int maxDark = 0;

		for (int dx = -3; dx <= 3; dx++)
		{
			for (int dy = -3; dy <= 3; dy++)
			{
				if (dx == 0 && dy == 0)
					continue;
				if (x + dx < 0 || x + dx >= XRES || y + dy < 0 || y + dy >= YRES)
					continue;

				if (!pmap[y+dy][x+dx])
				{
					// Count darkness around this spot
					int dark = 0;
					for (int ddx = -1; ddx <= 1; ddx++)
					{
						for (int ddy = -1; ddy <= 1; ddy++)
						{
							if (!pmap[y+dy+ddy][x+dx+ddx])
								dark++;
						}
					}
					if (dark > maxDark)
					{
						maxDark = dark;
						bestX = dx;
						bestY = dy;
					}
				}
			}
		}

		if (maxDark > 0 && sim->rng.chance(1, 5))
		{
			parts[i].x = x + bestX;
			parts[i].y = y + bestY;
		}
	}

	// Absorb heat from surroundings (shadow is cold)
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			auto r = pmap[y+ry][x+rx];
			if (r && TYP(r) != PT_SHDW)
			{
				int rID = ID(r);
				if (parts[rID].temp > parts[i].temp + 50)
				{
					parts[rID].temp -= 1.0f;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int strength = cpart->tmp;

	// Pure black, darker with more strength
	int darkness = 16 - strength / 15;
	if (darkness < 0) darkness = 0;

	*colr = darkness;
	*colg = darkness;
	*colb = darkness + 5;  // Slight blue tint

	// Strong shadows have an anti-glow effect (absorb light)
	if (strength > 50)
	{
		*firea = -(strength / 2);  // Negative fire = absorbs
		*firer = 0;
		*fireg = 0;
		*fireb = 0;
		*pixel_mode |= PMODE_BLEND;
		*cola = 200 + strength / 4;
	}

	return 0;
}
