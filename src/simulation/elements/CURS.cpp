#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_CURS()
{
	Identifier = "DEFAULT_PT_CURS";
	Name = "CURS";
	Colour = 0x990099_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.02f;  // Sinks slightly
	Diffusion = 0.15f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 3;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Flicker
	HeatConduct = 0;
	Description = "Cursed Dust. Brings misfortune - random negative effects on nearby particles!";

	Properties = TYPE_PART;

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
	parts[i].tmp = (parts[i].tmp + 1) % 20;

	// Ominous drifting
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-5, 10) * 0.02f;
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

				if (rt == PT_CURS || rt == PT_LUCK)
					continue;

				// CURSED EFFECTS!
				if (sim->rng.chance(1, 40))
				{
					int effect = sim->rng.between(0, 9);
					switch (effect)
					{
					case 0:
						// Damage - reduce life
						if (parts[rID].life > 0)
						{
							parts[rID].life -= 5;
						}
						break;
					case 1:
						// Overheat!
						parts[rID].temp += 50.0f;
						break;
					case 2:
						// Spontaneous combustion
						if (sim->rng.chance(1, 50))
						{
							sim->create_part(-1, x+rx, y+ry+1, PT_FIRE);
						}
						break;
					case 3:
						// Slow down
						parts[rID].vx *= 0.5f;
						parts[rID].vy *= 0.5f;
						break;
					case 4:
						// Lead transmutation (opposite of gold)
						if (rt == PT_GOLD)
						{
							if (sim->rng.chance(1, 50))
							{
								sim->part_change_type(rID, x+rx, y+ry, PT_STNE);
							}
						}
						break;
					case 5:
						// Decay
						if (rt == PT_WOOD || rt == PT_PLNT)
						{
							if (sim->rng.chance(1, 30))
							{
								sim->kill_part(rID);
							}
						}
						break;
					case 6:
						// Create acid
						if (rt == PT_WATR && sim->rng.chance(1, 100))
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_ACID);
						}
						break;
					case 7:
						// Corrupt
						if (rt == PT_YEST && sim->rng.chance(1, 20))
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_DUST);
						}
						break;
					case 8:
						// Freeze
						if (sim->rng.chance(1, 30))
						{
							parts[rID].temp -= 50.0f;
						}
						break;
					case 9:
						// Create smoke
						if (!pmap[y-1][x] && sim->rng.chance(1, 20))
						{
							int np = sim->create_part(-1, x, y-1, PT_SMKE);
							if (np >= 0)
							{
								parts[np].life = 50;
							}
						}
						break;
					}
				}
			}
		}
	}

	// Luck destroys curse
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			auto r = pmap[y+ry][x+rx];
			if (r && TYP(r) == PT_LUCK)
			{
				// Cancel out
				sim->kill_part(ID(r));
				sim->kill_part(i);
				return 1;
			}
		}
	}

	// Curse spreads slowly
	if (sim->rng.chance(1, 200))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_CURS);
		if (np >= 0)
		{
			sim->kill_part(i);
			return 1;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int flicker = cpart->tmp;

	// Dark purple
	*colr = 153;
	*colg = 0;
	*colb = 153;

	// Flickering ominous effect
	int pulse = (flicker < 10) ? flicker : 20 - flicker;
	*colr -= pulse * 5;
	*colb -= pulse * 3;

	// Dark aura
	*firea = 60 + pulse * 3;
	*firer = 100;
	*fireg = 0;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
