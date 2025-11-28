#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_POTM()
{
	Identifier = "DEFAULT_PT_POTM";
	Name = "POTM";
	Colour = 0x666699_rgb;
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
	DefaultProperties.tmp = 50;   // Wiper position (0-100)
	DefaultProperties.tmp2 = 0;   // Current flow indicator
	DefaultProperties.life = 0;   // Last adjustment direction
	HeatConduct = 50;
	Description = "Potentiometer. Variable resistor. Tmp sets position (0-100). Adjust with SPRK.";

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
	int position = parts[i].tmp;
	if (position < 0) position = 0;
	if (position > 100) position = 100;

	bool hasInput = false;
	int inputStrength = 0;
	bool adjustUp = false;
	bool adjustDown = false;

	// Check for inputs and adjustment signals
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

				// Spark input
				if (rt == PT_SPRK && parts[rID].life >= 3)
				{
					hasInput = true;
					inputStrength = 100;
				}

				// PSCN spark = increase position
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
								if (r2 && TYP(r2) == PT_SPRK && parts[ID(r2)].life == 3)
								{
									adjustUp = true;
								}
							}
						}
					}
				}

				// NSCN spark = decrease position
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
								if (r2 && TYP(r2) == PT_SPRK && parts[ID(r2)].life == 3)
								{
									adjustDown = true;
								}
							}
						}
					}
				}

				// Temperature-based adjustment (for thermistor-like behavior)
				if (parts[i].temp > 350.0f)
				{
					// Hot = lower resistance
					if (position < 100 && sim->rng.chance(1, 50))
						position++;
				}
				else if (parts[i].temp < 270.0f)
				{
					// Cold = higher resistance
					if (position > 0 && sim->rng.chance(1, 50))
						position--;
				}
			}
		}
	}

	// Adjust position based on control signals
	if (adjustUp && position < 100)
	{
		position++;
		parts[i].life = 1;
	}
	if (adjustDown && position > 0)
	{
		position--;
		parts[i].life = -1;
	}

	parts[i].tmp = position;

	// Pass current with attenuation based on position
	// Position 0 = full resistance (block), Position 100 = no resistance (pass)
	if (hasInput)
	{
		int outputStrength = inputStrength * position / 100;
		parts[i].tmp2 = outputStrength;

		if (outputStrength > 20)
		{
			// Output spark with probability based on position
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

							if ((rt == PT_METL || rt == PT_INWR || rt == PT_RESI ||
							     rt == PT_TRNS || rt == PT_PTRN)
							    && parts[rID].life == 0)
							{
								if (sim->rng.chance(outputStrength, 100))
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

		// Heat based on power dissipation
		float power = (100 - position) * inputStrength / 10000.0f;
		parts[i].temp += power;
	}
	else
	{
		parts[i].tmp2 = 0;
	}

	// Decay adjustment indicator
	if (parts[i].life > 0) parts[i].life--;
	if (parts[i].life < 0) parts[i].life++;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int position = cpart->tmp;
	int current = cpart->tmp2;

	// Blue-gray base
	*colr = 102;
	*colg = 102;
	*colb = 153;

	// Brightness based on position (wiper indicator)
	int posBright = position / 2;
	*colr = std::min(*colr + posBright, 200);
	*colg = std::min(*colg + posBright, 200);

	// Glow when current is flowing
	if (current > 0)
	{
		*firea = current / 3;
		*firer = 150;
		*fireg = 150;
		*fireb = 200;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
