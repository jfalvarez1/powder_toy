#include "simulation/ElementCommon.h"
#include <cmath>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_OSCI()
{
	Identifier = "DEFAULT_PT_OSCI";
	Name = "OSCI";
	Colour = 0x002200_rgb;
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
	DefaultProperties.tmp = 0;    // Row assignment (0=auto from Y pos, 1-10=fixed row)
	DefaultProperties.tmp2 = 0;   // Current brightness level
	DefaultProperties.life = 0;   // History buffer
	HeatConduct = 50;
	Description = "Oscilloscope. Place 10 tall near PROB. Each row lights when signal matches. Tmp=row(0=auto,1-10=fixed).";

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
	int rowSetting = parts[i].tmp;
	if (rowSetting < 0) rowSetting = 0;
	if (rowSetting > 10) rowSetting = 10;
	parts[i].tmp = rowSetting;

	// Find row number for this pixel
	// If rowSetting > 0, use fixed row
	// If rowSetting == 0, auto-detect based on Y position relative to OSCI column
	int myRow = 0;
	int displayHeight = 10;  // Default display height

	if (rowSetting > 0)
	{
		myRow = rowSetting - 1;  // Convert 1-10 to 0-9
	}
	else
	{
		// Auto mode: find lowest OSCI in this column and calculate row
		int lowestY = y;
		int highestY = y;

		// Search for other OSCI in this column
		for (int scanY = std::max(0, y - 20); scanY < std::min(YRES, y + 20); scanY++)
		{
			auto r = pmap[scanY][x];
			if (r && TYP(r) == PT_OSCI)
			{
				if (scanY > lowestY) lowestY = scanY;
				if (scanY < highestY) highestY = scanY;
			}
		}

		displayHeight = lowestY - highestY + 1;
		if (displayHeight < 1) displayHeight = 1;
		if (displayHeight > 20) displayHeight = 20;

		// My row is based on position (bottom = row 0, top = row max)
		myRow = lowestY - y;
		if (myRow < 0) myRow = 0;
		if (myRow >= displayHeight) myRow = displayHeight - 1;
	}

	// Find nearby probes and get their signals
	int probeSignal = 0;
	int probeChannel = 0;
	bool foundProbe = false;

	for (auto rx = -10; rx <= 10; rx++)
	{
		for (auto ry = -5; ry <= 5; ry++)
		{
			if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
				continue;

			auto r = pmap[y+ry][x+rx];
			if (!r) continue;
			auto rt = TYP(r);
			auto rID = ID(r);

			if (rt == PT_PROB)
			{
				probeSignal = parts[rID].tmp3;
				probeChannel = parts[rID].tmp4;
				foundProbe = true;
				break;
			}

			// Also sample directly from SGNL if no probe
			if (!foundProbe && rt == PT_SGNL)
			{
				probeSignal = parts[rID].tmp3;
				probeChannel = 0;
			}
		}
		if (foundProbe) break;
	}

	// Calculate if this row should be lit based on signal level
	// Signal 0-100 maps to rows 0 to (displayHeight-1)
	int signalRow = (probeSignal * displayHeight) / 101;  // 101 to handle 100 properly
	if (signalRow >= displayHeight) signalRow = displayHeight - 1;

	// Light up if we match the signal row (with some tolerance for smoother display)
	int brightness = 0;
	int rowDiff = abs(myRow - signalRow);

	if (rowDiff == 0)
	{
		brightness = 100;  // Direct hit
	}
	else if (rowDiff == 1)
	{
		brightness = 40;   // Adjacent row (for trail effect)
	}

	// Store brightness and channel info
	parts[i].tmp2 = brightness;
	parts[i].tmp3 = probeChannel;
	parts[i].tmp4 = myRow;

	// Shift history for trace effect
	parts[i].life = ((parts[i].life << 2) | (brightness > 50 ? 3 : (brightness > 0 ? 1 : 0))) & 0xFFFF;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int brightness = cpart->tmp2;
	int channel = cpart->tmp3 % 4;
	int history = cpart->life;

	// Dark screen background
	*colr = 0;
	*colg = 20;
	*colb = 0;

	// Channel-based trace color
	int traceR, traceG, traceB;
	switch (channel)
	{
		case 0:  // Yellow trace
			traceR = 255; traceG = 255; traceB = 0;
			break;
		case 1:  // Cyan trace
			traceR = 0; traceG = 255; traceB = 255;
			break;
		case 2:  // Magenta trace
			traceR = 255; traceG = 0; traceB = 255;
			break;
		case 3:  // Green trace
			traceR = 0; traceG = 255; traceB = 0;
			break;
		default:
			traceR = 0; traceG = 255; traceB = 0;
			break;
	}

	// Light up based on brightness
	if (brightness > 0)
	{
		float intensity = brightness / 100.0f;

		// Mix trace color with intensity
		*colr = (int)(traceR * intensity);
		*colg = std::max(20, (int)(traceG * intensity));
		*colb = (int)(traceB * intensity);

		// Phosphor glow effect
		*firea = (int)(100 * intensity);
		*firer = traceR;
		*fireg = traceG;
		*fireb = traceB;
		*pixel_mode |= FIRE_ADD;

		if (brightness > 70)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}
	else
	{
		// Check history for persistence/afterglow
		int recentHits = 0;
		for (int h = 0; h < 8; h++)
		{
			if ((history >> (h * 2)) & 3)
				recentHits++;
		}

		if (recentHits > 0)
		{
			// Fading afterglow
			float afterglow = recentHits / 16.0f;
			*colr = (int)(traceR * afterglow * 0.3f);
			*colg = std::max(20, (int)(traceG * afterglow * 0.3f));
			*colb = (int)(traceB * afterglow * 0.3f);

			if (recentHits > 2)
			{
				*firea = recentHits * 5;
				*firer = traceR / 2;
				*fireg = traceG / 2;
				*fireb = traceB / 2;
				*pixel_mode |= FIRE_ADD;
			}
		}
	}

	return 0;
}
