#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_HONY()
{
	Identifier = "DEFAULT_PT_HONY";
	Name = "HONY";
	Colour = 0xFFAA00_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.1f;  // Very slow
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.99f;
	Collision = 0.0f;
	Gravity = 0.05f;  // Slow fall
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 30;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 45;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Stickiness
	HeatConduct = 20;
	Description = "Honey. Very thick and sticky! Slows everything down. Tasty!";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 260.0f;  // Crystallizes when cold
	LowTemperatureTransition = PT_PSTE;
	HighTemperature = 400.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Honey is SLOW - self-dampen
	parts[i].vx *= 0.9f;
	parts[i].vy *= 0.95f;

	// Stickiness decreases when hot, increases when cold
	if (parts[i].temp > 320.0f)
	{
		parts[i].tmp = std::max(parts[i].tmp - 1, 20);  // More runny
	}
	else if (parts[i].temp < 280.0f)
	{
		parts[i].tmp = std::min(parts[i].tmp + 1, 150);  // More sticky
	}

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

				// SUPER STICKY - slow down everything!
				float slowFactor = 1.0f - (parts[i].tmp / 200.0f);
				parts[rID].vx *= slowFactor;
				parts[rID].vy *= slowFactor;

				// Really trap small particles
				if (rt == PT_DUST || rt == PT_SPOR || rt == PT_BCOL || rt == PT_COAL)
				{
					parts[rID].vx *= 0.2f;
					parts[rID].vy *= 0.2f;
				}

				// Creatures like honey
				if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH)
				{
					parts[rID].vx *= 0.3f;
					parts[rID].vy *= 0.5f;
				}

				// Honey + water = diluted honey
				if (rt == PT_WATR || rt == PT_DSTW)
				{
					if (sim->rng.chance(1, 100))
					{
						parts[i].tmp = std::max(parts[i].tmp - 5, 20);
					}
				}

				// Honey feeds organic things
				if (rt == PT_YEST || rt == PT_PLNT)
				{
					if (sim->rng.chance(1, 200))
					{
						sim->kill_part(i);
						return 1;
					}
				}

				// Ants/swarm love honey
				if (rt == PT_SWRM)
				{
					parts[rID].vx *= 0.5f;
					parts[rID].vy *= 0.5f;
					// Swarm eats honey
					if (sim->rng.chance(1, 50))
					{
						sim->kill_part(i);
						parts[rID].life = std::min(parts[rID].life + 100, 1000);
						return 1;
					}
				}

				// Fire burns honey
				if (rt == PT_FIRE || rt == PT_PLSM)
				{
					if (sim->rng.chance(1, 30))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = 50;
						return 0;
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int sticky = cpart->tmp;
	float temp = cpart->temp;

	// Golden amber color
	*colr = 255;
	*colg = 170;
	*colb = 0;

	// Darker when thicker
	if (sticky > 100)
	{
		*colg = 140;
	}

	// Lighter when warm and runny
	if (temp > 310.0f)
	{
		*colr = 255;
		*colg = 200;
		*colb = 50;
	}

	// Golden glow
	*firea = 30;
	*firer = 255;
	*fireg = 180;
	*fireb = 0;
	*pixel_mode |= FIRE_ADD;

	// Semi-transparent
	*pixel_mode |= PMODE_BLEND;
	*cola = 220;

	return 0;
}
