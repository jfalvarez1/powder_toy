#include "simulation/ElementCommon.h"
#include <cmath>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_OSCI()
{
	Identifier = "DEFAULT_PT_OSCI";
	Name = "OSCI";
	Colour = 0x001100_rgb;
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
	DefaultProperties.tmp = 0;    // Waveform history (24-bit, 6 samples of 4 bits each)
	DefaultProperties.tmp2 = 0;   // Current display value
	DefaultProperties.life = 0;   // Frame counter for animation
	HeatConduct = 50;
	Description = "Oscilloscope display. Place near PROB to show waveforms. Shows signal history as color pattern.";

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
	// Find nearby probes and get their waveform data
	int probeData[4] = {0, 0, 0, 0};  // Up to 4 channels
	bool hasProbe = false;

	for (auto rx = -5; rx <= 5; rx++)
	{
		for (auto ry = -5; ry <= 5; ry++)
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

				// Get data from probe
				if (rt == PT_PROB)
				{
					int channel = parts[rID].tmp % 4;
					int value = parts[rID].tmp2;
					probeData[channel] = std::max(probeData[channel], value);
					hasProbe = true;
				}

				// Also sample directly from nearby electronics if no probe
				if (!hasProbe)
				{
					if (rt == PT_SPRK)
					{
						probeData[0] = std::max(probeData[0], parts[rID].life * 30);
					}
					if (rt == PT_VCCS)
					{
						probeData[1] = std::max(probeData[1], (int)parts[rID].tmp2 / 2);
					}
					if (rt == PT_SGNL)
					{
						int phase = parts[rID].life;
						int waveform = parts[rID].tmp2 % 4;
						int output = 0;
						switch (waveform)
						{
							case 0: output = (phase < 500) ? 100 : 0; break;
							case 1: output = (int)(50 + 50 * sin(phase / 1000.0f * 2 * 3.14159f)); break;
							case 2: output = phase / 10; break;
							case 3: output = (phase < 100) ? 100 : 0; break;
						}
						probeData[2] = std::max(probeData[2], output);
					}
				}
			}
		}
	}

	// Combine probe data into display value
	// Use different bits for different channels
	int combined = 0;
	combined |= (probeData[0] / 16) & 0xF;         // Ch0 in bits 0-3
	combined |= ((probeData[1] / 16) & 0xF) << 4;  // Ch1 in bits 4-7
	combined |= ((probeData[2] / 16) & 0xF) << 8;  // Ch2 in bits 8-11
	combined |= ((probeData[3] / 16) & 0xF) << 12; // Ch3 in bits 12-15

	// Shift waveform history
	parts[i].tmp = ((parts[i].tmp << 4) | ((combined & 0xF0) >> 4)) & 0xFFFFFF;
	parts[i].tmp2 = combined;

	// Increment frame counter for animation
	parts[i].life = (parts[i].life + 1) % 60;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int combined = cpart->tmp2;
	int history = cpart->tmp;
	int frame = cpart->life;

	// Extract channel values
	int ch0 = (combined & 0xF) * 16;
	int ch1 = ((combined >> 4) & 0xF) * 16;
	int ch2 = ((combined >> 8) & 0xF) * 16;
	int ch3 = ((combined >> 12) & 0xF) * 16;

	// Dark green screen background
	*colr = 0;
	*colg = 17 + frame / 6;  // Slight flicker
	*colb = 0;

	// Mix colors based on active channels
	// Ch0 = Yellow, Ch1 = Cyan, Ch2 = Magenta, Ch3 = Green
	int totalSignal = ch0 + ch1 + ch2 + ch3;

	if (totalSignal > 0)
	{
		// Calculate mixed color from all channels
		int mixR = (ch0 * 255 + ch2 * 255) / 512;
		int mixG = (ch0 * 255 + ch1 * 255 + ch3 * 255) / 768;
		int mixB = (ch1 * 255 + ch2 * 255) / 512;

		// Add to base color
		*colr = std::min(mixR, 255);
		*colg = std::min(17 + mixG, 255);
		*colb = std::min(mixB, 255);

		// Phosphor glow effect
		float intensity = std::min(totalSignal / 400.0f, 1.0f);
		*firea = (int)(60 * intensity);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;

		// Scan line effect based on history
		int histBit = (history >> ((frame / 10) * 4)) & 0xF;
		if (histBit > 8)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}

	// Trace line visualization - show waveform pattern
	// Extract 6 samples from history (4 bits each)
	int samples[6];
	for (int s = 0; s < 6; s++)
	{
		samples[s] = (history >> (s * 4)) & 0xF;
	}

	// Create visual pattern based on waveform
	int waveHeight = samples[frame / 10] * 16;
	if (waveHeight > 0)
	{
		*colg = std::min(*colg + waveHeight / 2, 255);
		*firea += waveHeight / 4;
		*fireg = std::min(*fireg + waveHeight, 255);
	}

	return 0;
}
