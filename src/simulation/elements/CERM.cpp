#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_CERM()
{
	Identifier = "DEFAULT_PT_CERM";
	Name = "CERM";
	Colour = 0xE8DCC8_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
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
	Hardness = 80;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Stress/damage
	HeatConduct = 30;  // Poor heat conductor
	Description = "Ceramic. Heat-resistant and hard, but brittle. Shatters under pressure.";

	Properties = TYPE_SOLID | PROP_HOT_GLOW;

	LowPressure = -15.0f;  // Shatters under vacuum
	LowPressureTransition = PT_DUST;
	HighPressure = 25.0f;  // Shatters under high pressure
	HighPressureTransition = PT_DUST;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 2500.0f;  // Very high melting point!
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	float pressure = sim->pv[y/CELL][x/CELL];

	// Accumulate stress from pressure changes
	if (fabsf(pressure) > 5.0f)
	{
		parts[i].tmp += (int)(fabsf(pressure) / 5);
	}

	// Thermal shock - rapid temperature changes cause stress
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				float tempDiff = fabsf(parts[i].temp - parts[rID].temp);

				// Thermal shock!
				if (tempDiff > 200.0f)
				{
					parts[i].tmp += (int)(tempDiff / 100);
				}

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
					// Cold water on hot ceramic = thermal shock
					if (parts[i].temp > 500.0f)
					{
						parts[i].tmp += 10;
					}
					break;
				case PT_LAVA:
					// Can contain lava!
					// Slowly absorbs heat
					if (parts[i].temp < parts[rID].temp)
					{
						parts[i].temp += 1.0f;
						parts[rID].temp -= 0.5f;
					}
					break;
				case PT_ACID:
					// Resistant to acid
					if (sim->rng.chance(1, 500))
					{
						// Very slowly dissolves
						parts[i].tmp += 5;
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Fire resistant - absorbs heat slowly
					if (parts[i].temp < 2000.0f)
					{
						parts[i].temp += 2.0f;
					}
					break;
				case PT_VIBR:
				case PT_BVBR:
					// Vibranium destroys ceramic!
					parts[i].tmp += 20;
					break;
				default:
					break;
				}
			}
		}
	}

	// Shatter if too much stress accumulated
	if (parts[i].tmp > 100)
	{
		// Shatter into pieces!
		for (int j = 0; j < 4; j++)
		{
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_DUST);
			if (np >= 0)
			{
				parts[np].temp = parts[i].temp;
				parts[np].vx = sim->rng.between(-5, 5);
				parts[np].vy = sim->rng.between(-5, 5);
			}
		}
		sim->kill_part(i);
		return 1;
	}

	// Slowly heal stress over time
	if (parts[i].tmp > 0 && sim->rng.chance(1, 100))
	{
		parts[i].tmp--;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int stress = cpart->tmp;
	float temp = cpart->temp;

	// Cream/white ceramic color
	*colr = 232;
	*colg = 220;
	*colb = 200;

	// Show cracks (darker) when stressed
	if (stress > 30)
	{
		int crack = stress - 30;
		*colr -= crack;
		*colg -= crack;
		*colb -= crack;
	}

	// Hot glow
	if (temp > 1000.0f)
	{
		float glow = (temp - 1000.0f) / 1500.0f;
		if (glow > 1.0f) glow = 1.0f;

		*colr = (int)(232 + (255 - 232) * glow);
		*colg = (int)(220 * (1 - glow * 0.5f) + 100 * glow);
		*colb = (int)(200 * (1 - glow));

		*firea = (int)(glow * 100);
		*firer = 255;
		*fireg = 150;
		*fireb = 50;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
