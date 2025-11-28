#include "simulation/ElementCommon.h"
#include <cmath>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SGNL()
{
	Identifier = "DEFAULT_PT_SGNL";
	Name = "SGNL";
	Colour = 0xFF6600_rgb;
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
	DefaultProperties.tmp = 10;   // Frequency (1-100, higher = faster oscillation)
	DefaultProperties.tmp2 = 0;   // Waveform: 0=square, 1=sine, 2=sawtooth, 3=pulse
	DefaultProperties.life = 0;   // Phase counter
	HeatConduct = 0;
	Description = "Signal generator. Outputs oscillating AC signal. Tmp=frequency, Tmp2=waveform.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int frequency = parts[i].tmp;
	if (frequency < 1) frequency = 1;
	if (frequency > 100) frequency = 100;
	parts[i].tmp = frequency;

	int waveform = parts[i].tmp2 % 4;
	parts[i].tmp2 = waveform;

	// Increment phase
	parts[i].life += frequency;
	if (parts[i].life > 1000)
		parts[i].life -= 1000;

	// Calculate output level based on waveform
	int phase = parts[i].life;
	int output = 0;

	switch (waveform)
	{
		case 0:  // Square wave
			output = (phase < 500) ? 100 : 0;
			break;

		case 1:  // Sine wave (approximated)
		{
			float angle = (phase / 1000.0f) * 2.0f * 3.14159f;
			output = (int)(50 + 50 * sin(angle));
			break;
		}

		case 2:  // Sawtooth
			output = phase / 10;
			break;

		case 3:  // Pulse (short duty cycle)
			output = (phase < 100) ? 100 : 0;
			break;
	}

	// Output spark when output is high
	if (output > 50)
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

						if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN ||
						     rt == PT_NSCN || rt == PT_RESI || rt == PT_CAPA ||
						     rt == PT_TRNS || rt == PT_PTRN)
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

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int phase = cpart->life;
	int waveform = cpart->tmp2 % 4;
	int frequency = cpart->tmp;

	// Orange base
	*colr = 255;
	*colg = 102;
	*colb = 0;

	// Pulse effect based on phase
	int output = 0;
	switch (waveform)
	{
		case 0:
			output = (phase < 500) ? 100 : 0;
			break;
		case 1:
		{
			float angle = (phase / 1000.0f) * 2.0f * 3.14159f;
			output = (int)(50 + 50 * sin(angle));
			break;
		}
		case 2:
			output = phase / 10;
			break;
		case 3:
			output = (phase < 100) ? 100 : 0;
			break;
	}

	// Brightness varies with output
	if (output > 50)
	{
		*firea = output / 2;
		*firer = 255;
		*fireg = 150;
		*fireb = 0;
		*pixel_mode |= FIRE_ADD;
	}

	// Waveform indicator color tint
	switch (waveform)
	{
		case 0: break;  // Square - no tint
		case 1: *colb = 50; break;  // Sine - slight blue
		case 2: *colg = 150; break;  // Sawtooth - more green
		case 3: *colr = 200; *colg = 50; break;  // Pulse - more red
	}

	return 0;
}
