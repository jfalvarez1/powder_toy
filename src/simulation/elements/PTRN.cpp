#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PTRN()
{
	Identifier = "DEFAULT_PT_PTRN";
	Name = "PTRN";
	Colour = 0x504080_rgb;
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
	DefaultProperties.tmp = 0;   // Base current level
	DefaultProperties.tmp2 = 0;  // Emitter current level
	DefaultProperties.life = 0;  // Active state timer
	HeatConduct = 251;
	Description = "PNP Transistor. Complementary to NPN. Base LOW allows emitter-collector current.";

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

static int update(UPDATE_FUNC_ARGS)
{
	int baseInput = 0;
	int emitterInput = 0;
	bool hasVCC = false;

	// PNP: Conducts when base is LOW relative to emitter
	// Current flows from Emitter to Collector when base is pulled low

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

				// Detect VCC/power on emitter (top)
				if (rt == PT_VCCS || (rt == PT_BTRY))
				{
					hasVCC = true;
					emitterInput = 100;
				}

				// Spark detection
				if (rt == PT_SPRK)
				{
					int srcType = parts[rID].ctype;
					// Base input (PSCN or left side)
					if (srcType == PT_PSCN || rx < 0)
					{
						baseInput = std::max(baseInput, 50 + parts[rID].life * 5);
					}
					// Emitter input (top)
					if (ry < 0)
					{
						emitterInput = std::max(emitterInput, 50 + parts[rID].life * 5);
					}
				}

				// BTRY provides constant power
				if (rt == PT_BTRY && ry < 0)
				{
					emitterInput = 100;
				}
			}
		}
	}

	parts[i].tmp = baseInput;

	// PNP logic: conducts when base is LOW (inverted from NPN)
	// If base has no signal but emitter has power -> conduct
	int gain = 20;
	int outputCurrent = 0;

	if (baseInput < 30 && emitterInput > 30)  // Base LOW, Emitter has power
	{
		// PNP is "on"
		outputCurrent = std::min((100 - baseInput) * gain / 10, 100);
		parts[i].tmp2 = outputCurrent;
		parts[i].life = 4;

		// Output to collector (bottom/right)
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

						if ((rt == PT_NSCN || rt == PT_METL || rt == PT_INWR || rt == PT_PSCN)
						    && parts[rID].life == 0 && (ry > 0 || rx > 0))
						{
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
		parts[i].tmp2 = 0;
	}

	// Heat with current
	if (outputCurrent > 50)
	{
		parts[i].temp += outputCurrent * 0.01f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int baseCurrent = cpart->tmp;
	int outputCurrent = cpart->tmp2;
	int active = cpart->life;

	// Base purple/blue color (to distinguish from NPN)
	*colr = 80;
	*colg = 64;
	*colb = 128;

	if (active > 0)
	{
		int brightness = outputCurrent / 2;
		*colr = std::min(80 + brightness / 2, 150);
		*colg = std::min(64 + brightness / 2, 150);
		*colb = std::min(128 + brightness, 255);

		*firea = brightness;
		*firer = 150;
		*fireg = 150;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD;
	}

	if (baseCurrent < 30 && active > 0)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
