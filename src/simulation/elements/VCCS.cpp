#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_VCCS()
{
	Identifier = "DEFAULT_PT_VCCS";
	Name = "VCCS";
	Colour = 0xCC0000_rgb;
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
	DefaultProperties.tmp = 10;   // Base voltage (1-100), multiplied by cluster size
	DefaultProperties.tmp2 = 0;   // Calculated effective voltage (read-only display)
	DefaultProperties.life = 0;   // Spark timer
	HeatConduct = 0;
	Description = "VCC/Power supply. Voltage scales with cluster size. Tmp=base voltage. Larger = more power.";

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

// Count connected VCCS particles (cluster size)
static int countCluster(Simulation *sim, int x, int y, int maxCount)
{
	// Simple flood-fill count using a visited array
	// We'll use a simple BFS approach limited to nearby area
	bool visited[25][25] = {false};  // 25x25 area around particle
	int count = 0;
	int ox = x - 12;
	int oy = y - 12;

	// Stack for BFS
	int stackX[625];
	int stackY[625];
	int stackTop = 0;

	// Start with current position
	stackX[stackTop] = x;
	stackY[stackTop] = y;
	stackTop++;

	while (stackTop > 0 && count < maxCount)
	{
		stackTop--;
		int cx = stackX[stackTop];
		int cy = stackY[stackTop];

		// Check bounds
		int lx = cx - ox;
		int ly = cy - oy;
		if (lx < 0 || lx >= 25 || ly < 0 || ly >= 25)
			continue;
		if (visited[ly][lx])
			continue;
		if (cx < 0 || cx >= XRES || cy < 0 || cy >= YRES)
			continue;

		auto r = sim->pmap[cy][cx];
		if (!r || TYP(r) != PT_VCCS)
			continue;

		visited[ly][lx] = true;
		count++;

		// Add neighbors to stack
		for (int dx = -1; dx <= 1; dx++)
		{
			for (int dy = -1; dy <= 1; dy++)
			{
				if (dx || dy)
				{
					int nx = cx + dx;
					int ny = cy + dy;
					int nlx = nx - ox;
					int nly = ny - oy;
					if (nlx >= 0 && nlx < 25 && nly >= 0 && nly < 25 && !visited[nly][nlx])
					{
						if (stackTop < 625)
						{
							stackX[stackTop] = nx;
							stackY[stackTop] = ny;
							stackTop++;
						}
					}
				}
			}
		}
	}

	return count;
}

static int update(UPDATE_FUNC_ARGS)
{
	int baseVoltage = parts[i].tmp;
	if (baseVoltage < 1) baseVoltage = 1;
	if (baseVoltage > 100) baseVoltage = 100;
	parts[i].tmp = baseVoltage;

	// Count cluster size (only periodically to save CPU)
	int clusterSize = 1;
	if (sim->rng.chance(1, 10))
	{
		clusterSize = countCluster(sim, x, y, 100);
	}
	else
	{
		// Use cached value from tmp2 if available
		clusterSize = parts[i].tmp2 > 0 ? (parts[i].tmp2 / baseVoltage) : 1;
		if (clusterSize < 1) clusterSize = 1;
	}

	// Calculate effective voltage: base * sqrt(cluster_size)
	// This gives diminishing returns for very large clusters
	int effectiveVoltage = baseVoltage * (int)(sqrt((float)clusterSize) + 0.5f);
	if (effectiveVoltage > 500) effectiveVoltage = 500;  // Max voltage cap

	parts[i].tmp2 = effectiveVoltage;  // Store for display and probes

	// Output power periodically
	parts[i].life++;
	int outputInterval = 15 - effectiveVoltage / 50;  // Higher voltage = slightly faster
	if (outputInterval < 3) outputInterval = 3;
	if (outputInterval > 15) outputInterval = 15;

	if (parts[i].life > outputInterval)
	{
		parts[i].life = 0;

		// Output spark to adjacent conductors
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

						// Only output to basic conductors
						if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN || rt == PT_NSCN)
						    && parts[rID].life == 0)
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

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int effectiveVoltage = cpart->tmp2;
	if (effectiveVoltage < 1) effectiveVoltage = cpart->tmp;

	// Red color, brightness based on effective voltage
	int brightness = std::min(effectiveVoltage / 2, 105);
	*colr = 150 + brightness;
	*colg = brightness / 4;
	*colb = brightness / 4;

	// Glow intensity based on voltage
	*firea = 30 + effectiveVoltage / 10;
	*firer = 255;
	*fireg = 50 + effectiveVoltage / 10;
	*fireb = 50;
	*pixel_mode |= FIRE_ADD;

	// Extra glow for high voltage
	if (effectiveVoltage > 100)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
