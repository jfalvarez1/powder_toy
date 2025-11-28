#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_TNGL()
{
	Identifier = "DEFAULT_PT_TNGL";
	Name = "TNGL";
	Colour = 0x228B22_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIFE;
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

	Flammable = 50;
	Explosive = 0;
	Meltable = 0;
	Hardness = 10;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Growth direction
	DefaultProperties.life = 100;  // Growth energy
	HeatConduct = 20;
	Description = "Tangle Vine. Grows rapidly, grabs and entangles particles. Very clingy!";

	Properties = TYPE_SOLID | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_DUST;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Wither without life energy
	if (parts[i].life <= 0)
	{
		if (sim->rng.chance(1, 50))
		{
			sim->part_change_type(i, x, y, PT_DUST);
			return 0;
		}
	}

	bool hasWater = false;
	bool hasLight = (y < 50);  // Near top = has light

	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];

				// Grow into empty spaces
				if (!r && parts[i].life > 30)
				{
					// Growth direction preference
					int growChance = 100;
					if (ry < 0) growChance = 30;  // Grows upward preferentially
					if (ry > 0) growChance = 200;  // Slower downward

					if (sim->rng.chance(1, growChance))
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_TNGL);
						if (np >= 0)
						{
							parts[np].life = parts[i].life - 20;
							parts[np].tmp = (rx > 0) ? 1 : ((rx < 0) ? -1 : 0);
							parts[i].life -= 10;
						}
					}
					continue;
				}

				if (!r)
					continue;

				auto rt = TYP(r);
				auto rID = ID(r);

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
					hasWater = true;
					// Absorb water for growth
					if (sim->rng.chance(1, 50))
					{
						sim->kill_part(rID);
						parts[i].life = std::min(parts[i].life + 30, 200);
					}
					break;
				case PT_PLNT:
				case PT_VINE:
					// Overgrow other plants
					if (sim->rng.chance(1, 200))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_TNGL);
						parts[rID].life = 50;
					}
					break;
				case PT_TNGL:
					// Share nutrients
					if (parts[rID].life < parts[i].life - 20)
					{
						parts[i].life -= 5;
						parts[rID].life += 5;
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
					// Burn!
					sim->part_change_type(i, x, y, PT_FIRE);
					parts[i].life = 30;
					return 0;
				default:
					// GRAB moving particles!
					{
						float speed = fabsf(parts[rID].vx) + fabsf(parts[rID].vy);
						if (speed > 0.1f)
						{
							// Entangle!
							parts[rID].vx *= 0.3f;
							parts[rID].vy *= 0.3f;
						}

						// Crush weak particles
						if (rt == PT_DUST || rt == PT_SAND)
						{
							if (sim->rng.chance(1, 100))
							{
								sim->kill_part(rID);
								parts[i].life += 5;
							}
						}
					}
					break;
				}
			}
		}
	}

	// Photosynthesis
	if (hasLight && hasWater && sim->rng.chance(1, 50))
	{
		parts[i].life = std::min(parts[i].life + 5, 200);
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int energy = cpart->life;

	// Forest green, darker when wilting
	*colr = 34 + energy / 4;
	*colg = 139 + energy / 3;
	*colb = 34 + energy / 5;

	if (*colr > 80) *colr = 80;
	if (*colg > 180) *colg = 180;
	if (*colb > 80) *colb = 80;

	// Healthy vines glow slightly
	if (energy > 100)
	{
		*firea = energy / 5;
		*firer = 50;
		*fireg = 150;
		*fireb = 50;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
