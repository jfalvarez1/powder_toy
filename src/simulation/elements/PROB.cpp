#include "simulation/ElementCommon.h"
#include <cmath>

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
	DefaultProperties.tmp = 0;    // Channel ID (0-3): Yellow, Cyan, Magenta, Green
	DefaultProperties.tmp2 = 0;   // Current sample value (0-100)
	DefaultProperties.life = 0;   // Sample history
	HeatConduct = 50;
	Description = "Probe. Samples signals for OSCI. Tmp=channel(0-3). PSCN=ch+, NSCN=ch-. Colors: Yel,Cyn,Mag,Grn";

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
	int channel = parts[i].tmp;
	if (channel < 0) channel = 0;
	if (channel > 3) channel = 3;

	int sampleValue = 0;

	// Scan for channel control and signals
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

				// Channel control: PSCN spark = next channel, NSCN = prev channel
				if (rt == PT_SPRK && parts[rID].life == 3)
				{
					if (parts[rID].ctype == PT_PSCN)
					{
						channel = (channel + 1) % 4;
					}
					else if (parts[rID].ctype == PT_NSCN)
					{
						channel = (channel + 3) % 4;  // +3 mod 4 = -1
					}
				}

				// Sample from SGNL (signal generator) - read tmp3
				if (rt == PT_SGNL)
				{
					sampleValue = std::max(sampleValue, parts[rID].tmp3);
				}

				// Sample from VCCS (voltage)
				if (rt == PT_VCCS)
				{
					sampleValue = std::max(sampleValue, parts[rID].tmp2 / 5);
				}

				// Sample spark presence
				if (rt == PT_SPRK)
				{
					sampleValue = std::max(sampleValue, parts[rID].life * 20);
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
					sampleValue = std::max(sampleValue, parts[rID].tmp / 5);
				}

				// Sample from ammeter
				if (rt == PT_AMPR)
				{
					sampleValue = std::max(sampleValue, parts[rID].tmp / 5);
				}
			}
		}
	}

	parts[i].tmp = channel;

	// Clamp sample value
	if (sampleValue > 100) sampleValue = 100;
	if (sampleValue < 0) sampleValue = 0;

	parts[i].tmp2 = sampleValue;

	// Store sample for oscilloscope - use tmp3 for sample, tmp4 for channel
	parts[i].tmp3 = sampleValue;
	parts[i].tmp4 = channel;

	// Shift history (6 samples packed into life)
	parts[i].life = ((parts[i].life << 4) | (sampleValue / 7)) & 0xFFFFFF;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int channel = cpart->tmp % 4;
	int sample = cpart->tmp2;

	// Distinct colors for each channel
	switch (channel)
	{
		case 0:  // Yellow
			*colr = 255;
			*colg = 255;
			*colb = 0;
			break;
		case 1:  // Cyan
			*colr = 0;
			*colg = 255;
			*colb = 255;
			break;
		case 2:  // Magenta
			*colr = 255;
			*colg = 0;
			*colb = 255;
			break;
		case 3:  // Green
			*colr = 0;
			*colg = 255;
			*colb = 0;
			break;
	}

	// Brightness based on sample (dim when no signal)
	if (sample < 10)
	{
		*colr = *colr / 3;
		*colg = *colg / 3;
		*colb = *colb / 3;
	}
	else
	{
		float intensity = sample / 100.0f;
		*firea = (int)(80 * intensity);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;
	}

	// Strong glow when active
	if (sample > 50)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
