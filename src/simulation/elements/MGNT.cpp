#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MGNT()
{
	Identifier = "DEFAULT_PT_MGNT";
	Name = "MGNT";
	Colour = 0x404040_rgb;
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
	Meltable = 1;
	Hardness = 50;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 50;   // Magnetic strength
	DefaultProperties.tmp2 = 0;   // Polarity (0=north, 1=south)
	HeatConduct = 150;
	Description = "Magnet. Attracts metals and repels other magnets. Loses strength when hot.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_KILL_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1043.0f;  // Curie point - loses magnetism
	HighTemperatureTransition = PT_IRON;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int strength = parts[i].tmp;
	int polarity = parts[i].tmp2;

	// Lose magnetism at high temperatures (Curie effect)
	if (parts[i].temp > 800.0f)
	{
		parts[i].tmp = std::max(parts[i].tmp - 1, 0);
		if (parts[i].tmp <= 0)
		{
			// Demagnetized
			sim->part_change_type(i, x, y, PT_IRON);
			return 0;
		}
	}

	// Magnetic field effect - larger search radius based on strength
	int range = 2 + strength / 20;
	if (range > 6) range = 6;

	for (auto rx = -range; rx <= range; rx++)
	{
		for (auto ry = -range; ry <= range; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				float dist = sqrtf(rx*rx + ry*ry);
				float force = strength / (dist * dist + 1) * 0.05f;

				switch (rt)
				{
				case PT_IRON:
				case PT_METL:
				case PT_BMTL:
				case PT_BRMT:
				case PT_TTAN:
				case PT_NSCN:
				case PT_PSCN:
					// Attract metals!
					parts[rID].vx -= rx * force;
					parts[rID].vy -= ry * force;
					break;
				case PT_MGNT:
					{
						// Magnets interact!
						int otherPolarity = parts[rID].tmp2;
						if (polarity == otherPolarity)
						{
							// Same polarity = repel
							parts[rID].vx += rx * force * 2;
							parts[rID].vy += ry * force * 2;
						}
						else
						{
							// Opposite polarity = attract (but magnets are solid)
							// Just creates tension
						}
					}
					break;
				case PT_GOLD:
					// Weakly diamagnetic - slight repulsion
					parts[rID].vx += rx * force * 0.1f;
					parts[rID].vy += ry * force * 0.1f;
					break;
				case PT_SPRK:
					// Electric current near magnet - electromagnetic induction
					if (sim->rng.chance(1, 50))
					{
						// Creates energy
						parts[i].temp += 1.0f;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// FRFL (ferrofluid) is strongly attracted
	for (auto rx = -range; rx <= range; rx++)
	{
		for (auto ry = -range; ry <= range; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;

				if (TYP(r) == PT_FRFL)
				{
					float dist = sqrtf(rx*rx + ry*ry);
					float force = strength / (dist + 1) * 0.1f;
					int rID = ID(r);
					parts[rID].vx -= rx * force;
					parts[rID].vy -= ry * force;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int strength = cpart->tmp;
	int polarity = cpart->tmp2;

	// Gray metal look
	*colr = 64 + strength;
	*colg = 64 + strength;
	*colb = 64 + strength;

	if (*colr > 150) *colr = 150;
	if (*colg > 150) *colg = 150;
	if (*colb > 150) *colb = 150;

	// Color tint based on polarity
	if (polarity == 0)
	{
		// North = reddish
		*colr += 30;
	}
	else
	{
		// South = bluish
		*colb += 30;
	}

	// Magnetic field glow
	if (strength > 30)
	{
		*firea = strength / 4;
		*firer = 100;
		*fireg = 100;
		*fireb = 150;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
