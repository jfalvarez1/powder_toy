#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_BEEZ()
{
	Identifier = "DEFAULT_PT_BEEZ";
	Name = "BEEZ";
	Colour = 0xFFCC00_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIFE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 500;  // Lifespan
	DefaultProperties.tmp = 0;     // Pollen collected
	DefaultProperties.tmp2 = 0;    // Buzz animation
	HeatConduct = 20;
	Description = "Bees! Collect pollen from plants and make honey. Will sting threats!";

	Properties = TYPE_PART | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_DUST;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	parts[i].tmp2 = (parts[i].tmp2 + 1) % 10;  // Buzz animation

	// Fly around
	if (sim->rng.chance(1, 3))
	{
		parts[i].vx = sim->rng.between(-20, 20) * 0.15f;
		parts[i].vy = sim->rng.between(-20, 20) * 0.15f;
	}

	// Look for flowers/plants and hive
	int targetX = 0, targetY = 0;
	bool foundTarget = false;
	int searchRange = 15;

	for (int rx = -searchRange; rx <= searchRange && !foundTarget; rx++)
	{
		for (int ry = -searchRange; ry <= searchRange && !foundTarget; ry++)
		{
			if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
				continue;

			auto r = pmap[y+ry][x+rx];
			if (!r) continue;
			auto rt = TYP(r);

			// If carrying pollen, look for place to deposit
			if (parts[i].tmp > 0)
			{
				// Look for other bees or walls (hive)
				if (rt == PT_BRCK || rt == PT_WOOD)
				{
					targetX = rx;
					targetY = ry;
					foundTarget = true;
				}
			}
			else
			{
				// Look for plants to collect pollen
				if (rt == PT_PLNT || rt == PT_VINE)
				{
					targetX = rx;
					targetY = ry;
					foundTarget = true;
				}
			}
		}
	}

	// Move toward target
	if (foundTarget)
	{
		float dist = sqrtf(targetX*targetX + targetY*targetY);
		if (dist > 0)
		{
			parts[i].vx += targetX * 0.1f;
			parts[i].vy += targetY * 0.1f;
		}
	}

	// Nearby interactions
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
				{
					// Make honey when near hive with pollen
					if (parts[i].tmp >= 10)
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_HONY);
						if (np >= 0)
						{
							parts[np].tmp = 100;  // Fresh honey
							parts[i].tmp -= 10;
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				switch (rt)
				{
				case PT_PLNT:
				case PT_VINE:
					// Collect pollen!
					if (parts[i].tmp < 30 && sim->rng.chance(1, 20))
					{
						parts[i].tmp += 5;
						// Pollinate - spread plant
						if (sim->rng.chance(1, 50))
						{
							for (int dx = -2; dx <= 2; dx++)
							{
								for (int dy = -2; dy <= 2; dy++)
								{
									if (!pmap[y+dy][x+dx] && sim->rng.chance(1, 20))
									{
										sim->create_part(-1, x+dx, y+dy, PT_PLNT);
										goto done_pollinate;
									}
								}
							}
							done_pollinate:;
						}
					}
					break;
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// STING threats!
					if (sim->rng.chance(1, 50))
					{
						parts[rID].life -= 2;  // Sting damage
						// Bee dies after stinging
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Flee from fire!
					parts[i].vx -= rx * 2;
					parts[i].vy -= ry * 2;
					break;
				case PT_BEEZ:
					// Swarm behavior
					parts[i].vx += rx * 0.05f;
					parts[i].vy += ry * 0.05f;
					break;
				case PT_HONY:
					// Eat honey to live longer
					if (sim->rng.chance(1, 100))
					{
						parts[i].life = std::min(parts[i].life + 50, 1000);
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Reproduce when well-fed
	if (parts[i].life > 400 && parts[i].tmp > 20 && sim->rng.chance(1, 500))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_BEEZ);
		if (np >= 0)
		{
			parts[np].life = 300;
			parts[i].life -= 100;
			parts[i].tmp -= 10;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int pollen = cpart->tmp;
	int buzz = cpart->tmp2;

	// Yellow and black stripes (simulated by flicker)
	if (buzz < 5)
	{
		*colr = 255;
		*colg = 204;
		*colb = 0;
	}
	else
	{
		*colr = 50;
		*colg = 40;
		*colb = 0;
	}

	// Glow when carrying pollen
	if (pollen > 0)
	{
		*firea = 30 + pollen;
		*firer = 255;
		*fireg = 230;
		*fireb = 50;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
