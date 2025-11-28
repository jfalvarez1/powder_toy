#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_JELL()
{
	Identifier = "DEFAULT_PT_JELL";
	Name = "JELL";
	Colour = 0xFF4444_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.2f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = -0.6f;  // Very bouncy!
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 10;

	Weight = 30;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Color
	DefaultProperties.tmp2 = 0;  // Jiggle state
	HeatConduct = 20;
	Description = "Jelly. Jiggly, wiggly, and bouncy! Comes in fun colors.";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 350.0f;  // Melts at low temp
	HighTemperatureTransition = PT_WATR;

	Update = &update;
	Graphics = &graphics;
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	// Random jelly color
	sim->parts[i].tmp = sim->rng.between(0, 5);
	sim->parts[i].tmp2 = 0;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Jiggle animation
	parts[i].tmp2 = (parts[i].tmp2 + 1) % 20;

	int jellyNeighbors = 0;

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

				if (rt == PT_JELL)
				{
					jellyNeighbors++;
					// Jelly cohesion - stick together
					float dx = parts[rID].x - parts[i].x;
					float dy = parts[rID].y - parts[i].y;
					float dist = sqrtf(dx*dx + dy*dy);

					if (dist > 1.5f)
					{
						// Pull together
						parts[i].vx += dx * 0.05f;
						parts[i].vy += dy * 0.05f;
					}

					// Jiggle together!
					if (parts[i].tmp2 < 10)
					{
						parts[i].vy -= 0.1f;
					}
					else
					{
						parts[i].vy += 0.05f;
					}
				}

				// Bounce off things
				if (rt != PT_JELL)
				{
					float speed = fabsf(parts[i].vx) + fabsf(parts[i].vy);
					if (speed > 0.5f)
					{
						// Super bouncy!
						if (rx != 0) parts[i].vx *= -0.8f;
						if (ry != 0) parts[i].vy *= -0.8f;

						// Activate jiggle
						parts[i].tmp2 = 0;
					}
				}
			}
		}
	}

	// Lone jelly particles are less stable
	if (jellyNeighbors == 0)
	{
		// Random jiggle
		if (sim->rng.chance(1, 5))
		{
			parts[i].vx += sim->rng.between(-10, 10) * 0.02f;
			parts[i].vy += sim->rng.between(-10, 10) * 0.02f;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int color = cpart->tmp;
	int jiggle = cpart->tmp2;

	// Fun jelly colors
	switch (color)
	{
	case 0:  // Red
		*colr = 255; *colg = 68; *colb = 68;
		break;
	case 1:  // Green
		*colr = 68; *colg = 255; *colb = 68;
		break;
	case 2:  // Blue
		*colr = 68; *colg = 68; *colb = 255;
		break;
	case 3:  // Yellow
		*colr = 255; *colg = 255; *colb = 68;
		break;
	case 4:  // Orange
		*colr = 255; *colg = 150; *colb = 50;
		break;
	case 5:  // Purple
		*colr = 200; *colg = 68; *colb = 255;
		break;
	}

	// Jiggly brightness variation
	int wobble = (jiggle < 10) ? jiggle : 20 - jiggle;
	*colr = std::min(*colr + wobble * 3, 255);
	*colg = std::min(*colg + wobble * 3, 255);
	*colb = std::min(*colb + wobble * 3, 255);

	// Translucent glow
	*firea = 40;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD;

	// Semi-transparent
	*pixel_mode |= PMODE_BLEND;
	*cola = 180;

	return 0;
}
