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
	DefaultProperties.tmp = 0;   // Base signal detected
	DefaultProperties.tmp2 = 0;  // Output active
	DefaultProperties.life = 0;  // Cooldown timer
	HeatConduct = 251;
	Description = "NPN Transistor. Connect: PSCN=Base, NSCN=Collector, METL/INWR=Emitter. Base spark enables current flow.";

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
	 * NPN Transistor - uses WIRE TYPES to identify terminals:
	 *
	 *   PSCN ----+
	 *   (base)   |
	 *            v
	 *   NSCN ---[TRNS]---> METL/INWR
	 *   (collector)        (emitter output)
	 *
	 * When PSCN has spark (base signal), AND NSCN has spark (collector power),
	 * the transistor outputs spark to any connected METL or INWR (emitter).
	 */

	bool baseSignal = false;      // PSCN sparked = base activated
	bool collectorPower = false;  // NSCN sparked = power available

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

				// Check for sparked PSCN = Base signal
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN)
				{
					baseSignal = true;
				}

				// Check for sparked NSCN = Collector power
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN)
				{
					collectorPower = true;
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
							collectorPower = true;
					}
				}
			}
		}
	}

	parts[i].tmp = baseSignal ? 1 : 0;
	parts[i].tmp2 = 0;

	// Transistor conducts when base is active AND collector has power
	if (baseSignal && collectorPower && parts[i].life == 0)
	{
		parts[i].tmp2 = 1;
		parts[i].life = 4;  // Cooldown to prevent rapid re-triggering

		// Output to METL/INWR (emitter terminals)
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
	int baseActive = cpart->tmp;
	int conducting = cpart->tmp2;

	// Base brown color
	*colr = 128;
	*colg = 80;
	*colb = 64;

	// Bright when conducting
	if (conducting)
	{
		*colr = 200;
		*colg = 140;
		*colb = 90;

		*firea = 50;
		*firer = 255;
		*fireg = 200;
		*fireb = 100;
		*pixel_mode |= FIRE_ADD;
	}
	else if (baseActive)
	{
		// Base active but no power - dim indicator
		*colr = 160;
		*colg = 100;
		*colb = 75;
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
