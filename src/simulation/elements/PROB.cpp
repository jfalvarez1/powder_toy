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
	Description = "Probe. Place NEXT TO signal source or connect via wire. Tmp=channel(0-3). Use PROP tool to set channel.";

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
	parts[i].tmp = channel;

	int directSignal = 0;
	bool hasDirectSource = false;

	// Scan nearby area for direct signal sources
	for (int rx = -5; rx <= 5; rx++)
	{
		for (int ry = -5; ry <= 5; ry++)
		{
			if (rx == 0 && ry == 0)
				continue;

			int nx = x + rx;
			int ny = y + ry;
			if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES)
				continue;

			auto r = pmap[ny][nx];
			if (!r)
				continue;
			auto rt = TYP(r);
			auto rID = ID(r);

			// Sample from SGNL (signal generator)
			if (rt == PT_SGNL)
			{
				directSignal = std::max(directSignal, parts[rID].tmp3);
				hasDirectSource = true;
			}

			// Sample from VCCS (power supply voltage)
			if (rt == PT_VCCS)
			{
				directSignal = std::max(directSignal, parts[rID].tmp2 / 5);
				hasDirectSource = true;
			}

			// Sample sparks on wires (so wire connections work)
			if (rt == PT_SPRK)
			{
				directSignal = std::max(directSignal, 80);
				hasDirectSource = true;
			}

			// Sample from capacitor charge
			if (rt == PT_CAPA)
			{
				int charge = parts[rID].tmp;
				int cap = parts[rID].tmp2;
				if (cap > 0)
				{
					directSignal = std::max(directSignal, charge * 100 / (cap * 10));
					hasDirectSource = true;
				}
			}

			// Sample from voltmeter reading
			if (rt == PT_VOLT)
			{
				directSignal = std::max(directSignal, parts[rID].tmp / 3);
				hasDirectSource = true;
			}

			// Sample from ammeter reading
			if (rt == PT_AMPR)
			{
				directSignal = std::max(directSignal, parts[rID].tmp / 3);
				hasDirectSource = true;
			}
		}
	}

	int sampleValue = 0;

	if (hasDirectSource)
	{
		// Use direct signal - don't read from neighbors to avoid feedback
		sampleValue = directSignal;
	}
	else
	{
		// No direct source - propagate from neighboring probes
		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				if (rx == 0 && ry == 0)
					continue;

				int nx = x + rx;
				int ny = y + ry;
				if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES)
					continue;

				auto r = pmap[ny][nx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				if (rt == PT_PROB)
				{
					int neighborSignal = parts[rID].tmp2;
					sampleValue = std::max(sampleValue, neighborSignal);
				}
			}
		}
	}

	// Clamp sample value
	if (sampleValue > 100) sampleValue = 100;
	if (sampleValue < 0) sampleValue = 0;

	parts[i].tmp2 = sampleValue;

	// Store for oscilloscope: tmp3 = sample, tmp4 = channel
	parts[i].tmp3 = sampleValue;
	parts[i].tmp4 = channel;

	// Visual history
	parts[i].life = ((parts[i].life << 4) | (sampleValue / 7)) & 0xFFFFFF;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int channel = cpart->tmp % 4;
	int sample = cpart->tmp2;

	// Channel colors
	switch (channel)
	{
		case 0:  // Yellow
			*colr = 255; *colg = 255; *colb = 0;
			break;
		case 1:  // Cyan
			*colr = 0; *colg = 255; *colb = 255;
			break;
		case 2:  // Magenta
			*colr = 255; *colg = 0; *colb = 255;
			break;
		case 3:  // Green
			*colr = 0; *colg = 255; *colb = 0;
			break;
	}

	// Dim when no signal, bright when active
	if (sample < 10)
	{
		*colr = *colr / 4;
		*colg = *colg / 4;
		*colb = *colb / 4;
	}
	else
	{
		float intensity = sample / 100.0f;
		*firea = (int)(100 * intensity);
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;

		if (sample > 50)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}

	return 0;
}
