#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_NRVS()
{
	Identifier = "DEFAULT_PT_NRVS";
	Name = "NRVS";
	Colour = 0xFFFFCC_rgb;
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

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Signal state
	DefaultProperties.tmp2 = 0;  // Refractory period
	HeatConduct = 30;
	Description = "Nerve. Transmits electrical signals very fast! Has refractory period.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_DUST;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Refractory period countdown
	if (parts[i].tmp2 > 0)
	{
		parts[i].tmp2--;
		parts[i].tmp = 0;  // Can't fire during refractory
		return 0;
	}

	bool receivedSignal = false;

	// Check for incoming signals
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

				// Receive signal from spark
				if (rt == PT_SPRK)
				{
					receivedSignal = true;
				}

				// Receive signal from firing nerve
				if (rt == PT_NRVS && parts[rID].tmp == 2)
				{
					receivedSignal = true;
				}
			}
		}
	}

	// Fire if received signal and not in refractory
	if (receivedSignal && parts[i].tmp == 0)
	{
		parts[i].tmp = 1;  // Start firing
	}

	// Signal propagation
	if (parts[i].tmp > 0)
	{
		parts[i].tmp++;

		// Peak of signal
		if (parts[i].tmp == 2)
		{
			// Create spark effect
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

						// Trigger connected muscle!
						if (rt == PT_MUSC && parts[rID].tmp == 0)
						{
							parts[rID].tmp = 1;
							parts[rID].tmp2 = 20;
						}

						// Signal other nerves
						if (rt == PT_NRVS && parts[rID].tmp == 0 && parts[rID].tmp2 == 0)
						{
							// Signal will be picked up next frame
						}

						// Create spark on conductors
						if ((rt == PT_METL || rt == PT_IRON || rt == PT_NSCN || rt == PT_PSCN) &&
						    parts[rID].life == 0)
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
							parts[rID].life = 4;
							parts[rID].ctype = rt;
						}
					}
				}
			}
		}

		// Signal ends, enter refractory period
		if (parts[i].tmp >= 4)
		{
			parts[i].tmp = 0;
			parts[i].tmp2 = 10;  // Refractory period
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int signal = cpart->tmp;
	int refractory = cpart->tmp2;

	// Cream/white nerve color
	*colr = 255;
	*colg = 255;
	*colb = 204;

	if (signal > 0)
	{
		// Bright yellow when firing!
		int intensity = (signal == 2) ? 255 : 150;
		*colr = 255;
		*colg = 255;
		*colb = 0;

		*firea = intensity;
		*firer = 255;
		*fireg = 255;
		*fireb = 100;
		*pixel_mode |= FIRE_ADD | PMODE_GLOW;
	}
	else if (refractory > 0)
	{
		// Darker during refractory
		*colr = 180;
		*colg = 180;
		*colb = 150;
	}

	return 0;
}
