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
	DefaultProperties.tmp = 10;   // Frequency (1-100)
	DefaultProperties.tmp2 = 0;   // Waveform type
	DefaultProperties.life = 0;   // Phase counter
	HeatConduct = 0;
	Description = "Signal Generator. PSCN=freq+, NSCN=freq-. Tmp=freq(1-100). Tmp2=wave(0-6):SQR,SIN,SAW,PLS,TRI,RMP,RND";

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

	int waveform = parts[i].tmp2 % 7;  // 7 waveform types
	if (waveform < 0) waveform = 0;
	parts[i].tmp2 = waveform;

	// Check for frequency adjustment controls
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r) continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// PSCN spark = increase frequency
				if (rt == PT_SPRK && parts[rID].ctype == PT_PSCN && parts[rID].life == 3)
				{
					if (frequency < 100) frequency++;
				}
				// NSCN spark = decrease frequency
				if (rt == PT_SPRK && parts[rID].ctype == PT_NSCN && parts[rID].life == 3)
				{
					if (frequency > 1) frequency--;
				}
				// METL spark = cycle waveform
				if (rt == PT_SPRK && parts[rID].ctype == PT_METL && parts[rID].life == 3)
				{
					waveform = (waveform + 1) % 7;
					parts[i].tmp2 = waveform;
				}
			}
		}
	}

	parts[i].tmp = frequency;

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

		case 1:  // Sine wave
		{
			float angle = (phase / 1000.0f) * 2.0f * 3.14159f;
			output = (int)(50 + 50 * sin(angle));
			break;
		}

		case 2:  // Sawtooth (ramp up)
			output = phase / 10;
			break;

		case 3:  // Pulse (10% duty cycle)
			output = (phase < 100) ? 100 : 0;
			break;

		case 4:  // Triangle wave
			if (phase < 500)
				output = phase / 5;
			else
				output = (1000 - phase) / 5;
			break;

		case 5:  // Ramp down (inverse sawtooth)
			output = 100 - (phase / 10);
			break;

		case 6:  // Random/noise
			if (sim->rng.chance(frequency, 100))
				output = sim->rng.between(0, 100);
			else
				output = 0;
			break;
	}

	// Store output for probes to read (use tmp3)
	sim->parts[i].tmp3 = output;

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
	int waveform = cpart->tmp2 % 7;
	// int frequency = cpart->tmp;  // Unused but kept for reference

	// Calculate output for display
	int output = 0;
	switch (waveform)
	{
		case 0: output = (phase < 500) ? 100 : 0; break;
		case 1: output = (int)(50 + 50 * sin((phase / 1000.0f) * 2.0f * 3.14159f)); break;
		case 2: output = phase / 10; break;
		case 3: output = (phase < 100) ? 100 : 0; break;
		case 4: output = (phase < 500) ? phase / 5 : (1000 - phase) / 5; break;
		case 5: output = 100 - (phase / 10); break;
		case 6: output = 50; break;  // Random shows as medium
	}

	// Different base colors for each waveform type
	switch (waveform)
	{
		case 0:  // Square - Orange
			*colr = 255; *colg = 100; *colb = 0;
			break;
		case 1:  // Sine - Blue-ish
			*colr = 200; *colg = 100; *colb = 150;
			break;
		case 2:  // Sawtooth - Green-ish
			*colr = 200; *colg = 180; *colb = 0;
			break;
		case 3:  // Pulse - Red
			*colr = 255; *colg = 50; *colb = 50;
			break;
		case 4:  // Triangle - Cyan-ish
			*colr = 150; *colg = 200; *colb = 150;
			break;
		case 5:  // Ramp down - Purple-ish
			*colr = 200; *colg = 100; *colb = 200;
			break;
		case 6:  // Random - White-ish
			*colr = 200; *colg = 200; *colb = 200;
			break;
	}

	// Brightness varies with output
	float intensity = output / 100.0f;
	*firea = (int)(60 * intensity);
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD;

	if (output > 70)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
