#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_PLZM()
{
	Identifier = "DEFAULT_PT_PLZM";
	Name = "PLZM";
	Colour = 0xFF80FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_NUCLEAR;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.03f;  // Slightly floats
	Diffusion = 0.05f;
	HotAir = 0.005f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = 8000.0f + 273.15f;  // Extremely hot
	DefaultProperties.life = 300;  // Duration
	DefaultProperties.tmp = 0;     // Color phase
	HeatConduct = 5;  // Poor conduction (stays hot)
	Description = "Plasma Ball. Superheated matter, extremely hot, floats and emits radiation. Created by NEUT hitting gases.";

	Properties = TYPE_GAS | PROP_LIFE_DEC | PROP_LIFE_KILL;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 5000.0f;  // Becomes regular plasma when cooled
	LowTemperatureTransition = PT_PLSM;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.between(200, 400);
	sim->parts[i].tmp = sim->rng.between(0, 360);  // Random color phase
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// Color cycling
	parts[i].tmp = (parts[i].tmp + 3) % 360;

	// Emit light constantly
	if (sim->rng.chance(1, 5))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_PHOT);
		if (np >= 0)
		{
			// Color based on phase
			int hue = parts[i].tmp;
			if (hue < 120)
				parts[np].ctype = 0x00FF00FF;  // Magenta
			else if (hue < 240)
				parts[np].ctype = 0x00FFFFFF;  // Cyan
			else
				parts[np].ctype = 0x00FFFF00;  // Yellow

			parts[np].vx = sim->rng.between(-5, 5) * 0.5f;
			parts[np].vy = sim->rng.between(-5, 5) * 0.5f;
			parts[np].temp = parts[i].temp;
		}
	}

	// Emit neutrons occasionally (fusion!)
	if (sim->rng.chance(1, 200))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_NEUT);
		if (np >= 0)
		{
			parts[np].vx = sim->rng.between(-5, 5);
			parts[np].vy = sim->rng.between(-5, 5);
			parts[np].temp = parts[i].temp;
		}
	}

	// Erratic floating movement
	if (sim->rng.chance(1, 10))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.05f;
		parts[i].vy += sim->rng.between(-10, 10) * 0.05f;
	}

	// Limit velocity
	float speed = sqrtf(parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy);
	if (speed > 1.5f)
	{
		parts[i].vx *= 1.5f / speed;
		parts[i].vy *= 1.5f / speed;
	}

	// Heat nearby air
	sim->pv[y/CELL][x/CELL] += 0.1f;

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

				// Heat everything nearby
				if (parts[rID].temp < parts[i].temp)
				{
					float diff = parts[i].temp - parts[rID].temp;
					parts[rID].temp += diff * 0.1f;
				}

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
					// Instantly vaporize water
					sim->part_change_type(rID, x+rx, y+ry, PT_WTRV);
					parts[rID].temp = 500.0f + 273.15f;
					sim->pv[y/CELL][x/CELL] += 5.0f;  // Steam explosion
					break;

				case PT_O2:
				case PT_H2:
					// Gases become plasma
					if (sim->rng.chance(1, 5))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_PLSM);
						parts[rID].life = sim->rng.between(50, 100);
					}
					break;

				case PT_DEUT:
					// Deuterium fusion! Create more plasma and neutrons
					if (sim->rng.chance(1, 20))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_PLZM);
						parts[rID].life = sim->rng.between(100, 200);
						parts[rID].temp = 10000.0f + 273.15f;
						// Fusion releases neutrons
						sim->create_part(-1, x+rx, y+ry, PT_NEUT);
						sim->pv[y/CELL][x/CELL] += 10.0f;
					}
					break;

				case PT_URAN:
				case PT_PLUT:
					// Trigger fission chain reaction!
					if (sim->rng.chance(1, 10))
					{
						for (int n = 0; n < 3; n++)
						{
							int np = sim->create_part(-1, x+rx+sim->rng.between(-1,1), y+ry+sim->rng.between(-1,1), PT_NEUT);
							if (np >= 0)
							{
								parts[np].vx = sim->rng.between(-5, 5);
								parts[np].vy = sim->rng.between(-5, 5);
							}
						}
						sim->pv[y/CELL][x/CELL] += 20.0f;
						parts[rID].temp = 9000.0f + 273.15f;
					}
					break;

				case PT_METL:
				case PT_IRON:
				case PT_BMTL:
					// Melt metals
					if (parts[rID].temp < elements[rt].HighTemperature)
					{
						parts[rID].temp += 500.0f;
						if (parts[rID].temp >= elements[rt].HighTemperature && elements[rt].HighTemperatureTransition == PT_LAVA)
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_LAVA);
							parts[rID].ctype = rt;
							parts[rID].life = sim->rng.between(240, 359);
						}
					}
					break;

				case PT_PLZM:
					// Plasma balls merge (slightly)
					if (parts[rID].life < parts[i].life - 50)
					{
						parts[rID].life += 10;
						parts[i].life -= 5;
					}
					break;

				case PT_CRYO:
					// Extreme temperature differential - explosion!
					sim->pv[y/CELL][x/CELL] += 30.0f;
					parts[i].life -= 50;
					sim->kill_part(rID);
					break;

				default:
					// Ignite flammable things
					if (elements[rt].Flammable > 0)
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_FIRE);
						parts[rID].life = sim->rng.between(100, 200);
						parts[rID].temp = 2000.0f + 273.15f;
					}
					break;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int phase = cpart->tmp;

	// Cycling colors based on phase
	float h = phase / 60.0f;
	int hi = (int)h % 6;
	float f = h - (int)h;

	int v = 255;
	int p = 100;
	int q = 255 - (int)(155 * f);
	int t = 100 + (int)(155 * f);

	switch (hi)
	{
	case 0: *colr = v; *colg = t; *colb = p; break;
	case 1: *colr = q; *colg = v; *colb = p; break;
	case 2: *colr = p; *colg = v; *colb = t; break;
	case 3: *colr = p; *colg = q; *colb = v; break;
	case 4: *colr = t; *colg = p; *colb = v; break;
	case 5: *colr = v; *colg = p; *colb = q; break;
	}

	// Intense glow
	*firea = 255;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode = PMODE_NONE;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW | PMODE_BLUR;

	// Pulsing effect
	int pulse = (cpart->life % 20) * 5;
	*firea = 200 + pulse / 2;

	return 0;
}
