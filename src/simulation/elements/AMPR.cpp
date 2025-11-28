#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_AMPR()
{
	Identifier = "DEFAULT_PT_AMPR";
	Name = "AMPR";
	Colour = 0x0066CC_rgb;
	MenuVisible = 1;
	MenuSection = SC_ELEC;
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
	Hardness = 1;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;    // Current reading (sparks per 100 frames)
	DefaultProperties.tmp2 = 0;   // Spark counter for current window
	DefaultProperties.life = 0;   // Frame counter
	HeatConduct = 50;
	Description = "Ammeter. Measures current flow (sparks/sec). Conducts electricity. Blue=low, Cyan=med, White=high.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 600.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int sparkCount = 0;

	// Count sparks passing through or nearby
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

				// Count sparks
				if (rt == PT_SPRK && parts[rID].life == 3)
				{
					sparkCount++;
				}
			}
		}
	}

	// Add to running count
	parts[i].tmp2 += sparkCount;
	parts[i].life++;

	// Every 50 frames, update reading
	if (parts[i].life >= 50)
	{
		// Current = sparks per window, scaled
		parts[i].tmp = parts[i].tmp2 * 2;  // Scale to reasonable display value
		if (parts[i].tmp > 500) parts[i].tmp = 500;

		// Reset for next window
		parts[i].tmp2 = 0;
		parts[i].life = 0;
	}

	// Ammeter conducts - pass current through
	// Check if we should spark adjacent wires
	bool hasSpark = false;
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (r && TYP(r) == PT_SPRK)
				{
					hasSpark = true;
					break;
				}
			}
		}
		if (hasSpark) break;
	}

	// Pass spark to other side
	if (hasSpark)
	{
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (r)
					{
						auto rt = TYP(r);
						auto rID = ID(r);

						if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN || rt == PT_NSCN)
						    && parts[rID].life == 0)
						{
							if (sim->rng.chance(1, 2))  // 50% chance to pass
							{
								sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
								parts[rID].ctype = rt;
								parts[rID].life = 4;
							}
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
	int current = cpart->tmp;

	// Color based on current level:
	// Dark Blue (0-20), Blue (20-100), Cyan (100-250), White (250+)
	if (current < 20)
	{
		*colr = 0;
		*colg = 50 + current * 2;
		*colb = 150 + current * 2;
	}
	else if (current < 100)
	{
		*colr = 0;
		*colg = 100 + current;
		*colb = 200;
	}
	else if (current < 250)
	{
		*colr = (current - 100);
		*colg = 200 + (current - 100) / 3;
		*colb = 255;
	}
	else
	{
		*colr = 200;
		*colg = 255;
		*colb = 255;
	}

	// Glow when reading current
	if (current > 0)
	{
		*firea = std::min(current / 4, 60);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;
	}

	// Bright glow for high current
	if (current > 100)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
