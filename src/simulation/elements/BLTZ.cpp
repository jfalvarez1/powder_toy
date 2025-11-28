#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_BLTZ()
{
	Identifier = "DEFAULT_PT_BLTZ";
	Name = "BLTZ";
	Colour = 0xAAAAFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_ELEC;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.90f;
	Collision = 0.0f;
	Gravity = -0.02f;  // Slightly floats upward
	Diffusion = 0.20f;  // Moves erratically
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = 5000.0f + 273.15f;  // Very hot
	DefaultProperties.life = 500;  // Lasts a while
	HeatConduct = 5;  // Poor heat conduction (stays hot)
	Description = "Ball Lightning. Floats erratically, zaps conductors, and creates sparks. Rare phenomenon.";

	Properties = TYPE_ENERGY | PROP_LIFE_DEC | PROP_LIFE_KILL;

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
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// Erratic movement - ball lightning is unpredictable
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.1f;
		parts[i].vy += sim->rng.between(-10, 10) * 0.1f;
	}

	// Limit velocity
	float speed = sqrtf(parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy);
	if (speed > 2.0f)
	{
		parts[i].vx *= 2.0f / speed;
		parts[i].vy *= 2.0f / speed;
	}

	// Emit light (photons)
	if (sim->rng.chance(1, 20))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_PHOT);
		if (np >= 0)
		{
			parts[np].ctype = 0x00FFFFFF;  // White light
			parts[np].vx = sim->rng.between(-5, 5) * 0.5f;
			parts[np].vy = sim->rng.between(-5, 5) * 0.5f;
		}
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

				// Spark conductors
				if ((elements[rt].Properties & PROP_CONDUCTS) && parts[rID].life == 0)
				{
					if (sim->rng.chance(1, 5))
					{
						parts[rID].ctype = rt;
						sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
						parts[rID].life = 4;

						// Create thunder/lightning arc
						if (sim->rng.chance(1, 10))
						{
							sim->create_part(-1, x+rx, y+ry, PT_THDR);
						}
					}
				}
				// Attracted to grounded metals
				else if (rt == PT_METL || rt == PT_IRON || rt == PT_BMTL)
				{
					parts[i].vx += rx * 0.05f;
					parts[i].vy += ry * 0.05f;
				}
				// Explode on contact with water
				else if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW || rt == PT_CBNW)
				{
					// Steam explosion!
					sim->pv[y/CELL][x/CELL] += 10.0f;
					sim->part_change_type(rID, x+rx, y+ry, PT_WTRV);
					parts[rID].temp = 500.0f + 273.15f;

					// Create thunder
					if (sim->rng.chance(1, 3))
					{
						sim->create_part(-1, x, y, PT_THDR);
					}

					// Sometimes explode
					if (sim->rng.chance(1, 10))
					{
						parts[i].life = 0;  // Will die
						sim->pv[y/CELL][x/CELL] += 30.0f;
					}
				}
				// Ignite flammable materials
				else if (elements[rt].Flammable > 0 && sim->rng.chance(1, 20))
				{
					sim->part_change_type(rID, x+rx, y+ry, PT_FIRE);
					parts[rID].life = sim->rng.between(50, 100);
				}
			}
		}
	}

	// Occasionally emit sparks/electrons
	if (sim->rng.chance(1, 50))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_ELEC);
		if (np >= 0)
		{
			parts[np].vx = sim->rng.between(-3, 3);
			parts[np].vy = sim->rng.between(-3, 3);
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Bright glowing ball effect
	int pulse = (int)(10 * sin(cpart->life * 0.1f));

	*colr = 180 + pulse;
	*colg = 180 + pulse;
	*colb = 255;

	*firea = 200;
	*firer = 150 + pulse;
	*fireg = 150 + pulse;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
