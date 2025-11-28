#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_GRND()
{
	Identifier = "DEFAULT_PT_GRND";
	Name = "GRND";
	Colour = 0x004400_rgb;
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
	DefaultProperties.tmp = 0;   // Absorbed current counter
	DefaultProperties.tmp2 = 0;  // Visual pulse
	DefaultProperties.life = 0;
	HeatConduct = 255;
	Description = "Ground. Absorbs electrical current. Essential reference for circuits.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC;

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
	int absorbed = 0;

	// Absorb all incoming sparks
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

				// Absorb sparks - convert back to base material
				if (rt == PT_SPRK)
				{
					absorbed++;
					int baseType = parts[rID].ctype;
					if (baseType > 0 && baseType < PT_NUM)
					{
						sim->part_change_type(rID, x+rx, y+ry, baseType);
						parts[rID].life = 4;  // Brief cooldown
					}
					else
					{
						sim->kill_part(rID);
					}
				}

				// Discharge capacitors
				if (rt == PT_CAPA && parts[rID].tmp > 0)
				{
					parts[rID].tmp -= 10;
					if (parts[rID].tmp < 0) parts[rID].tmp = 0;
					absorbed++;
				}
			}
		}
	}

	// Track absorbed current for visual effect
	if (absorbed > 0)
	{
		parts[i].tmp += absorbed;
		parts[i].tmp2 = 10;  // Visual pulse
	}

	// Decay visual
	if (parts[i].tmp2 > 0)
		parts[i].tmp2--;

	// Slowly reset counter
	if (parts[i].tmp > 0 && sim->rng.chance(1, 10))
		parts[i].tmp--;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int absorbed = cpart->tmp;
	int pulse = cpart->tmp2;

	// Dark green base
	*colr = 0;
	*colg = 68 + std::min(absorbed, 50);
	*colb = 0;

	// Pulse when absorbing current
	if (pulse > 0)
	{
		*colg += pulse * 10;
		*firea = pulse * 5;
		*firer = 0;
		*fireg = 150;
		*fireb = 0;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
