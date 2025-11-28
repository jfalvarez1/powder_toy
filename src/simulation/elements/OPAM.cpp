#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_OPAM()
{
	Identifier = "DEFAULT_PT_OPAM";
	Name = "OPAM";
	Colour = 0x2F2F2F_rgb;
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
	DefaultProperties.tmp = 0;    // (+) input level
	DefaultProperties.tmp2 = 0;   // (-) input level
	DefaultProperties.life = 0;   // Output state
	HeatConduct = 100;
	Description = "Op-Amp. Amplifies difference between inputs. PSCN=(+), NSCN=(-), output to METL/INWR.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 400.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	/*
	 * Op-Amp - uses WIRE TYPES for terminals:
	 *
	 *   PSCN ---[+]
	 *              \
	 *               [OPAM]---> METL/INWR (output)
	 *              /
	 *   NSCN ---[-]
	 *
	 * PSCN spark = (+) non-inverting input
	 * NSCN spark = (-) inverting input
	 * Output = amplified difference (V+ - V-)
	 * Outputs to METL/INWR when (V+ > V-)
	 */

	int plusInput = 0;   // Non-inverting input
	int minusInput = 0;  // Inverting input
	bool hasPower = false;

	// Scan for inputs using wire types
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

				// Power supply detection
				if (rt == PT_VCCS || rt == PT_BTRY)
				{
					hasPower = true;
				}

				// Sparked PSCN = (+) non-inverting input
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN)
				{
					plusInput = 100;
				}

				// Sparked NSCN = (-) inverting input
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN)
				{
					minusInput = 100;
				}

				// Check PSCN with nearby spark
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
									plusInput = std::max(plusInput, 100);
								}
							}
						}
					}
				}

				// Check NSCN with nearby spark
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
									minusInput = std::max(minusInput, 100);
								}
							}
						}
					}
				}
			}
		}
	}

	parts[i].tmp = plusInput;
	parts[i].tmp2 = minusInput;

	// Op-amp behavior: output = gain * (V+ - V-)
	// With very high gain, output saturates to rail
	int difference = plusInput - minusInput;
	int gain = 100;  // Very high gain (ideal op-amp)
	int output = difference * gain / 10;

	// Clamp to rails (0 to 100)
	if (output > 100) output = 100;
	if (output < 0) output = 0;

	// Only output if powered or has input
	if (hasPower || (plusInput > 0 || minusInput > 0))
	{
		parts[i].life = output;

		// Output to METL/INWR when output is positive
		if (output > 30)
		{
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

							// Output only to METL/INWR (neutral outputs)
							if ((rt == PT_METL || rt == PT_INWR)
							    && parts[rID].life == 0)
							{
								if (sim->rng.chance(output, 100))
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
	}
	else
	{
		parts[i].life = 0;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int plusIn = cpart->tmp;
	int minusIn = cpart->tmp2;
	int output = cpart->life;

	// Dark gray chip
	*colr = 47;
	*colg = 47;
	*colb = 47;

	// Show activity
	if (output > 0)
	{
		int brightness = output / 2;
		*colr = std::min(47 + brightness, 150);
		*colg = std::min(47 + brightness, 150);
		*colb = std::min(47 + brightness / 2, 100);

		*firea = brightness / 2;
		*firer = 200;
		*fireg = 200;
		*fireb = 100;
		*pixel_mode |= FIRE_ADD;
	}

	// Input indicators
	if (plusIn > 50)
	{
		*colg += 30;  // Green tint for (+)
	}
	if (minusIn > 50)
	{
		*colr += 30;  // Red tint for (-)
	}

	return 0;
}
