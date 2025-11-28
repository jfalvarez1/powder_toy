#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_RAIN()
{
	Identifier = "DEFAULT_PT_RAIN";
	Name = "RAIN";
	Colour = 0x6699CC_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 0.5f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.02f;  // Floats like a cloud
	Diffusion = 0.10f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Water content
	HeatConduct = 30;
	Description = "Rain Cloud. Floats around and continuously produces rain drops!";

	Properties = TYPE_GAS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 260.0f;  // Turns to snow cloud
	LowTemperatureTransition = NT;
	HighTemperature = 400.0f;  // Evaporates
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Drift around
	if (sim->rng.chance(1, 10))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.01f;
		parts[i].vy += sim->rng.between(-5, 3) * 0.01f;
	}

	// Rain production!
	if (parts[i].tmp > 0)
	{
		// Higher chance when warmer
		int rainChance = (parts[i].temp > 290.0f) ? 30 : 60;

		if (sim->rng.chance(1, rainChance))
		{
			// Create rain drop below
			int np = -1;
			for (int dy = 1; dy <= 3; dy++)
			{
				if (!pmap[y+dy][x])
				{
					if (parts[i].temp < 273.0f)
					{
						// Snow!
						np = sim->create_part(-1, x, y+dy, PT_SNOW);
					}
					else
					{
						np = sim->create_part(-1, x, y+dy, PT_WATR);
					}
					break;
				}
			}

			if (np >= 0)
			{
				parts[np].temp = parts[i].temp;
				parts[np].vy = 1.0f;  // Falling
				parts[i].tmp--;  // Use up water
			}
		}
	}

	// Absorb water vapor to refill
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				if (rt == PT_WTRV)
				{
					if (parts[i].tmp < 200 && sim->rng.chance(1, 20))
					{
						sim->kill_part(rID);
						parts[i].tmp += 10;
					}
				}

				// Clouds merge
				if (rt == PT_RAIN && sim->rng.chance(1, 100))
				{
					parts[i].tmp = std::min(parts[i].tmp + parts[rID].tmp / 2, 300);
					sim->kill_part(rID);
				}
			}
		}
	}

	// Cloud dissipates when empty
	if (parts[i].tmp <= 0 && sim->rng.chance(1, 50))
	{
		sim->kill_part(i);
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int water = cpart->tmp;
	float temp = cpart->temp;

	// Darker when more water
	int darkness = water / 3;
	if (darkness > 80) darkness = 80;

	*colr = 102 - darkness / 2;
	*colg = 153 - darkness / 2;
	*colb = 204 - darkness / 3;

	// Blue tint when cold (snow cloud)
	if (temp < 273.0f)
	{
		*colr = 150;
		*colg = 180;
		*colb = 220;
	}

	// Fluffy cloud glow
	*firea = 50 + water / 4;
	*firer = 150;
	*fireg = 170;
	*fireb = 200;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
