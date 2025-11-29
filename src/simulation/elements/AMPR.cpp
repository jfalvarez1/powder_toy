#include "simulation/ElementCommon.h"
#include <cmath>
#include <queue>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

// Flood-fill to propagate spark through all connected AMPR and spark edge conductors
static void propagateSparkThroughCluster(Simulation *sim, int startX, int startY)
{
	bool visited[YRES][XRES] = {false};
	std::queue<std::pair<int,int>> toVisit;
	toVisit.push({startX, startY});
	visited[startY][startX] = true;

	while (!toVisit.empty())
	{
		auto [cx, cy] = toVisit.front();
		toVisit.pop();

		// Check all 8 neighbors
		for (int dx = -1; dx <= 1; dx++)
		{
			for (int dy = -1; dy <= 1; dy++)
			{
				if (dx == 0 && dy == 0) continue;
				int nx = cx + dx;
				int ny = cy + dy;
				if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES) continue;

				auto r = sim->pmap[ny][nx];
				if (!r) continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// If it's another AMPR, add to queue for flood-fill
				if (rt == PT_AMPR && !visited[ny][nx])
				{
					visited[ny][nx] = true;
					toVisit.push({nx, ny});
				}
				// If it's a conductor, spark it!
				else if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN || rt == PT_NSCN ||
				          rt == PT_IRON || rt == PT_BMTL || rt == PT_TUNG) && sim->parts[rID].life == 0)
				{
					sim->part_change_type(rID, nx, ny, PT_SPRK);
					sim->parts[rID].ctype = rt;
					sim->parts[rID].life = 4;
				}
			}
		}
	}
}

// 7-segment digit patterns (bits: 0=A, 1=B, 2=C, 3=D, 4=E, 5=F, 6=G)
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
	if (py == 0 && px >= 1 && px <= 2) return pattern & 0b0000001; // A
	if ((py == 1 || py == 2)) {
		if (px == 0) return pattern & 0b0100000; // F
		if (px == 3) return pattern & 0b0000010; // B
	}
	if (py == 3 && px >= 1 && px <= 2) return pattern & 0b1000000; // G
	if ((py == 4 || py == 5)) {
		if (px == 0) return pattern & 0b0010000; // E
		if (px == 3) return pattern & 0b0000100; // C
	}
	if (py == 6 && px >= 1 && px <= 2) return pattern & 0b0001000; // D

	return false;
}

// Letter m pattern (4x7) - lowercase
static bool isLetterM(int px, int py)
{
	if (py >= 2 && py <= 6) {
		return (px == 0 || px == 2 || px == 3);
	}
	if (py == 1) {
		return (px == 1);
	}
	return false;
}

// Letter A pattern (4x7)
static bool isLetterA(int px, int py)
{
	if (py == 0 && px >= 1 && px <= 2) return true;
	if ((py >= 1 && py <= 2) && (px == 0 || px == 3)) return true;
	if (py == 3 && px >= 0 && px <= 3) return true;
	if ((py >= 4 && py <= 6) && (px == 0 || px == 3)) return true;
	return false;
}

void Element::Element_AMPR()
{
	Identifier = "DEFAULT_PT_AMPR";
	Name = "AMPR";
	Colour = 0x0066CC_rgb;
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
	DefaultProperties.tmp = 0;    // Current reading (microamps, 0-99999)
	DefaultProperties.tmp2 = 0;   // Display cluster minX
	DefaultProperties.tmp3 = 0;   // Display cluster minY
	DefaultProperties.tmp4 = 0;   // Spark counter for current window
	DefaultProperties.life = 0;   // Frame counter
	HeatConduct = 50;
	Description = "Ammeter. 7-segment display. Draw 35x7 for best display. Shows XX.XXX mA. Conducts electricity.";

	Properties = TYPE_SOLID;  // Don't use PROP_CONDUCTS - we handle spark passing manually

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
	// Find cluster bounds (only do this periodically)
	if (parts[i].life % 10 == 0)
	{
		int minX = x, minY = y;
		for (int sx = -45; sx <= 0; sx++)
		{
			for (int sy = -10; sy <= 0; sy++)
			{
				int nx = x + sx;
				int ny = y + sy;
				if (nx >= 0 && ny >= 0 && nx < XRES && ny < YRES)
				{
					auto r = pmap[ny][nx];
					if (r && TYP(r) == PT_AMPR)
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

	parts[i].life++;

	// Check for sparks or recently-sparked conductors adjacent to this AMPR
	// We need to detect both active sparks (PT_SPRK) and conductors in refractory period
	bool foundSpark = false;
	int sparkCountThisFrame = 0;

	for (int rx = -1; rx <= 1; rx++)
	{
		for (int ry = -1; ry <= 1; ry++)
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

				// Check for active spark (PT_SPRK with any life value)
				if (rt == PT_SPRK && parts[rID].life >= 1)
				{
					int ctype = parts[rID].ctype;
					// Only count sparks on wire conductors
					if (ctype == PT_METL || ctype == PT_INWR || ctype == PT_PSCN ||
					    ctype == PT_NSCN || ctype == PT_IRON || ctype == PT_BMTL || ctype == PT_TUNG)
					{
						foundSpark = true;
						// Count ALL sparks for current measurement (not just fresh ones)
						sparkCountThisFrame++;
					}
				}
				// Also check for conductor in refractory period (just finished sparking)
				// This catches sparks we might have missed due to update order
				else if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN ||
				          rt == PT_NSCN || rt == PT_IRON || rt == PT_BMTL || rt == PT_TUNG)
				         && parts[rID].life > 0 && parts[rID].life <= 4)
				{
					// This conductor was recently sparked - count it too!
					foundSpark = true;
					sparkCountThisFrame++;
				}
			}
		}
	}

	// Count sparks for current measurement
	if (sparkCountThisFrame > 0)
	{
		parts[i].tmp4 += sparkCountThisFrame;
	}

	// If we found a spark, use flood-fill to propagate through entire cluster instantly
	if (foundSpark)
	{
		propagateSparkThroughCluster(sim, x, y);
	}

	// Every 10 frames, update reading
	if (parts[i].life >= 10)
	{
		// Current in microamps = sparks * scale factor
		// Divide by more to account for multiple detections per spark
		int current_ua = parts[i].tmp4 * 100;  // 100 uA (0.1mA) per spark detection
		if (current_ua > 99999) current_ua = 99999;

		// Smooth the reading
		parts[i].tmp = (parts[i].tmp * 2 + current_ua) / 3;

		// Reset for next window
		parts[i].tmp4 = 0;
		parts[i].life = 0;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int current_ua = cpart->tmp;  // microamps
	int minX = cpart->tmp2;
	int minY = cpart->tmp3;

	// Calculate position within display
	int px = (int)cpart->x - minX;
	int py = (int)cpart->y - minY;

	// Display format: "XX.XXX mA"
	// Layout: D0(4) D1(4) .(2) D2(4) D3(4) D4(4) space(2) m(4) A(4) = 32 pixels wide
	// Height: 7 pixels

	// Extract digits for XX.XXX format (current in uA, display as mA)
	int displayVal = current_ua;
	if (displayVal > 99999) displayVal = 99999;

	int d0 = (displayVal / 10000) % 10;
	int d1 = (displayVal / 1000) % 10;
	int d2 = (displayVal / 100) % 10;
	int d3 = (displayVal / 10) % 10;
	int d4 = displayVal % 10;

	bool lit = false;

	// Digit 0: px 0-3
	if (px >= 0 && px < 4 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px, py, d0);
	}
	// Digit 1: px 5-8
	else if (px >= 5 && px < 9 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 5, py, d1);
	}
	// Decimal point: px 10-11, py 5-6
	else if (px >= 10 && px < 12 && py >= 5 && py < 7)
	{
		lit = true;
	}
	// Digit 2: px 13-16
	else if (px >= 13 && px < 17 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 13, py, d2);
	}
	// Digit 3: px 18-21
	else if (px >= 18 && px < 22 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 18, py, d3);
	}
	// Digit 4: px 23-26
	else if (px >= 23 && px < 27 && py >= 0 && py < 7)
	{
		lit = isSegmentLit(px - 23, py, d4);
	}
	// Letter m: px 29-32
	else if (px >= 29 && px < 33 && py >= 0 && py < 7)
	{
		lit = isLetterM(px - 29, py);
	}
	// Letter A: px 34-37
	else if (px >= 34 && px < 38 && py >= 0 && py < 7)
	{
		lit = isLetterA(px - 34, py);
	}

	// Set colors - cyan/blue for ammeter
	if (lit)
	{
		*colr = 0;
		*colg = 200;
		*colb = 255;
		*firea = 100;
		*firer = 0;
		*fireg = 150;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD;
		*pixel_mode |= PMODE_GLOW;
	}
	else
	{
		// Dark blue background
		*colr = 10;
		*colg = 20;
		*colb = 40;
	}

	return 0;
}
