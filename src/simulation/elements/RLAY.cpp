#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_RLAY()
{
	Identifier = "DEFAULT_PT_RLAY";
	Name = "RLAY";
	Colour = 0x8B4513_rgb;
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
	DefaultProperties.tmp = 0;    // Control coil state
	DefaultProperties.tmp2 = 0;   // Switch state (0=open, 1=closed)
	DefaultProperties.life = 0;   // Switch delay counter
	HeatConduct = 80;
	Description = "Relay. PSCN=control coil, NSCN=signal in, METL/INWR=signal out. Passes signal when coil energized.";

	Properties = TYPE_SOLID;

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
	/*
	 * Relay - uses WIRE TYPES for terminals:
	 *
	 *   PSCN ----+
	 *   (coil)   |
	 *            v
	 *   NSCN ---[RLAY]---> METL/INWR
	 *   (signal in)        (signal out)
	 *
	 * PSCN spark = energize coil (control)
	 * NSCN spark = signal input (when coil energized, passes to output)
	 * Output to METL/INWR when coil is energized AND signal present
	 */

	bool coilEnergized = false;
	bool hasSignalInput = false;

	// Check for control (coil) and signal inputs using wire types
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

				// Sparked PSCN = coil control
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN)
				{
					coilEnergized = true;
				}

				// Sparked NSCN = signal input
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN)
				{
					hasSignalInput = true;
				}

				// Check PSCN with nearby spark (coil control)
				if (rt == PT_PSCN)
				{
					for (int ddx = -1; ddx <= 1; ddx++)
					{
						for (int ddy = -1; ddy <= 1; ddy++)
						{
							int nx = x + rx + ddx;
							int ny = y + ry + ddy;
							if (nx >= 0 && nx < XRES && ny >= 0 && ny < YRES)
							{
								auto r2 = pmap[ny][nx];
								if (r2 && TYP(r2) == PT_SPRK)
								{
									coilEnergized = true;
								}
							}
						}
					}
				}

				// Check NSCN with nearby spark (signal input)
				if (rt == PT_NSCN)
				{
					for (int ddx = -1; ddx <= 1; ddx++)
					{
						for (int ddy = -1; ddy <= 1; ddy++)
						{
							int nx = x + rx + ddx;
							int ny = y + ry + ddy;
							if (nx >= 0 && nx < XRES && ny >= 0 && ny < YRES)
							{
								auto r2 = pmap[ny][nx];
								if (r2 && TYP(r2) == PT_SPRK)
								{
									hasSignalInput = true;
								}
							}
						}
					}
				}
			}
		}
	}

	parts[i].tmp = coilEnergized ? 100 : 0;

	// Relay switching with delay (mechanical lag)
	if (coilEnergized && parts[i].tmp2 == 0)
	{
		// Closing delay
		parts[i].life++;
		if (parts[i].life > 3)  // Takes a few frames to close
		{
			parts[i].tmp2 = 1;  // Close switch
			parts[i].life = 0;
		}
	}
	else if (!coilEnergized && parts[i].tmp2 == 1)
	{
		// Opening delay
		parts[i].life++;
		if (parts[i].life > 3)
		{
			parts[i].tmp2 = 0;  // Open switch
			parts[i].life = 0;
		}
	}
	else
	{
		parts[i].life = 0;
	}

	// Pass signal through if switch is closed
	if (parts[i].tmp2 == 1 && hasSignalInput)
	{
		// Output to METL/INWR (neutral output conductors)
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

						// Output only to METL or INWR
						if ((rt == PT_METL || rt == PT_INWR)
						    && parts[rID].life == 0)
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

	// Coil heats up when energized
	if (coilEnergized)
	{
		if (sim->rng.chance(1, 10))
			parts[i].temp += 0.1f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int coilState = cpart->tmp;
	int switchState = cpart->tmp2;
	int delay = cpart->life;

	// Brown base (coil color)
	*colr = 139;
	*colg = 69;
	*colb = 19;

	// Coil energized indicator
	if (coilState > 0)
	{
		*colr = 180;
		*colg = 100;
		*colb = 50;

		*firea = 20;
		*firer = 200;
		*fireg = 150;
		*fireb = 50;
		*pixel_mode |= FIRE_ADD;
	}

	// Switch closed indicator (brighter)
	if (switchState == 1)
	{
		*colr = std::min(*colr + 50, 255);
		*colg = std::min(*colg + 50, 200);
		*colb = std::min(*colb + 30, 150);
		*pixel_mode |= PMODE_GLOW;
	}

	// Switching animation
	if (delay > 0)
	{
		*firea += 10;
	}

	return 0;
}
