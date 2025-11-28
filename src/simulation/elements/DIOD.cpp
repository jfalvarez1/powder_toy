#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_DIOD()
{
	Identifier = "DEFAULT_PT_DIOD";
	Name = "DIOD";
	Colour = 0x303030_rgb;
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
	DefaultProperties.tmp = 0;   // Forward current detected
	DefaultProperties.tmp2 = 0;  // Reverse current detected (blocked)
	DefaultProperties.life = 0;  // Cooldown timer
	HeatConduct = 251;
	Description = "Diode. Current flows PSCN->DIOD->NSCN only. Blocks reverse flow.";

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
	 * Diode - uses WIRE TYPES for direction:
	 *
	 *   PSCN ---[DIOD]---> NSCN/METL
	 *   (anode)            (cathode)
	 *
	 * Current flows from PSCN (anode) to NSCN/METL (cathode).
	 * Reverse direction (NSCN to PSCN) is blocked.
	 */

	bool forwardInput = false;    // PSCN sparked = forward bias
	bool reverseInput = false;    // NSCN sparked trying to go backwards

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

				// Sparked PSCN = forward input (anode)
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN)
				{
					forwardInput = true;
				}

				// Sparked NSCN = reverse input (blocked)
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN)
				{
					reverseInput = true;
				}

				// Sparked METL adjacent to PSCN also counts as forward
				if (rt == PT_SPRK && parts[rID].ctype == PT_METL)
				{
					// Check if there's a PSCN nearby this spark
					for (int dx = -1; dx <= 1; dx++)
					{
						for (int dy = -1; dy <= 1; dy++)
						{
							int nx = x + rx + dx;
							int ny = y + ry + dy;
							if (nx >= 0 && nx < XRES && ny >= 0 && ny < YRES)
							{
								auto r2 = pmap[ny][nx];
								if (r2 && TYP(r2) == PT_PSCN)
								{
									forwardInput = true;
								}
							}
						}
					}
				}

				// Also check adjacent PSCN for nearby sparks
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
									forwardInput = true;
								}
							}
						}
					}
				}
			}
		}
	}

	parts[i].tmp = forwardInput ? 1 : 0;
	parts[i].tmp2 = reverseInput ? 1 : 0;

	// Forward bias - conduct to NSCN/METL/INWR
	if (forwardInput && parts[i].life == 0)
	{
		parts[i].life = 4;  // Cooldown

		// Output to NSCN, METL, INWR (cathode side)
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

						// Output to NSCN, METL, or INWR
						if ((rt == PT_NSCN || rt == PT_METL || rt == PT_INWR)
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

	// Reverse bias - heat up slightly (blocking)
	if (reverseInput)
	{
		if (sim->rng.chance(1, 20))
			parts[i].temp += 0.1f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int forward = cpart->tmp;
	int reverse = cpart->tmp2;
	int conducting = cpart->life;

	// Dark body
	*colr = 48;
	*colg = 48;
	*colb = 48;

	// Conducting glow (forward)
	if (conducting > 0 && forward)
	{
		*colr = 120;
		*colg = 120;
		*colb = 100;

		*firea = 40;
		*firer = 200;
		*fireg = 200;
		*fireb = 150;
		*pixel_mode |= FIRE_ADD;
	}

	// Red warning when blocking reverse
	if (reverse)
	{
		*colr = 100;
		*colg = 40;
		*colb = 40;
	}

	return 0;
}
