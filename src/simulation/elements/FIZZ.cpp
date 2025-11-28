#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_FIZZ()
{
	Identifier = "DEFAULT_PT_FIZZ";
	Name = "FIZZ";
	Colour = 0xCC9966_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 25;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // CO2 content (fizziness)
	DefaultProperties.tmp2 = 0;   // Shake level
	HeatConduct = 40;
	Description = "Fizzy Drink. Bubbles when shaken or heated! Explodes if shaken too much!";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	float pressure = sim->pv[y/CELL][x/CELL];
	float speed = fabsf(parts[i].vx) + fabsf(parts[i].vy);

	// Being shaken increases fizz
	if (speed > 0.5f)
	{
		parts[i].tmp2 += (int)(speed * 2);
	}

	// Temperature affects fizziness
	if (parts[i].temp > 300.0f)
	{
		parts[i].tmp2 += 2;
	}

	// Pressure affects fizziness
	if (pressure > 2.0f)
	{
		parts[i].tmp2 += (int)pressure;
	}

	// Release bubbles based on shake level
	if (parts[i].tmp > 0 && parts[i].tmp2 > 20)
	{
		if (sim->rng.chance(parts[i].tmp2, 200))
		{
			// Create CO2 bubble
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y - 1, PT_CO2);
			if (np >= 0)
			{
				parts[np].temp = parts[i].temp;
				parts[np].vy = -1.0f;
			}
			parts[i].tmp--;
			parts[i].tmp2 -= 5;
		}
	}

	// EXPLOSION if shaken too much!
	if (parts[i].tmp2 > 150 && parts[i].tmp > 50)
	{
		// Explosive fizz eruption!
		sim->pv[y/CELL][x/CELL] += 10.0f;

		for (int j = 0; j < 10; j++)
		{
			int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-3, 0), PT_CO2);
			if (np >= 0)
			{
				parts[np].vx = sim->rng.between(-10, 10);
				parts[np].vy = sim->rng.between(-15, -5);
				parts[np].temp = parts[i].temp;
			}
		}

		// Spray liquid everywhere
		for (int j = 0; j < 5; j++)
		{
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-2, 0), PT_FIZZ);
			if (np >= 0)
			{
				parts[np].vx = sim->rng.between(-8, 8);
				parts[np].vy = sim->rng.between(-12, -3);
				parts[np].tmp = 10;  // Low fizz
				parts[np].tmp2 = 0;
			}
		}

		parts[i].tmp = 10;
		parts[i].tmp2 = 0;
	}

	// Calm down over time
	if (parts[i].tmp2 > 0 && sim->rng.chance(1, 10))
	{
		parts[i].tmp2--;
	}

	// Goes flat over time
	if (parts[i].tmp > 0 && sim->rng.chance(1, 1000))
	{
		parts[i].tmp--;
	}

	// Fizz spreads excitement to neighbors
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				if (TYP(r) == PT_FIZZ)
				{
					int rID = ID(r);
					// Share shake level
					if (parts[i].tmp2 > parts[rID].tmp2 + 10)
					{
						parts[i].tmp2 -= 5;
						parts[rID].tmp2 += 5;
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int fizz = cpart->tmp;
	int shake = cpart->tmp2;

	// Brown soda color
	*colr = 204;
	*colg = 153;
	*colb = 102;

	// Lighter when flat
	if (fizz < 30)
	{
		*colr = 180;
		*colg = 140;
		*colb = 100;
	}

	// Bubbling effect when shaken
	if (shake > 20)
	{
		*firea = shake;
		*firer = 255;
		*fireg = 230;
		*fireb = 200;
		*pixel_mode |= FIRE_ADD;
	}

	// WARNING flash when about to explode
	if (shake > 100)
	{
		*colr = 255;
		*colg = 200;
		*colb = 150;
		*firea = 150;
	}

	return 0;
}
