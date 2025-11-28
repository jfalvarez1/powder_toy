#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_ZEND()
{
	Identifier = "DEFAULT_PT_ZEND";
	Name = "ZEND";
	Colour = 0x4A4A6A_rgb;
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
	DefaultProperties.tmp = 50;   // Breakdown voltage threshold (1-100)
	DefaultProperties.tmp2 = 0;   // Conducting state
	DefaultProperties.life = 0;   // Cooldown timer
	HeatConduct = 251;
	Description = "Zener Diode. PSCN=anode, NSCN=cathode. Conducts forward, and reverse above threshold (tmp).";

	Properties = TYPE_SOLID | PROP_LIFE_DEC;

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
	/*
	 * Zener Diode - uses WIRE TYPES:
	 *
	 *   PSCN ---[ZEND]---> NSCN/METL
	 *   (anode)            (cathode)
	 *
	 * Forward: PSCN spark -> outputs to NSCN/METL (like normal diode)
	 * Reverse: NSCN spark -> outputs to PSCN IF voltage > threshold (Zener breakdown)
	 *
	 * tmp = breakdown voltage threshold (1-100)
	 */

	int threshold = parts[i].tmp;
	if (threshold < 1) threshold = 1;
	if (threshold > 100) threshold = 100;
	parts[i].tmp = threshold;

	int forwardVoltage = 0;
	int reverseVoltage = 0;

	// Scan for wire-type inputs
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

				// PSCN spark = forward bias (anode)
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN)
				{
					forwardVoltage = 100;
				}

				// NSCN spark = reverse bias (cathode)
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN)
				{
					reverseVoltage = 100;
				}

				// Check adjacent PSCN/NSCN for nearby sparks
				if (rt == PT_PSCN)
				{
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
									forwardVoltage = 100;
								}
							}
						}
					}
				}

				if (rt == PT_NSCN)
				{
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
									reverseVoltage = 100;
								}
							}
						}
					}
				}

				// Battery/VCCS add voltage
				if (rt == PT_BTRY || rt == PT_VCCS)
				{
					reverseVoltage += 50;
				}
			}
		}
	}

	bool shouldConduct = false;
	bool isReverse = false;

	// Forward bias - always conduct
	if (forwardVoltage > 20)
	{
		shouldConduct = true;
	}

	// Reverse bias - conduct only above threshold (Zener breakdown)
	if (reverseVoltage >= threshold)
	{
		shouldConduct = true;
		isReverse = true;
	}

	parts[i].tmp2 = shouldConduct ? (isReverse ? 2 : 1) : 0;

	if (shouldConduct && parts[i].life == 0)
	{
		parts[i].life = 4;

		// Output based on direction
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

						if (parts[rID].life == 0)
						{
							// Forward: output to NSCN/METL/INWR
							if (!isReverse && (rt == PT_NSCN || rt == PT_METL || rt == PT_INWR))
							{
								sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
								parts[rID].ctype = rt;
								parts[rID].life = 4;
							}
							// Reverse breakdown: output to PSCN/METL/INWR
							if (isReverse && (rt == PT_PSCN || rt == PT_METL || rt == PT_INWR))
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

	// Heat when conducting in reverse
	if (isReverse && shouldConduct)
	{
		if (sim->rng.chance(1, 10))
			parts[i].temp += 0.5f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int threshold = cpart->tmp;
	int conducting = cpart->tmp2;

	// Dark blue-gray body
	*colr = 74;
	*colg = 74;
	*colb = 106;

	// Threshold indicator (brighter = higher threshold)
	*colb += threshold / 3;

	if (conducting == 1)  // Forward
	{
		*colr = 100;
		*colg = 120;
		*colb = 140;

		*firea = 30;
		*firer = 100;
		*fireg = 150;
		*fireb = 200;
		*pixel_mode |= FIRE_ADD;
	}
	else if (conducting == 2)  // Reverse breakdown
	{
		*colr = 140;
		*colg = 100;
		*colb = 160;

		*firea = 50;
		*firer = 180;
		*fireg = 100;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD;
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
