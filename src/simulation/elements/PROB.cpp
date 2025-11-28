#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PROB()
{
	Identifier = "DEFAULT_PT_PROB";
	Name = "PROB";
	Colour = 0xFFFF00_rgb;
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
	DefaultProperties.tmp = 0;    // Channel ID (0-3) for oscilloscope
	DefaultProperties.tmp2 = 0;   // Current sample value (0-255)
	DefaultProperties.life = 0;   // Sample history encoded in pavg (visual)
	HeatConduct = 50;
	Description = "Oscilloscope Probe. Place near circuit to sample signals. Tmp=channel (0-3). Connect to OSCI.";

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
	// Clamp channel ID
	int channel = parts[i].tmp % 4;
	if (channel < 0) channel = 0;
	parts[i].tmp = channel;

	int sampleValue = 0;

	// Sample nearby electrical activity
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

				// Sample from VCCS (voltage)
				if (rt == PT_VCCS)
				{
					sampleValue = std::max(sampleValue, (int)parts[rID].tmp2 / 2);
				}

				// Sample spark presence
				if (rt == PT_SPRK)
				{
					sampleValue = std::max(sampleValue, parts[rID].life * 25);
				}

				// Sample from signal generator
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
					sampleValue = std::max(sampleValue, output);
				}

				// Sample from capacitor (charge level)
				if (rt == PT_CAPA)
				{
					int charge = parts[rID].tmp;
					int cap = parts[rID].tmp2;
					if (cap > 0)
						sampleValue = std::max(sampleValue, charge * 100 / (cap * 10));
				}

				// Sample from voltmeter
				if (rt == PT_VOLT)
				{
					sampleValue = std::max(sampleValue, (int)parts[rID].tmp / 2);
				}

				// Sample from ammeter
				if (rt == PT_AMPR)
				{
					sampleValue = std::max(sampleValue, (int)parts[rID].tmp / 3);
				}
			}
		}
	}

	// Clamp sample value
	if (sampleValue > 255) sampleValue = 255;
	if (sampleValue < 0) sampleValue = 0;

	parts[i].tmp2 = sampleValue;

	// Shift history and add new sample (store in pavg for persistence)
	// pavg[0] stores recent history as bit-packed values
	// We'll use life to store a simplified waveform pattern
	parts[i].life = ((parts[i].life << 4) | (sampleValue / 16)) & 0xFFFFFF;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int channel = cpart->tmp;
	int sample = cpart->tmp2;

	// Channel colors: 0=Yellow, 1=Cyan, 2=Magenta, 3=Green
	switch (channel)
	{
		case 0:
			*colr = 255;
			*colg = 255;
			*colb = 0;
			break;
		case 1:
			*colr = 0;
			*colg = 255;
			*colb = 255;
			break;
		case 2:
			*colr = 255;
			*colg = 0;
			*colb = 255;
			break;
		case 3:
			*colr = 0;
			*colg = 255;
			*colb = 0;
			break;
	}

	// Brightness based on sample
	if (sample > 0)
	{
		float intensity = sample / 255.0f;
		*firea = (int)(50 * intensity);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;
	}

	// Pulsing effect when active
	if (sample > 50)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
