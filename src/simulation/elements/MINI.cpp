#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MINI()
{
	Identifier = "DEFAULT_PT_MINI";
	Name = "MINI";
	Colour = 0x8B0000_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 80;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 500.0f + 273.15f;  // Hot!
	DefaultProperties.tmp = 0;   // Eruption timer
	DefaultProperties.tmp2 = 50; // Magma pressure
	HeatConduct = 100;
	Description = "Mini Volcano. Periodically erupts with lava and smoke!";

	Properties = TYPE_SOLID | PROP_HOT_GLOW;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 500.0f;  // Cools to rock
	LowTemperatureTransition = PT_ROCK;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Build up pressure
	parts[i].tmp++;
	parts[i].tmp2 = std::min(parts[i].tmp2 + 1, 200);

	// Stay hot
	if (parts[i].temp < 1500.0f)
	{
		parts[i].temp += 5.0f;
	}

	bool erupting = false;

	// Eruption conditions
	if (parts[i].tmp >= 100 || parts[i].tmp2 >= 150)
	{
		// Check for open space above
		bool hasOpening = false;
		for (int dy = 1; dy <= 3; dy++)
		{
			if (y - dy >= 0 && !pmap[y-dy][x])
			{
				hasOpening = true;
				break;
			}
		}

		if (hasOpening)
		{
			erupting = true;

			// ERUPTION!
			// Create lava
			for (int j = 0; j < 3; j++)
			{
				int py = y - sim->rng.between(1, 3);
				int px = x + sim->rng.between(-1, 1);
				if (py >= 0 && px >= 0 && px < XRES && !pmap[py][px])
				{
					int np = sim->create_part(-1, px, py, PT_LAVA);
					if (np >= 0)
					{
						parts[np].temp = 2000.0f + sim->rng.between(0, 500);
						parts[np].ctype = PT_ROCK;
						parts[np].vy = sim->rng.between(-15, -5);
						parts[np].vx = sim->rng.between(-3, 3);
					}
				}
			}

			// Create smoke
			for (int j = 0; j < 2; j++)
			{
				int py = y - sim->rng.between(2, 5);
				int px = x + sim->rng.between(-2, 2);
				if (py >= 0 && px >= 0 && px < XRES && !pmap[py][px])
				{
					int np = sim->create_part(-1, px, py, PT_SMKE);
					if (np >= 0)
					{
						parts[np].life = 100;
						parts[np].temp = parts[i].temp;
						parts[np].vy = sim->rng.between(-10, -3);
					}
				}
			}

			// Pressure wave
			sim->pv[y/CELL][x/CELL] += 5.0f;

			// Reset eruption timer
			parts[i].tmp = 0;
			parts[i].tmp2 -= 50;
		}
	}

	// Small lava seepage between eruptions
	if (!erupting && sim->rng.chance(1, 50))
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			if (!pmap[y-1][x+dx] && sim->rng.chance(1, 5))
			{
				int np = sim->create_part(-1, x+dx, y-1, PT_LAVA);
				if (np >= 0)
				{
					parts[np].temp = 1500.0f;
					parts[np].ctype = PT_ROCK;
					parts[np].vy = -0.5f;
				}
				break;
			}
		}
	}

	// Heat up surroundings
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				int rID = ID(r);

				// Radiate heat
				if (parts[rID].temp < parts[i].temp - 100)
				{
					parts[rID].temp += 20.0f;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int pressure = cpart->tmp2;
	float temp = cpart->temp;

	// Dark red to orange based on pressure
	*colr = 139 + pressure / 3;
	*colg = pressure / 4;
	*colb = 0;

	if (*colr > 255) *colr = 255;
	if (*colg > 100) *colg = 100;

	// Hot glow
	float glowIntensity = (temp - 500) / 20.0f;
	if (glowIntensity > 150) glowIntensity = 150;

	*firea = (int)glowIntensity;
	*firer = 255;
	*fireg = 100;
	*fireb = 0;
	*pixel_mode |= FIRE_ADD;

	// Extra glow when about to erupt
	if (pressure > 100)
	{
		*firea += (pressure - 100);
		*colr = 255;
		*colg = 100;
	}

	return 0;
}
