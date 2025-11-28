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
	DefaultProperties.tmp = 0;   // Base signal (inverted logic)
	DefaultProperties.tmp2 = 0;  // Output active
	DefaultProperties.life = 0;  // Cooldown timer
	HeatConduct = 251;
	Description = "PNP Transistor. Connect: PSCN=Base, NSCN=Emitter, METL=Collector. NO base spark enables current.";

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
	 * PNP Transistor - uses WIRE TYPES (inverted from NPN):
	 *
	 *   PSCN ----+
	 *   (base)   |  <- When NOT sparked, transistor is ON
	 *            v
	 *   NSCN ---[PTRN]---> METL/INWR
	 *   (emitter)          (collector output)
	 *
	 * When PSCN has NO spark (base inactive), AND NSCN has spark (emitter power),
	 * the transistor outputs spark to any connected METL or INWR (collector).
	 */

	bool baseSignal = false;      // PSCN sparked = base HIGH (transistor OFF)
	bool emitterPower = false;    // NSCN sparked = emitter has power
	bool hasPSCN = false;         // Track if PSCN is connected

	// Scan adjacent particles for wire types
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

				// Check for PSCN connection
				if (rt == PT_PSCN)
				{
					hasPSCN = true;
				}

				// Check for sparked PSCN = Base HIGH (blocks)
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN)
				{
					baseSignal = true;
				}

				// Check for sparked NSCN = Emitter power
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN)
				{
					emitterPower = true;
				}

				// Also check if adjacent PSCN/NSCN have nearby sparks
				if (rt == PT_PSCN || rt == PT_NSCN)
				{
					bool hasSpark = false;
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
									hasSpark = true;
								}
							}
						}
					}
					if (hasSpark)
					{
						if (rt == PT_PSCN)
							baseSignal = true;
						else
							emitterPower = true;
					}
				}
			}
		}
	}

	parts[i].tmp = baseSignal ? 1 : 0;
	parts[i].tmp2 = 0;

	// PNP conducts when base is LOW (no signal) AND emitter has power
	// If no PSCN connected, treat as base LOW (always on when powered)
	bool baseOff = !baseSignal || !hasPSCN;

	if (baseOff && emitterPower && parts[i].life == 0)
	{
		parts[i].tmp2 = 1;
		parts[i].life = 4;  // Cooldown

		// Output to METL/INWR (collector terminals)
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
						if ((rt == PT_METL || rt == PT_INWR) && parts[rID].life == 0)
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

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int baseHigh = cpart->tmp;
	int conducting = cpart->tmp2;

	// Base purple/blue color
	*colr = 80;
	*colg = 64;
	*colb = 128;

	// Bright when conducting
	if (conducting)
	{
		*colr = 120;
		*colg = 100;
		*colb = 200;

		*firea = 50;
		*firer = 150;
		*fireg = 150;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD;
	}
	else if (!baseHigh)
	{
		// Base off (ready to conduct) - subtle glow
		*colr = 90;
		*colg = 75;
		*colb = 150;
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
