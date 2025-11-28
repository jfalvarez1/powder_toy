#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_VNOM()
{
	Identifier = "DEFAULT_PT_VNOM";
	Name = "VNOM";
	Colour = 0x4B0082_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 25;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Potency
	HeatConduct = 35;
	Description = "Venom. Deadly poison that damages living things. Dilutes in water.";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 400.0f;
	HighTemperatureTransition = PT_GAS;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Venom loses potency over time
	if (sim->rng.chance(1, 1000))
	{
		parts[i].tmp--;
		if (parts[i].tmp <= 0)
		{
			// Becomes harmless
			sim->part_change_type(i, x, y, PT_WATR);
			return 0;
		}
	}

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

				switch (rt)
				{
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Poison stickmen!
					if (sim->rng.chance(parts[i].tmp, 500))
					{
						parts[rID].life -= parts[i].tmp / 10;
						parts[i].tmp -= 10;
						// Visual feedback
						parts[rID].temp += 5.0f;
					}
					break;
				case PT_PLNT:
				case PT_VINE:
				case PT_WOOD:
					// Kill plants
					if (sim->rng.chance(parts[i].tmp, 200))
					{
						sim->kill_part(rID);
						parts[i].tmp -= 5;
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Dilute in water
					if (sim->rng.chance(1, 20))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_VNOM);
						parts[rID].tmp = parts[i].tmp / 3;
						parts[i].tmp = parts[i].tmp * 2 / 3;
					}
					break;
				case PT_SOAP:
					// Soap neutralizes venom
					if (sim->rng.chance(1, 10))
					{
						parts[i].tmp -= 20;
						if (parts[i].tmp <= 0)
						{
							sim->part_change_type(i, x, y, PT_WATR);
							return 0;
						}
					}
					break;
				case PT_YEST:
					// Kills microorganisms
					if (sim->rng.chance(parts[i].tmp, 100))
					{
						sim->kill_part(rID);
						parts[i].tmp -= 2;
					}
					break;
				case PT_VNOM:
					// Combine venom
					if (parts[rID].tmp < parts[i].tmp && sim->rng.chance(1, 50))
					{
						parts[rID].tmp += parts[i].tmp / 4;
						parts[i].tmp -= parts[i].tmp / 4;
					}
					break;
				case PT_ACID:
					// Venom is neutralized by acid
					if (sim->rng.chance(1, 30))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Venom slowly evaporates when hot
	if (parts[i].temp > 350.0f && sim->rng.chance(1, 100))
	{
		// Create toxic gas
		int np = sim->create_part(-1, x, y - 1, PT_GAS);
		if (np >= 0)
		{
			parts[np].temp = parts[i].temp;
		}
		parts[i].tmp -= 5;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int potency = cpart->tmp;

	// Purple, darker when more potent
	*colr = 75 + (100 - potency) / 2;
	*colg = 0 + potency / 5;
	*colb = 130 + potency / 2;

	if (*colb > 200) *colb = 200;

	// Toxic glow when potent
	if (potency > 50)
	{
		*firea = potency / 3;
		*firer = 100;
		*fireg = 0;
		*fireb = 150;
		*pixel_mode |= FIRE_ADD;
	}

	*pixel_mode |= PMODE_BLEND;
	*cola = 200;

	return 0;
}
