#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);

void Element::Element_TNDR()
{
	Identifier = "DEFAULT_PT_TNDR";
	Name = "TNDR";
	Colour = 0x8B4513_rgb;
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
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 1000;  // Extremely flammable
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 20;

	DefaultProperties.temp = R_TEMP + 273.15f;
	HeatConduct = 50;
	Description = "Tinder. Extremely flammable kindling. Ignites from any heat source or spark.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 323.0f;  // Ignites at just 50C!
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
}

static int update(UPDATE_FUNC_ARGS)
{
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

				// Ignites from anything warm or sparky
				switch (rt)
				{
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
				case PT_THDR:
				case PT_SPRK:
				case PT_EMBR:
					sim->part_change_type(i, x, y, PT_FIRE);
					parts[i].life = sim->rng.between(50, 100);
					parts[i].temp = 600.0f + 273.15f;
					return 0;
				default:
					// Also ignites from hot particles
					if (parts[ID(r)].temp > 350.0f)
					{
						if (sim->rng.chance(1, 10))
						{
							sim->part_change_type(i, x, y, PT_FIRE);
							parts[i].life = sim->rng.between(50, 100);
							parts[i].temp = 600.0f + 273.15f;
							return 0;
						}
					}
					break;
				}
			}
		}
	}
	return 0;
}
