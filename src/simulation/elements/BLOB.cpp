#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_BLOB()
{
	Identifier = "DEFAULT_PT_BLOB";
	Name = "BLOB";
	Colour = 0xFF69B4_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.05f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 20;
	Explosive = 0;
	Meltable = 0;
	Hardness = 30;

	Weight = 35;

	DefaultProperties.temp = R_TEMP + 10.0f + 273.15f;  // Slightly warm
	DefaultProperties.life = 100;  // Hunger
	DefaultProperties.tmp = 0;     // Size/mass
	HeatConduct = 40;
	Description = "Living Blob. A creature that eats organic matter and grows. Gets hungry!";

	Properties = TYPE_LIQUID | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.0f;  // Dies if frozen
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;  // Dies if boiled
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Blob gets hungry over time
	if (parts[i].life <= 0)
	{
		// Starved! Shrink
		parts[i].tmp--;
		parts[i].life = 100;

		if (parts[i].tmp < 0)
		{
			// Died of starvation
			sim->kill_part(i);
			return 1;
		}
	}

	// Movement - blobs like to clump together
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
				{
					// Reproduce if well-fed and big enough
					if (parts[i].tmp >= 50 && parts[i].life > 80 && sim->rng.chance(1, 200))
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_BLOB);
						if (np >= 0)
						{
							parts[np].tmp = 10;  // Baby blob
							parts[np].life = 100;
							parts[i].tmp -= 20;  // Cost to reproduce
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				// Attracted to other blobs
				if (rt == PT_BLOB)
				{
					parts[i].vx += rx * 0.01f;
					parts[i].vy += ry * 0.01f;
					continue;
				}

				// EAT!
				bool ate = false;
				switch (rt)
				{
				case PT_PLNT:
				case PT_VINE:
				case PT_WOOD:
				case PT_SAWD:
					// Plants are tasty
					if (sim->rng.chance(1, 20))
					{
						sim->kill_part(rID);
						parts[i].tmp += 5;
						parts[i].life = std::min(parts[i].life + 50, 200);
						ate = true;
					}
					break;
				case PT_YEST:
					// Yeast is delicious!
					if (sim->rng.chance(1, 10))
					{
						sim->kill_part(rID);
						parts[i].tmp += 10;
						parts[i].life = std::min(parts[i].life + 80, 200);
						ate = true;
					}
					break;
				case PT_DUST:
				case PT_BCOL:
					// Will eat dust if hungry
					if (parts[i].life < 50 && sim->rng.chance(1, 50))
					{
						sim->kill_part(rID);
						parts[i].tmp += 1;
						parts[i].life = std::min(parts[i].life + 20, 200);
						ate = true;
					}
					break;
				case PT_SLIM:
					// Slime is a rival - fight!
					if (sim->rng.chance(1, 30))
					{
						if (parts[i].tmp > 20)
						{
							sim->kill_part(rID);
							parts[i].tmp += 15;
						}
						else
						{
							sim->kill_part(i);
							return 1;
						}
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
					// OUCH!
					parts[i].tmp -= 5;
					parts[i].life -= 20;
					// Try to flee
					parts[i].vx -= rx * 0.5f;
					parts[i].vy -= ry * 0.5f;
					break;
				case PT_WATR:
				case PT_DSTW:
					// Water is refreshing
					if (sim->rng.chance(1, 100))
					{
						parts[i].life = std::min(parts[i].life + 10, 200);
					}
					break;
				case PT_ACID:
					// Acid is deadly!
					if (sim->rng.chance(1, 10))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				default:
					break;
				}

				// Move toward food
				if (rt == PT_PLNT || rt == PT_YEST || rt == PT_VINE)
				{
					parts[i].vx += rx * 0.05f;
					parts[i].vy += ry * 0.05f;
				}
			}
		}
	}

	// Big blobs are slower
	if (parts[i].tmp > 30)
	{
		parts[i].vx *= 0.95f;
		parts[i].vy *= 0.95f;
	}

	// Limit size
	if (parts[i].tmp > 100)
		parts[i].tmp = 100;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int size = cpart->tmp;
	int hunger = cpart->life;

	// Pink when healthy, pale when hungry
	*colr = 255;
	*colg = 105 + hunger / 2;
	*colb = 180 + hunger / 3;

	if (*colg > 200) *colg = 200;
	if (*colb > 230) *colb = 230;

	// Bigger blobs glow more
	if (size > 20)
	{
		*firea = size / 2;
		*firer = 255;
		*fireg = 150;
		*fireb = 200;
		*pixel_mode |= FIRE_ADD;
	}

	// Pulsing effect when hungry
	if (hunger < 50)
	{
		int pulse = (hunger % 10) * 5;
		*colr -= pulse;
	}

	return 0;
}
