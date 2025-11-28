#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_GLTR()
{
	Identifier = "DEFAULT_PT_GLTR";
	Name = "GLTR";
	Colour = 0xFFD700_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.7f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.96f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.05f;  // Very light
	Diffusion = 0.20f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 0;

	Weight = 5;  // Super light, floats around

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Color (0-5)
	DefaultProperties.tmp2 = 0;  // Sparkle timer
	HeatConduct = 200;
	Description = "Glitter. Sparkly particles that stick to everything and spread everywhere!";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1500.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = sim->rng.between(0, 5);  // Random color
	sim->parts[i].tmp2 = sim->rng.between(0, 20);  // Random sparkle phase
}

static int update(UPDATE_FUNC_ARGS)
{
	// Sparkle animation
	parts[i].tmp2 = (parts[i].tmp2 + 1) % 20;

	// Glitter sticks to things!
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Don't stick to other glitter or liquids
				if (rt == PT_GLTR || rt == PT_WATR || rt == PT_DSTW || rt == PT_OIL ||
				    rt == PT_CLNE || rt == PT_VOID)
					continue;

				// Stick to solids!
				if (sim->rng.chance(1, 30))
				{
					parts[i].vx *= 0.1f;
					parts[i].vy *= 0.1f;
				}

				// Transfer glitter to moving things (it gets everywhere!)
				float speed = fabsf(parts[rID].vx) + fabsf(parts[rID].vy);
				if (speed > 0.5f && sim->rng.chance(1, 50))
				{
					// Spawn new glitter near the moving particle
					int np = sim->create_part(-1, x+rx+sim->rng.between(-1,1), y+ry+sim->rng.between(-1,1), PT_GLTR);
					if (np >= 0)
					{
						parts[np].tmp = parts[i].tmp;  // Same color
						parts[np].vx = parts[rID].vx * 0.5f;
						parts[np].vy = parts[rID].vy * 0.5f;
					}
				}

				// Stickmen get covered in glitter
				if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH)
				{
					if (sim->rng.chance(1, 100))
					{
						int np = sim->create_part(-1, x+rx+sim->rng.between(-2,2), y+ry+sim->rng.between(-2,2), PT_GLTR);
						if (np >= 0)
						{
							parts[np].tmp = parts[i].tmp;
						}
					}
				}
			}
		}
	}

	// Float around prettily
	if (sim->rng.chance(1, 20))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
		parts[i].vy += sim->rng.between(-10, 10) * 0.02f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int color = cpart->tmp;
	int sparkle = cpart->tmp2;

	// Different colors of glitter
	switch (color)
	{
	case 0:  // Gold
		*colr = 255; *colg = 215; *colb = 0;
		break;
	case 1:  // Silver
		*colr = 200; *colg = 200; *colb = 220;
		break;
	case 2:  // Red
		*colr = 255; *colg = 50; *colb = 50;
		break;
	case 3:  // Blue
		*colr = 50; *colg = 100; *colb = 255;
		break;
	case 4:  // Green
		*colr = 50; *colg = 255; *colb = 100;
		break;
	case 5:  // Pink
		*colr = 255; *colg = 100; *colb = 200;
		break;
	}

	// Sparkle effect!
	if (sparkle < 5)
	{
		*colr = 255;
		*colg = 255;
		*colb = 255;
		*firea = 100;
		*firer = 255;
		*fireg = 255;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD | PMODE_GLOW;
	}
	else
	{
		*firea = 30;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
