#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_FOAM()
{
	Identifier = "DEFAULT_PT_FOAM";
	Name = "FOAM";
	Colour = 0xFFFAF0_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.90f;
	Collision = 0.0f;
	Gravity = -0.01f;  // Slightly buoyant
	Diffusion = 0.05f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 10;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 50;   // Bubble count (density)
	HeatConduct = 10;  // Good insulator
	Description = "Foam. Expands when heated, insulates. Light and bubbly.";

	Properties = TYPE_LIQUID;

	LowPressure = -2.0f;  // Pops under vacuum
	LowPressureTransition = PT_WTRV;
	HighPressure = 10.0f;  // Squishes under pressure
	HighPressureTransition = PT_WATR;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 500.0f;  // Melts/burns
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Expand when heated!
	if (parts[i].temp > 323.0f)  // Above 50C
	{
		// Try to expand
		if (parts[i].tmp < 200 && sim->rng.chance(1, 50))
		{
			parts[i].tmp++;

			// Create new foam if we're really expanding
			if (parts[i].tmp > 100 && sim->rng.chance(1, 20))
			{
				for (int rx = -1; rx <= 1; rx++)
				{
					for (int ry = -1; ry <= 1; ry++)
					{
						if (!pmap[y+ry][x+rx] && sim->rng.chance(1, 5))
						{
							int np = sim->create_part(-1, x+rx, y+ry, PT_FOAM);
							if (np >= 0)
							{
								parts[np].tmp = 30;
								parts[np].temp = parts[i].temp;
								parts[i].tmp -= 30;
							}
							break;
						}
					}
				}
			}
		}
	}

	// Contract when cold
	if (parts[i].temp < 290.0f && parts[i].tmp > 20)
	{
		if (sim->rng.chance(1, 100))
		{
			parts[i].tmp--;
		}
	}

	// Pop bubbles sometimes
	if (sim->rng.chance(1, 1000))
	{
		parts[i].tmp = std::max(parts[i].tmp - 1, 1);
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
				case PT_FIRE:
				case PT_PLSM:
					// Foam extinguishes fire!
					if (sim->rng.chance(1, 10))
					{
						sim->kill_part(rID);
						parts[i].tmp -= 10;
						if (parts[i].tmp <= 0)
						{
							sim->kill_part(i);
							return 1;
						}
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Water makes more foam!
					if (sim->rng.chance(1, 100))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_FOAM);
						parts[rID].tmp = 30;
					}
					break;
				case PT_SOAP:
					// Soap makes lots of foam!
					if (sim->rng.chance(1, 20))
					{
						parts[i].tmp = std::min(parts[i].tmp + 20, 200);
					}
					break;
				case PT_OIL:
				case PT_DESL:
					// Oil destroys foam
					if (sim->rng.chance(1, 50))
					{
						parts[i].tmp -= 5;
						if (parts[i].tmp <= 0)
						{
							sim->kill_part(i);
							return 1;
						}
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Insulation - slow down heat transfer from nearby particles
	// (This is handled by low HeatConduct)

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int density = cpart->tmp;

	// Whiter and more opaque when denser
	*colr = 255;
	*colg = 250 - density / 4;
	*colb = 240 - density / 3;

	// Semi-transparent
	*pixel_mode |= PMODE_BLEND;
	*cola = 100 + density / 2;

	// Bubbly glow
	if (density > 50)
	{
		*firea = density / 4;
		*firer = 255;
		*fireg = 255;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
