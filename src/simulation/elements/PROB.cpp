#include "simulation/ElementCommon.h"
#include <cmath>
#include <queue>
#include <vector>

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

// Flood-fill to propagate signal through connected probes
static void propagateSignal(Simulation *sim, int startX, int startY, int signal, int channel)
{
	bool visited[YRES][XRES] = {false};
	std::queue<std::pair<int,int>> toVisit;
	toVisit.push({startX, startY});
	visited[startY][startX] = true;

	while (!toVisit.empty())
	{
		auto [cx, cy] = toVisit.front();
		toVisit.pop();

		// Set signal on this probe
		auto r = sim->pmap[cy][cx];
		if (r && TYP(r) == PT_PROB)
		{
			int idx = ID(r);
			// Only update if our signal is higher (avoids overwriting with stale data)
			if (signal > sim->parts[idx].tmp2)
			{
				sim->parts[idx].tmp2 = signal;
				sim->parts[idx].tmp3 = signal;
				sim->parts[idx].tmp4 = channel;
			}
		}

		// Check all 8 neighbors
		for (int dx = -1; dx <= 1; dx++)
		{
			for (int dy = -1; dy <= 1; dy++)
			{
				if (dx == 0 && dy == 0) continue;
				int nx = cx + dx;
				int ny = cy + dy;
				if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES) continue;
				if (visited[ny][nx]) continue;

				auto nr = sim->pmap[ny][nx];
				if (nr && TYP(nr) == PT_PROB)
				{
					visited[ny][nx] = true;
					toVisit.push({nx, ny});
				}
			}
		}
	}
}

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
			// VCCS tmp2 is effective voltage (10-500), scale to 0-100 range
			if (rt == PT_VCCS)
			{
				int vccsVoltage = parts[rID].tmp2;
				// Scale: 10V = signal 20, 50V = signal 100
				directSignal = std::max(directSignal, std::min(vccsVoltage * 2, 100));
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

	if (hasDirectSource)
	{
		// Clamp signal
		if (directSignal > 100) directSignal = 100;
		if (directSignal < 0) directSignal = 0;

		// Propagate signal instantly to all connected probes via flood-fill
		propagateSignal(sim, x, y, directSignal, channel);
	}
	else
	{
		// No direct source - check neighboring probes for signal
		int neighborSignal = 0;
		for (int rx = -1; rx <= 1; rx++)
		{
			for (int ry = -1; ry <= 1; ry++)
			{
				if (rx == 0 && ry == 0) continue;
				int nx = x + rx;
				int ny = y + ry;
				if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES) continue;

				auto r = pmap[ny][nx];
				if (r && TYP(r) == PT_PROB)
				{
					neighborSignal = std::max(neighborSignal, (int)parts[ID(r)].tmp2);
				}
			}
		}

		if (neighborSignal > 0)
		{
			// Take signal from neighbors (propagation)
			parts[i].tmp2 = neighborSignal;
			parts[i].tmp3 = neighborSignal;
		}
		else
		{
			// No neighbors with signal - decay
			int currentSignal = parts[i].tmp2;
			if (currentSignal > 0)
			{
				currentSignal -= 5;
				if (currentSignal < 0) currentSignal = 0;
				parts[i].tmp2 = currentSignal;
				parts[i].tmp3 = currentSignal;
			}
		}
	}

	parts[i].tmp4 = channel;

	// Visual history
	int sampleValue = parts[i].tmp2;
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
