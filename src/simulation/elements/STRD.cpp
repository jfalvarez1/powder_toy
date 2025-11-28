#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_STRD()
{
	Identifier = "DEFAULT_PT_STRD";
	Name = "STRD";
	Colour = 0xFFFF99_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.05f;  // Floats up
	Diffusion = 0.20f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Twinkle phase
	DefaultProperties.tmp2 = 0;  // Wish charges
	HeatConduct = 0;
	Description = "Stardust. Magical sparkling dust from space. Grants wishes (random effects)!";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1000.0f;
	HighTemperatureTransition = PT_PLSM;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = sim->rng.between(0, 50);
	sim->parts[i].tmp2 = 3;  // 3 wish charges
}

static int update(UPDATE_FUNC_ARGS)
{
	// Twinkle animation
	parts[i].tmp = (parts[i].tmp + 1) % 50;

	// Float gracefully
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-15, 5) * 0.02f;
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

				// Grant wishes to living things!
				if ((rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH) && parts[i].tmp2 > 0)
				{
					if (sim->rng.chance(1, 30))
					{
						// Random wish effect!
						int wish = sim->rng.between(0, 7);
						switch (wish)
						{
						case 0:  // Health
							parts[rID].life = 100;
							break;
						case 1:  // Speed
							parts[rID].vx *= 2;
							parts[rID].vy *= 2;
							break;
						case 2:  // Gold shower
							for (int j = 0; j < 5; j++)
							{
								sim->create_part(-1, x + sim->rng.between(-3, 3), y - sim->rng.between(1, 3), PT_GOLD);
							}
							break;
						case 3:  // Protection (diamond armor?)
							sim->create_part(-1, x + rx, y + ry - 1, PT_DMND);
							break;
						case 4:  // Food (cake!)
							sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_CAKE);
							break;
						case 5:  // Fire immunity (cool down)
							parts[rID].temp = 300.0f;
							break;
						case 6:  // Flight (upward boost)
							parts[rID].vy = -10;
							break;
						case 7:  // More stardust!
							for (int j = 0; j < 3; j++)
							{
								int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_STRD);
								if (np >= 0)
								{
									parts[np].tmp2 = 1;
								}
							}
							break;
						}

						parts[i].tmp2--;
						if (parts[i].tmp2 <= 0)
						{
							// No more wishes - becomes regular sparkle
							sim->kill_part(i);
							return 1;
						}
					}
				}

				// Other stardust combines charges
				if (rt == PT_STRD && sim->rng.chance(1, 100))
				{
					if (parts[rID].tmp2 < parts[i].tmp2)
					{
						parts[rID].tmp2++;
						parts[i].tmp2--;
					}
				}
			}
		}
	}

	// Slowly fade
	if (sim->rng.chance(1, 2000))
	{
		sim->kill_part(i);
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int twinkle = cpart->tmp;
	int wishes = cpart->tmp2;

	// Golden stardust
	*colr = 255;
	*colg = 255;
	*colb = 150;

	// Twinkling effect
	int pulse = (twinkle % 25);
	pulse = (pulse < 12) ? pulse : 25 - pulse;

	*firea = 80 + pulse * 10 + wishes * 20;
	*firer = 255;
	*fireg = 255;
	*fireb = 200;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	// Extra bright when twinkling
	if (twinkle < 5)
	{
		*colr = 255;
		*colg = 255;
		*colb = 255;
		*firea = 200;
	}

	return 0;
}
