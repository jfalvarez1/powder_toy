#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_LUCK()
{
	Identifier = "DEFAULT_PT_LUCK";
	Name = "LUCK";
	Colour = 0x00FF00_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.03f;  // Floats up
	Diffusion = 0.20f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Sparkle timer
	HeatConduct = 0;
	Description = "Lucky Dust. Brings good fortune - random positive effects on nearby particles!";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 500.0f;
	HighTemperatureTransition = PT_NONE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	parts[i].tmp = (parts[i].tmp + 1) % 30;

	// Lucky floating
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-10, 5) * 0.02f;
	}

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

				if (rt == PT_LUCK || rt == PT_CURS)
					continue;

				// LUCKY EFFECTS!
				if (sim->rng.chance(1, 50))
				{
					int effect = sim->rng.between(0, 9);
					switch (effect)
					{
					case 0:
						// Healing - restore life
						if (parts[rID].life > 0 && parts[rID].life < 100)
						{
							parts[rID].life += 10;
						}
						break;
					case 1:
						// Cool down dangerous heat
						if (parts[rID].temp > 500.0f)
						{
							parts[rID].temp -= 100.0f;
						}
						break;
					case 2:
						// Protect from fire
						if (rt == PT_FIRE || rt == PT_PLSM)
						{
							sim->kill_part(rID);
						}
						break;
					case 3:
						// Speed boost
						parts[rID].vx *= 1.2f;
						parts[rID].vy *= 1.2f;
						break;
					case 4:
						// Gold transmutation!
						if (rt == PT_STNE || rt == PT_SAND)
						{
							if (sim->rng.chance(1, 100))
							{
								sim->part_change_type(rID, x+rx, y+ry, PT_GOLD);
							}
						}
						break;
					case 5:
						// Duplicate valuable elements
						if (rt == PT_GOLD || rt == PT_DMND)
						{
							if (sim->rng.chance(1, 200))
							{
								sim->create_part(-1, x+rx+sim->rng.between(-1,1), y+ry+sim->rng.between(-1,1), rt);
							}
						}
						break;
					case 6:
						// Neutralize acid
						if (rt == PT_ACID)
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_WATR);
						}
						break;
					case 7:
						// Fertilize plants
						if (rt == PT_PLNT)
						{
							sim->create_part(-1, x+rx+sim->rng.between(-1,1), y+ry+sim->rng.between(-1,1), PT_PLNT);
						}
						break;
					case 8:
						// Energize
						parts[rID].temp = std::max(parts[rID].temp, R_TEMP + 273.15f);
						break;
					case 9:
						// Create sparkle
						if (!pmap[y-1][x])
						{
							int np = sim->create_part(-1, x, y-1, PT_EMBR);
							if (np >= 0)
							{
								parts[np].life = 10;
								parts[np].ctype = 0x00FF00;
							}
						}
						break;
					}
				}
			}
		}
	}

	// Luck fades slowly
	if (sim->rng.chance(1, 500))
	{
		sim->kill_part(i);
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int sparkle = cpart->tmp;

	// Bright green
	*colr = 0;
	*colg = 255;
	*colb = 0;

	// Sparkle effect
	int pulse = (sparkle < 15) ? sparkle : 30 - sparkle;
	*firea = 80 + pulse * 5;
	*firer = 100;
	*fireg = 255;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	// Extra bright at peak
	if (sparkle < 5)
	{
		*colr = 150;
		*colg = 255;
		*colb = 150;
	}

	return 0;
}
