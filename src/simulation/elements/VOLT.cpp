#include "simulation/ElementCommon.h"
#include <cmath>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

// 7-segment digit patterns (bits: 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G)
// Segment layout:
//  AAA
// F   B
//  GGG
// E   C
//  DDD
static const unsigned char DIGITS[10] = {
	0b0111111, // 0: A,B,C,D,E,F
	0b0000110, // 1: B,C
	0b1011011, // 2: A,B,D,E,G
	0b1001111, // 3: A,B,C,D,G
	0b1100110, // 4: B,C,F,G
	0b1101101, // 5: A,C,D,F,G
	0b1111101, // 6: A,C,D,E,F,G
	0b0000111, // 7: A,B,C
	0b1111111, // 8: all
	0b1101111, // 9: A,B,C,D,F,G
};

// Check if pixel (px,py) within a 4x7 digit cell should be lit for given digit
static bool isSegmentLit(int px, int py, int digit)
{
	if (digit < 0 || digit > 9) return false;
	unsigned char pattern = DIGITS[digit];

	// Map position to segment (4 wide x 7 tall digit)
	// Row 0: segment A
	if (py == 0 && px >= 1 && px <= 2) return pattern & 0b0000001; // A
	// Row 1-2: segments F and B
	if ((py == 1 || py == 2)) {
		if (px == 0) return pattern & 0b0100000; // F
		if (px == 3) return pattern & 0b0000010; // B
	}
	// Row 3: segment G
	if (py == 3 && px >= 1 && px <= 2) return pattern & 0b1000000; // G
	// Row 4-5: segments E and C
	if ((py == 4 || py == 5)) {
		if (px == 0) return pattern & 0b0010000; // E
		if (px == 3) return pattern & 0b0000100; // C
	}
	// Row 6: segment D
	if (py == 6 && px >= 1 && px <= 2) return pattern & 0b0001000; // D

	return false;
}

// Letter V pattern (4x7)
static bool isLetterV(int px, int py)
{
	if (py <= 4) {
		return (px == 0 || px == 3);
	}
	if (py == 5) {
		return (px == 1 || px == 2);
	}
	if (py == 6) {
		return (px == 1 || px == 2);
	}
	return false;
}

void Element::Element_VOLT()
{
	Identifier = "DEFAULT_PT_VOLT";
	Name = "VOLT";
	Colour = 0x00AA00_rgb;
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
	DefaultProperties.tmp = 0;    // Measured voltage (mV, 0-99999)
	DefaultProperties.tmp2 = 0;   // Display cluster minX
	DefaultProperties.tmp3 = 0;   // Display cluster minY
	DefaultProperties.life = 0;   // Frame counter
	HeatConduct = 50;
	Description = "Voltmeter. 7-segment display. Draw 32x7 for best display. Shows XX.XXX V format.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 600.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int voltage = 0;  // in millivolts

	// Find cluster bounds (only do this periodically to save CPU)
	if (parts[i].life % 10 == 0)
	{
		int minX = x, minY = y;
		// Simple scan to find top-left of cluster
		for (int sx = -40; sx <= 0; sx++)
		{
			for (int sy = -10; sy <= 0; sy++)
			{
				int nx = x + sx;
				int ny = y + sy;
				if (nx >= 0 && ny >= 0 && nx < XRES && ny < YRES)
				{
					auto r = pmap[ny][nx];
					if (r && TYP(r) == PT_VOLT)
					{
						if (ny < minY || (ny == minY && nx < minX))
						{
							minX = nx;
							minY = ny;
						}
					}
				}
			}
		}
		parts[i].tmp2 = minX;
		parts[i].tmp3 = minY;
	}

	// Scan for voltage sources
	for (int rx = -3; rx <= 3; rx++)
	{
		for (int ry = -3; ry <= 3; ry++)
		{
			if (rx || ry)
			{
				int nx = x + rx;
				int ny = y + ry;
				if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES)
					continue;

				auto r = pmap[ny][nx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Measure voltage from VCCS (tmp2 is voltage in units)
				if (rt == PT_VCCS)
				{
					voltage = std::max(voltage, parts[rID].tmp2 * 100);  // Convert to mV
				}

				// Measure from BTRY
				if (rt == PT_BTRY)
				{
					voltage = std::max(voltage, 5000);  // 5.000 V
				}

				// Measure spark intensity
				if (rt == PT_SPRK && parts[rID].life >= 2)
				{
					voltage = std::max(voltage, 3300);  // 3.3V logic level
				}

				// Measure from signal generator
				if (rt == PT_SGNL)
				{
					int sig = parts[rID].tmp3;  // Signal output 0-100
					voltage = std::max(voltage, sig * 50);  // 0-5V
				}

				// Measure from capacitor charge
				if (rt == PT_CAPA)
				{
					int charge = parts[rID].tmp;
					int cap = parts[rID].tmp2;
					if (cap > 0)
						voltage = std::max(voltage, charge * 1000 / cap);
				}

				// Measure from probe
				if (rt == PT_PROB)
				{
					int sig = parts[rID].tmp2;
					voltage = std::max(voltage, sig * 50);
				}
			}
		}
	}

	// Smooth reading
	int current = parts[i].tmp;
	parts[i].tmp = (current * 7 + voltage) / 8;

	parts[i].life++;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int voltage_mv = cpart->tmp;  // millivolts
	int minX = cpart->tmp2;
	int minY = cpart->tmp3;

	// Calculate position within display
	int px = (int)cpart->x - minX;
	int py = (int)cpart->y - minY;

	// Display format: "XX.XXX V"
	// Layout: D0(4) D1(4) .(2) D2(4) D3(4) D4(4) space(2) V(4) = 28 pixels wide
	// Height: 7 pixels

	// Extract digits for XX.XXX format (voltage in mV, display as V)
	// voltage_mv / 1000 = volts, we show XX.XXX
	int displayVal = voltage_mv;  // Already in mV
	if (displayVal > 99999) displayVal = 99999;

	int d0 = (displayVal / 10000) % 10;  // Tens of volts
	int d1 = (displayVal / 1000) % 10;   // Units of volts
	int d2 = (displayVal / 100) % 10;    // Tenths
	int d3 = (displayVal / 10) % 10;     // Hundredths
	int d4 = displayVal % 10;            // Thousandths

	bool lit = false;

	// Digit 0 (tens): px 0-3
	if (px >= 0 && px < 4 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px, py, d0);
	}
	// Digit 1 (units): px 5-8
	else if (px >= 5 && px < 9 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 5, py, d1);
	}
	// Decimal point: px 10, py 5-6
	else if (px >= 10 && px < 12 && py >= 5 && py < 7)
	{
		lit = true;
	}
	// Digit 2 (tenths): px 13-16
	else if (px >= 13 && px < 17 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 13, py, d2);
	}
	// Digit 3 (hundredths): px 18-21
	else if (px >= 18 && px < 22 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 18, py, d3);
	}
	// Digit 4 (thousandths): px 23-26
	else if (px >= 23 && px < 27 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 23, py, d4);
	}
	// Letter V: px 29-32
	else if (px >= 29 && px < 33 && py >= 0 && py < 7)
	{
		lit = isLetterV(px - 29, py);
	}

	// Set colors
	if (lit)
	{
		// Bright green for lit segments
		*colr = 0;
		*colg = 255;
		*colb = 0;
		*firea = 100;
		*firer = 0;
		*fireg = 200;
		*fireb = 0;
		*pixel_mode |= FIRE_ADD;
		*pixel_mode |= PMODE_GLOW;
	}
	else
	{
		// Dark background for display
		*colr = 10;
		*colg = 30;
		*colb = 10;
	}

	return 0;
}
