#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_TRNS()
{
	Identifier = "DEFAULT_PT_TRNS";
	Name = "TRNS";
	Colour = 0x805040_rgb;
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
	DefaultProperties.tmp = 0;   // Base current level (0-100)
	DefaultProperties.tmp2 = 0;  // Collector current level
	DefaultProperties.life = 0;  // Active state timer
	HeatConduct = 251;
	Description = "NPN Transistor. Base current controls collector-emitter current. For amplifier circuits.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 500.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

// Helper to find connected component types
static int findConnectedType(Simulation *sim, int x, int y, int targetType, int excludeX, int excludeY)
{
	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
		{
			if ((rx || ry) && (x + rx != excludeX || y + ry != excludeY))
			{
				if (x + rx >= 0 && x + rx < XRES && y + ry >= 0 && y + ry < YRES)
				{
					auto r = sim->pmap[y+ry][x+rx];
					if (r && TYP(r) == targetType)
						return ID(r);
				}
			}
		}
	}
	return -1;
}

static int update(UPDATE_FUNC_ARGS)
{
	int baseInput = 0;
	int collectorInput = 0;
	bool emitterGround = false;

	// Scan for inputs - NPN: Base controls C->E flow
	// Look for spark inputs and categorize by position
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
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Base input (left side or PSCN) - controls the transistor
				if (rt == PT_SPRK)
				{
					// Check what the spark is on
					int srcType = parts[rID].ctype;
					if (srcType == PT_PSCN || rx < 0)
					{
						baseInput = std::max(baseInput, 50 + parts[rID].life * 5);
					}
					else if (srcType == PT_NSCN || rx > 0)
					{
						collectorInput = std::max(collectorInput, 50 + parts[rID].life * 5);
					}
					else
					{
						// Generic spark - use position
						if (ry < 0)
							collectorInput = std::max(collectorInput, 50);
						else if (rx < 0)
							baseInput = std::max(baseInput, 50);
					}
				}

				// Ground detection (emitter side, bottom)
				if (rt == PT_GRND || (rt == PT_METL && ry > 0))
				{
					emitterGround = true;
				}

				// Direct PSCN = base, NSCN = collector
				if (rt == PT_PSCN && parts[rID].life == 0)
				{
					// Check if PSCN has spark nearby
					for (int dx = -1; dx <= 1; dx++)
					{
						for (int dy = -1; dy <= 1; dy++)
						{
							int nx = x + rx + dx;
							int ny = y + ry + dy;
							if (nx >= 0 && nx < XRES && ny >= 0 && ny < YRES)
							{
								auto r2 = pmap[ny][nx];
								if (r2 && TYP(r2) == PT_SPRK)
								{
									baseInput = std::max(baseInput, 60);
								}
							}
						}
					}
				}
			}
		}
	}

	// Store base current level
	parts[i].tmp = baseInput;

	// Transistor amplification: collector current = base current * gain (beta ~= 100)
	// In TPT terms: if base has signal, allow/amplify collector to emitter flow
	int gain = 20;  // Amplification factor
	int outputCurrent = 0;

	if (baseInput > 20)  // Threshold for turn-on
	{
		// Transistor is "on" - amplify
		outputCurrent = std::min(baseInput * gain / 10, 100);
		parts[i].tmp2 = outputCurrent;
		parts[i].life = 4;  // Stay active

		// Output spark to emitter side (bottom/right/NSCN)
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

						// Output to NSCN, METL on right/bottom side
						if ((rt == PT_NSCN || rt == PT_METL || rt == PT_INWR || rt == PT_PSCN)
						    && parts[rID].life == 0 && (ry > 0 || rx > 0))
						{
							// Create spark proportional to output
							if (sim->rng.chance(outputCurrent, 100))
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
	else
	{
		parts[i].tmp2 = 0;  // Off
	}

	// Heat increases with current flow
	if (outputCurrent > 50)
	{
		parts[i].temp += outputCurrent * 0.01f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int baseCurrent = cpart->tmp;
	int collectorCurrent = cpart->tmp2;
	int active = cpart->life;

	// Base brown color
	*colr = 128;
	*colg = 80;
	*colb = 64;

	// Brighter when conducting
	if (active > 0)
	{
		int brightness = collectorCurrent / 2;
		*colr = std::min(128 + brightness, 255);
		*colg = std::min(80 + brightness, 200);
		*colb = 64;

		// Glow effect
		*firea = brightness;
		*firer = 255;
		*fireg = 200;
		*fireb = 100;
		*pixel_mode |= FIRE_ADD;
	}

	// Base indicator (left side glow when base is active)
	if (baseCurrent > 20)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
