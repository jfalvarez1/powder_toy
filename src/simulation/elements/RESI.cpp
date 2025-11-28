#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_RESI()
{
	Identifier = "DEFAULT_PT_RESI";
	Name = "RESI";
	Colour = 0x8B7355_rgb;
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
	DefaultProperties.tmp = 50;   // Resistance value (1-100, higher = more resistance)
	DefaultProperties.tmp2 = 0;   // Current flowing through
	DefaultProperties.life = 0;   // Spark delay counter
	HeatConduct = 150;  // Good heat conduction to dissipate
	Description = "Resistor. Limits current flow and delays spark propagation. Tmp sets resistance (1-100).";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1500.0f;  // Much more heat resistant
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int resistance = parts[i].tmp;
	if (resistance < 1) resistance = 1;
	if (resistance > 100) resistance = 100;
	parts[i].tmp = resistance;

	// Count down spark delay
	if (parts[i].life > 0)
	{
		parts[i].life--;
		if (parts[i].life == 0)
		{
			// Now output spark
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
							     rt == PT_NSCN || rt == PT_RESI || rt == PT_TRNS)
							    && parts[rID].life == 0)
							{
								// Probability based on inverse resistance
								if (sim->rng.chance(100 - resistance + 10, 110))
								{
									if (rt == PT_RESI || rt == PT_TRNS)
									{
										// Pass current to next resistor/transistor
										parts[rID].tmp2 = parts[i].tmp2 * (100 - resistance) / 100;
										parts[rID].life = parts[rID].tmp / 10 + 1;
									}
									else
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
			parts[i].tmp2 = 0;
		}
	}

	// Check for incoming spark
	bool hasInput = false;
	int inputCurrent = 0;

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

				if (rt == PT_SPRK && parts[rID].life == 3)
				{
					hasInput = true;
					inputCurrent = 100;
				}
				// Accept current from other resistors
				if (rt == PT_RESI && parts[rID].tmp2 > 0 && parts[rID].life > 0)
				{
					hasInput = true;
					inputCurrent = std::max(inputCurrent, parts[rID].tmp2);
				}
			}
		}
	}

	// Start conducting with delay based on resistance
	if (hasInput && parts[i].life == 0)
	{
		parts[i].tmp2 = inputCurrent;
		parts[i].life = resistance / 10 + 1;  // Delay based on resistance
	}

	// Heat dissipation (P = I^2 * R) - but very gradual
	if (parts[i].tmp2 > 0)
	{
		// Only heat up occasionally, not every frame
		if (sim->rng.chance(1, 20))
		{
			float power = (parts[i].tmp2 * resistance) / 50000.0f;
			parts[i].temp += power;
		}
	}

	// Natural cooling toward ambient
	if (parts[i].temp > R_TEMP + 273.15f)
	{
		parts[i].temp -= 0.1f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int resistance = cpart->tmp;
	int current = cpart->tmp2;
	int conducting = cpart->life;

	// Color bands based on resistance value
	// Low resistance = dark, high resistance = light
	int shade = 55 + resistance * 2;
	*colr = shade;
	*colg = shade - 20;
	*colb = shade - 30;

	// Glow when conducting
	if (conducting > 0 && current > 0)
	{
		int brightness = current / 2;
		*firea = brightness;
		*firer = 255;
		*fireg = 200;
		*fireb = 100;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
