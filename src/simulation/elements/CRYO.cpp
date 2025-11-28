#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_CRYO()
{
	Identifier = "DEFAULT_PT_CRYO";
	Name = "CRYO";
	Colour = 0x00FFFF_rgb;
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
	HotAir = -0.0005f * CFDS;  // Cools surrounding air
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 25;

	DefaultProperties.temp = 20.0f;  // 20K - extremely cold
	HeatConduct = 250;  // Conducts heat very well (absorbs heat from surroundings)
	Description = "Cryogenic Liquid. Extremely cold (-253C), flash-freezes liquids and gases on contact.";

	Properties = TYPE_LIQUID | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;  // Can't freeze further
	LowTemperatureTransition = NT;
	HighTemperature = 77.0f;  // Boils at 77K (like liquid nitrogen)
	HighTemperatureTransition = PT_N2;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

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

				// Flash-freeze effect: rapidly cool nearby particles
				if (parts[rID].temp > parts[i].temp + 50.0f)
				{
					// Transfer cold rapidly
					float tempDiff = parts[rID].temp - parts[i].temp;
					parts[rID].temp -= tempDiff * 0.1f;
					parts[i].temp += tempDiff * 0.02f;  // CRYO heats up slower
				}

				// Special interactions
				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
				case PT_CBNW:
					// Instantly freeze water into ice
					if (sim->rng.chance(1, 5))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_ICEI);
						parts[rID].ctype = rt;
					}
					break;
				case PT_WTRV:
				case PT_FOG:
					// Freeze vapor into snow
					if (sim->rng.chance(1, 3))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_SNOW);
					}
					break;
				case PT_O2:
					// Liquify oxygen
					if (sim->rng.chance(1, 10))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LO2);
					}
					break;
				case PT_N2:
					// Liquify nitrogen
					if (sim->rng.chance(1, 10))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_LNTG);
					}
					break;
				case PT_CO2:
					// CO2 becomes dry ice
					if (sim->rng.chance(1, 10))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_DRIC);
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Extinguish fire instantly
					sim->kill_part(rID);
					parts[i].temp += 50.0f;  // Absorb heat
					break;
				case PT_LAVA:
					// Rapidly solidify lava
					if (sim->rng.chance(1, 3))
					{
						int ctype = parts[rID].ctype;
						if (ctype > 0 && ctype < PT_NUM && elements[ctype].Enabled)
							sim->part_change_type(rID, x+rx, y+ry, ctype);
						else
							sim->part_change_type(rID, x+rx, y+ry, PT_STNE);
						parts[i].temp += 100.0f;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Slowly evaporate if getting warm
	if (parts[i].temp > 70.0f && sim->rng.chance(1, 100))
	{
		parts[i].temp -= 5.0f;  // Evaporative cooling
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Icy blue glow effect
	*colr = 100;
	*colg = 200 + (cpart->temp / 2);
	*colb = 255;
	if (*colg > 255) *colg = 255;

	*firea = 30;
	*firer = 100;
	*fireg = 200;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
