#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PRPL()
{
	Identifier = "DEFAULT_PT_PRPL";
	Name = "PRPL";
	Colour = 0x8800FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_EXPLOSIVE;
	Enabled = 1;

	Advection = 0.9f;
	AirDrag = 0.04f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.20f;
	Collision = 0.0f;
	Gravity = -0.06f;  // Rises like fire
	Diffusion = 0.15f;
	HotAir = -0.001f * CFDS;  // Creates cold air!
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 2;

	DefaultProperties.temp = 100.0f;  // Very cold! (below freezing)
	DefaultProperties.life = 50;      // Burn time
	HeatConduct = 5;
	Description = "Purple Fire. Cold fire that freezes instead of burns! Spreads on ice.";

	Properties = TYPE_GAS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 300.0f;  // Extinguished by warmth
	HighTemperatureTransition = PT_NONE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Die when life runs out
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Die when too warm
	if (parts[i].temp > 273.0f)
	{
		parts[i].life -= 5;
	}

	// Cool surroundings intensely
	sim->pv[y/CELL][x/CELL] -= 0.02f;

	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];

				// Spread to empty space (like fire)
				if (!r && parts[i].life > 20 && sim->rng.chance(1, 30))
				{
					int np = sim->create_part(-1, x+rx, y+ry, PT_PRPL);
					if (np >= 0)
					{
						parts[np].life = parts[i].life - 10;
						parts[np].temp = parts[i].temp;
					}
					continue;
				}

				if (!r)
					continue;

				auto rt = TYP(r);
				auto rID = ID(r);

				// FREEZE things instead of burn!
				parts[rID].temp -= 20.0f;

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
					// Freeze water instantly!
					sim->part_change_type(rID, x+rx, y+ry, PT_ICEI);
					parts[i].life += 10;  // Gains energy from freezing
					break;
				case PT_ICEI:
				case PT_SNOW:
					// Spread on ice!
					if (sim->rng.chance(1, 20))
					{
						int np = sim->create_part(-1, x+rx+sim->rng.between(-1,1), y+ry+sim->rng.between(-1,1), PT_PRPL);
						if (np >= 0)
						{
							parts[np].life = 30;
							parts[np].temp = parts[i].temp;
						}
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					{
						// Cancel out with regular fire!
						sim->kill_part(rID);
						parts[i].life -= 20;
						// Create steam from the conflict
						int np = sim->create_part(-1, x+rx, y+ry, PT_WTRV);
						if (np >= 0)
						{
							parts[np].temp = 373.0f;
						}
					}
					break;
				case PT_LAVA:
					// Turn lava to stone!
					sim->part_change_type(rID, x+rx, y+ry, PT_STNE);
					parts[i].life += 20;
					break;
				case PT_OIL:
				case PT_DESL:
					// Freeze oil into a solid
					sim->part_change_type(rID, x+rx, y+ry, PT_ICEI);
					break;
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Freezes stickmen!
					parts[rID].temp -= 10.0f;
					if (parts[rID].temp < 250.0f)
					{
						// Frozen solid!
						parts[rID].life -= 5;
					}
					break;
				case PT_PLNT:
				case PT_VINE:
				case PT_WOOD:
					// Flash freeze plants
					if (sim->rng.chance(1, 20))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_ICEI);
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Float upward erratically
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.05f;
		parts[i].vy -= sim->rng.between(0, 10) * 0.02f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int life = cpart->life;

	// Purple/blue cold fire
	*colr = 100 + life;
	*colg = 0;
	*colb = 200 + life;

	if (*colr > 180) *colr = 180;
	if (*colb > 255) *colb = 255;

	// Intense cold glow
	*firea = 150 + life;
	*firer = 100;
	*fireg = 50;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	// Flicker
	if (life % 5 < 2)
	{
		*firea += 50;
	}

	return 0;
}
