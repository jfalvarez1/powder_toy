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
	DefaultProperties.tmp = 0;   // Direction (like DIOD)
	DefaultProperties.tmp2 = 50; // Breakdown voltage threshold (1-100)
	DefaultProperties.life = 0;  // Conducting state
	HeatConduct = 251;
	Description = "Zener Diode. Conducts forward, and reverse when voltage exceeds threshold. Voltage regulator.";

	Properties = TYPE_SOLID;

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
	int direction = parts[i].tmp % 4;
	int threshold = parts[i].tmp2;
	if (threshold < 1) threshold = 1;
	if (threshold > 100) threshold = 100;
	parts[i].tmp2 = threshold;

	// Direction offsets (same as DIOD)
	int dx = 0, dy = 0;
	int inputDx = 0, inputDy = 0;
	switch (direction)
	{
		case 0: dx = 1; dy = 0; inputDx = -1; inputDy = 0; break;
		case 1: dx = 0; dy = 1; inputDx = 0; inputDy = -1; break;
		case 2: dx = -1; dy = 0; inputDx = 1; inputDy = 0; break;
		case 3: dx = 0; dy = -1; inputDx = 0; inputDy = 1; break;
	}

	int forwardVoltage = 0;
	int reverseVoltage = 0;

	// Count spark sources to estimate voltage
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

				if (rt == PT_SPRK && parts[rID].life >= 3)
				{
					// Check direction
					if ((inputDx != 0 && rx * inputDx > 0) || (inputDy != 0 && ry * inputDy > 0))
					{
						forwardVoltage += 30;
					}
					else if ((dx != 0 && rx * dx > 0) || (dy != 0 && ry * dy > 0))
					{
						reverseVoltage += 30;
					}
				}

				// Battery adds voltage
				if (rt == PT_BTRY)
				{
					if ((inputDx != 0 && rx * inputDx > 0) || (inputDy != 0 && ry * inputDy > 0))
					{
						forwardVoltage += 50;
					}
					else
					{
						reverseVoltage += 50;
					}
				}

				// Capacitor discharge adds voltage
				if (rt == PT_CAPA && parts[rID].tmp > 200)
				{
					reverseVoltage += parts[rID].tmp / 10;
				}
			}
		}
	}

	bool shouldConduct = false;

	// Forward bias - always conduct (like normal diode)
	if (forwardVoltage > 20)
	{
		shouldConduct = true;
	}

	// Reverse bias - conduct only if exceeds threshold (Zener breakdown)
	if (reverseVoltage > threshold)
	{
		shouldConduct = true;
		// Zener regulation: clamp voltage
		reverseVoltage = threshold;
	}

	if (shouldConduct && parts[i].life == 0)
	{
		parts[i].life = 4;

		// Output spark (in appropriate direction)
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

						if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN ||
						     rt == PT_NSCN || rt == PT_RESI)
						    && parts[rID].life == 0)
						{
							// Forward direction output
							if (forwardVoltage > 20 &&
							    ((dx != 0 && rx * dx > 0) || (dy != 0 && ry * dy > 0)))
							{
								sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
								parts[rID].ctype = rt;
								parts[rID].life = 4;
							}
							// Reverse breakdown output (goes backwards)
							if (reverseVoltage >= threshold &&
							    ((inputDx != 0 && rx * inputDx > 0) || (inputDy != 0 && ry * inputDy > 0)))
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

	if (parts[i].life > 0)
	{
		parts[i].life--;
	}

	// Heat when conducting in reverse (power dissipation)
	if (reverseVoltage >= threshold)
	{
		parts[i].temp += (reverseVoltage - threshold) * 0.1f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int threshold = cpart->tmp2;
	int conducting = cpart->life;

	// Dark blue-gray body
	*colr = 74;
	*colg = 74;
	*colb = 106;

	// Threshold indicator (brighter = higher threshold)
	*colb += threshold / 3;

	if (conducting > 0)
	{
		*colr = 120;
		*colg = 120;
		*colb = 160;

		*firea = 40;
		*firer = 100;
		*fireg = 100;
		*fireb = 200;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
