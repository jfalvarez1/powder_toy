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
	DefaultProperties.tmp = 0;    // Time position in trace (auto-assigned by column position)
	DefaultProperties.tmp2 = 0;   // Current signal level at this time position
	DefaultProperties.life = 0;   // Display intensity
	DefaultProperties.tmp3 = 0;   // Channel color
	DefaultProperties.tmp4 = 0;   // Row position (for vertical amplitude)
	HeatConduct = 50;
	Description = "Oscilloscope. Draw horizontal row for time trace, or grid for full display. Place PROB nearby.";

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
	// Find the extent of OSCI display (both horizontal and vertical)
	int leftX = x, rightX = x;
	int topY = y, bottomY = y;

	// Scan for connected OSCI to find display bounds
	for (int scanX = std::max(0, x - 50); scanX < std::min(XRES, x + 50); scanX++)
	{
		auto r = pmap[y][scanX];
		if (r && TYP(r) == PT_OSCI)
		{
			if (scanX < leftX) leftX = scanX;
			if (scanX > rightX) rightX = scanX;
		}
	}
	for (int scanY = std::max(0, y - 30); scanY < std::min(YRES, y + 30); scanY++)
	{
		auto r = pmap[scanY][x];
		if (r && TYP(r) == PT_OSCI)
		{
			if (scanY < topY) topY = scanY;
			if (scanY > bottomY) bottomY = scanY;
		}
	}

	int displayWidth = rightX - leftX + 1;
	int displayHeight = bottomY - topY + 1;

	// My position in the display
	int myTimePos = x - leftX;  // 0 = left (oldest), max = right (newest)
	int myAmplitudePos = bottomY - y;  // 0 = bottom, max = top

	// Find nearby probe
	int probeSignal = 0;
	int probeChannel = 0;
	bool foundProbe = false;

	for (int scanX = leftX - 10; scanX <= rightX + 10 && !foundProbe; scanX++)
	{
		for (int scanY = topY - 10; scanY <= bottomY + 10; scanY++)
		{
			if (scanX < 0 || scanX >= XRES || scanY < 0 || scanY >= YRES)
				continue;

			auto r = pmap[scanY][scanX];
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

			if (!foundProbe && rt == PT_SGNL)
			{
				probeSignal = parts[rID].tmp3;
				probeChannel = 0;
			}
		}
	}

	// For a 1D horizontal trace (single row), just show current signal as brightness
	if (displayHeight == 1)
	{
		// Shift register behavior: rightmost gets current signal, others shift left
		if (myTimePos == displayWidth - 1)
		{
			// I'm the rightmost - get current signal
			parts[i].tmp2 = probeSignal;
		}
		else
		{
			// Get signal from neighbor to the right
			auto rRight = pmap[y][x + 1];
			if (rRight && TYP(rRight) == PT_OSCI)
			{
				parts[i].tmp2 = parts[ID(rRight)].tmp2;
			}
		}

		parts[i].life = parts[i].tmp2;  // Brightness = signal level
		parts[i].tmp3 = probeChannel;
	}
	else
	{
		// 2D grid display - show waveform with vertical amplitude
		// The rightmost column gets the current signal level
		// Other columns shift the pattern left

		int signalForThisColumn = 0;

		if (myTimePos == displayWidth - 1)
		{
			// Rightmost column - use current probe signal
			signalForThisColumn = probeSignal;
		}
		else
		{
			// Look at the OSCI at same row, one column to the right
			auto rRight = pmap[y][x + 1];
			if (rRight && TYP(rRight) == PT_OSCI)
			{
				// Get what signal level that column was displaying
				signalForThisColumn = parts[ID(rRight)].tmp2;
			}
		}

		parts[i].tmp2 = signalForThisColumn;

		// Calculate if this row should light up based on signal level
		// Signal 0-100 maps to rows 0 (bottom) to displayHeight-1 (top)
		int signalRow = (signalForThisColumn * (displayHeight - 1)) / 100;
		if (signalRow < 0) signalRow = 0;
		if (signalRow >= displayHeight) signalRow = displayHeight - 1;

		int rowDiff = abs(myAmplitudePos - signalRow);

		int brightness = 0;
		if (rowDiff == 0)
		{
			brightness = 100;
		}
		else if (rowDiff == 1)
		{
			brightness = 40;
		}

		parts[i].life = brightness;
		parts[i].tmp3 = probeChannel;
		parts[i].tmp4 = myAmplitudePos;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int signalLevel = cpart->tmp2;
	int brightness = cpart->life;
	int channel = cpart->tmp3 % 4;

	// Dark screen background
	*colr = 0;
	*colg = 12;
	*colb = 0;

	// Channel colors
	int traceR, traceG, traceB;
	switch (channel)
	{
		case 0:  // Green (classic)
			traceR = 30; traceG = 255; traceB = 30;
			break;
		case 1:  // Cyan
			traceR = 0; traceG = 255; traceB = 255;
			break;
		case 2:  // Yellow
			traceR = 255; traceG = 255; traceB = 0;
			break;
		case 3:  // Magenta
			traceR = 255; traceG = 30; traceB = 255;
			break;
		default:
			traceR = 30; traceG = 255; traceB = 30;
			break;
	}

	if (brightness > 0)
	{
		float intensity = brightness / 100.0f;

		*colr = (int)(traceR * intensity);
		*colg = std::max(12, (int)(traceG * intensity));
		*colb = (int)(traceB * intensity);

		*firea = (int)(70 * intensity);
		*firer = traceR;
		*fireg = traceG;
		*fireb = traceB;
		*pixel_mode |= FIRE_ADD;

		if (brightness > 50)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}
	else if (signalLevel > 0)
	{
		// For 1D trace mode - dim trace based on signal level
		float intensity = signalLevel / 200.0f;  // Dimmer
		*colg = std::max(12, (int)(80 * intensity));
	}

	return 0;
}
