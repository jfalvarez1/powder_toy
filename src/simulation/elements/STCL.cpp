#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_STCL()
{
	Identifier = "DEFAULT_PT_STCL";
	Name = "STCL";
	Colour = 0x404050_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = 0.0f;
	Gravity = -0.1f;  // Rises like a cloud
	Diffusion = 0.70f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = 273.15f;  // Cold cloud
	DefaultProperties.life = 2000;     // Lasts a while
	DefaultProperties.tmp = 0;         // Charge buildup
	HeatConduct = 100;
	Description = "Storm Cloud. Produces rain and lightning. Builds electrical charge over time.";

	Properties = TYPE_GAS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_SNOW;  // Very cold = snow
	HighTemperature = 400.0f;
	HighTemperatureTransition = PT_WTRV;  // Hot = evaporates

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].life = sim->rng.between(1500, 2500);
	sim->parts[i].tmp = sim->rng.between(0, 50);  // Initial charge
}

static int update(UPDATE_FUNC_ARGS)
{
	// Build up electrical charge over time
	if (sim->rng.chance(1, 10))
	{
		parts[i].tmp = std::min(parts[i].tmp + 1, 200);
	}

	// Clouds clump together
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

				switch (rt)
				{
				case PT_STCL:
					// Clouds attract each other
					parts[i].vx += rx * 0.01f;
					parts[i].vy += ry * 0.01f;
					// Share charge
					if (parts[i].tmp > parts[rID].tmp + 10)
					{
						int transfer = (parts[i].tmp - parts[rID].tmp) / 4;
						parts[i].tmp -= transfer;
						parts[rID].tmp += transfer;
					}
					break;
				case PT_WTRV:
				case PT_FOG:
					// Absorb water vapor - grow the cloud
					if (sim->rng.chance(1, 50))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_STCL);
						parts[rID].life = sim->rng.between(1000, 2000);
						parts[rID].tmp = parts[i].tmp / 2;
					}
					break;
				case PT_WATR:
				case PT_DSTW:
					// Absorb water (evaporate into cloud)
					if (sim->rng.chance(1, 100))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_STCL);
						parts[rID].life = sim->rng.between(500, 1000);
						parts[rID].tmp = 0;
					}
					break;
				case PT_METL:
				case PT_IRON:
				case PT_BMTL:
				case PT_INWR:
					// Lightning strikes metal! (if charged enough)
					if (parts[i].tmp >= 100 && sim->rng.chance(1, 20))
					{
						// Create lightning bolt toward metal
						int lx = x, ly = y;
						for (int step = 0; step < 10; step++)
						{
							int dx = (rx > 0) ? 1 : ((rx < 0) ? -1 : 0);
							int dy = (ry > 0) ? 1 : ((ry < 0) ? -1 : 0);
							// Add some randomness
							dx += sim->rng.between(-1, 1);
							dy += sim->rng.between(-1, 1);
							lx += dx;
							ly += dy;
							if (lx >= 0 && lx < XRES && ly >= 0 && ly < YRES)
							{
								sim->create_part(-1, lx, ly, PT_THDR);
							}
						}
						parts[i].tmp = 0;  // Discharge
						sim->pv[y/CELL][x/CELL] += 5.0f;  // Thunder boom
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Rain production
	if (parts[i].life > 500)  // Only mature clouds rain
	{
		// Heavier rain when cloud is dense (high life)
		int rainChance = 500 - (parts[i].life / 10);
		if (rainChance < 50) rainChance = 50;

		if (sim->rng.chance(1, rainChance))
		{
			// Create rain below
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + 1, PT_WATR);
			if (np >= 0)
			{
				parts[np].temp = parts[i].temp;
				parts[np].vy = 1.0f;  // Falling
				parts[i].life -= 1;  // Cloud gets smaller
			}
		}
	}

	// High charge = spontaneous lightning!
	if (parts[i].tmp >= 150 && sim->rng.chance(1, 100))
	{
		// Strike down!
		int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + 1, PT_THDR);
		if (np >= 0)
		{
			parts[np].vy = 5.0f;
		}
		parts[i].tmp -= 50;
		sim->pv[y/CELL][x/CELL] += 3.0f;  // Thunder!
	}

	// Very cold clouds produce snow instead
	if (parts[i].temp < 263.15f && parts[i].life > 500)
	{
		if (sim->rng.chance(1, 300))
		{
			int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + 1, PT_SNOW);
			if (np >= 0)
			{
				parts[np].temp = parts[i].temp;
				parts[i].life -= 1;
			}
		}
	}

	// Clouds that run out of water dissipate
	if (parts[i].life <= 0)
	{
		// Discharge any remaining charge as lightning
		if (parts[i].tmp > 50)
		{
			sim->create_part(i, x, y, PT_THDR);
		}
		else
		{
			sim->create_part(i, x, y, PT_WTRV);
		}
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Darker when more charged
	int charge = cpart->tmp;
	int darkness = 80 - charge / 3;
	if (darkness < 30) darkness = 30;

	*colr = darkness;
	*colg = darkness;
	*colb = darkness + 20;

	// Lightning flicker when highly charged
	if (charge > 100 && cpart->life % 10 < 2)
	{
		*colr = 200;
		*colg = 200;
		*colb = 255;
	}

	// Cloud-like rendering
	*pixel_mode |= PMODE_BLUR;
	*pixel_mode |= FIRE_ADD;
	*firea = 20;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;

	return 0;
}
