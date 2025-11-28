#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_GOOP()
{
	Identifier = "DEFAULT_PT_GOOP";
	Name = "GOOP";
	Colour = 0x00FF66_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.95f;
	Loss = 0.80f;
	Collision = -0.5f;  // Bouncy!
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 40;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Stretch state
	HeatConduct = 30;
	Description = "Goop. Stretchy, bouncy slime that sticks to things and stretches!";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 450.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int goopNeighbors = 0;
	bool touchingSolid = false;

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

				if (rt == PT_GOOP)
				{
					goopNeighbors++;
					// Goop sticks together - elastic connection
					float dx = parts[rID].x - parts[i].x;
					float dy = parts[rID].y - parts[i].y;
					float dist = sqrtf(dx*dx + dy*dy);

					if (dist > 1.5f)
					{
						// Pull together
						parts[i].vx += dx * 0.1f;
						parts[i].vy += dy * 0.1f;
						parts[i].tmp = 1;  // Stretched
					}
					else if (dist < 0.8f)
					{
						// Push apart
						parts[i].vx -= dx * 0.05f;
						parts[i].vy -= dy * 0.05f;
					}
				}

				// Stick to solids
				if (rt == PT_BRCK || rt == PT_STNE || rt == PT_METL || rt == PT_WOOD ||
				    rt == PT_GLAS || rt == PT_CNCT || rt == PT_IRON)
				{
					touchingSolid = true;
					// Reduce velocity near solids
					parts[i].vx *= 0.8f;
					parts[i].vy *= 0.8f;
				}

				// Stick to and trap particles
				if (rt == PT_DUST || rt == PT_SAND || rt == PT_COAL || rt == PT_BCOL)
				{
					parts[rID].vx *= 0.5f;
					parts[rID].vy *= 0.5f;
				}

				// Water makes goop slimier
				if (rt == PT_WATR || rt == PT_DSTW)
				{
					if (sim->rng.chance(1, 50))
					{
						// Absorb water, get bigger
						sim->kill_part(rID);
						int np = sim->create_part(-1, x+rx, y+ry, PT_GOOP);
						if (np >= 0)
						{
							parts[np].temp = parts[i].temp;
						}
					}
				}
			}
		}
	}

	// Bounce more when stretched
	if (parts[i].tmp > 0)
	{
		// Snap back!
		parts[i].vy -= 0.2f;
		parts[i].tmp = 0;
	}

	// Goop bounces off surfaces
	if (touchingSolid && (fabsf(parts[i].vx) > 0.5f || fabsf(parts[i].vy) > 0.5f))
	{
		// Bounce with energy conservation
		if (fabsf(parts[i].vx) > fabsf(parts[i].vy))
			parts[i].vx *= -0.7f;
		else
			parts[i].vy *= -0.7f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int stretch = cpart->tmp;

	// Bright green slime
	*colr = 0;
	*colg = 255;
	*colb = 102;

	// Darker when stretched
	if (stretch)
	{
		*colg = 200;
		*colb = 80;
	}

	// Gooey glow
	*firea = 40;
	*firer = 50;
	*fireg = 200;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD;

	// Semi-transparent
	*pixel_mode |= PMODE_BLEND;
	*cola = 200;

	return 0;
}
