#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_FLOT()
{
	Identifier = "DEFAULT_PT_FLOT";
	Name = "FLOT";
	Colour = 0x99CCFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Float strength
	HeatConduct = 10;
	Description = "Floaty. Anti-gravity field! Makes everything nearby float upward.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 600.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int strength = parts[i].tmp;
	if (strength <= 0) strength = 100;

	// Anti-gravity field - affects everything in radius
	int range = 3 + strength / 30;
	if (range > 8) range = 8;

	for (int rx = -range; rx <= range; rx++)
	{
		for (int ry = -range; ry <= range; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				float dist = sqrtf(rx*rx + ry*ry);
				if (dist > range)
					continue;

				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Don't affect other floaty or immovable things
				if (rt == PT_FLOT || rt == PT_DMND || rt == PT_CLNE || rt == PT_VOID)
					continue;

				// Anti-gravity force (stronger closer)
				float force = (strength / 100.0f) * (1 - dist / range) * 0.3f;

				// Apply upward force
				parts[rID].vy -= force;

				// Slight inward pull (toward the floaty)
				if (dist > 1)
				{
					parts[rID].vx -= rx * force * 0.05f;
					parts[rID].vy -= ry * force * 0.05f;
				}

				// Cap upward velocity
				if (parts[rID].vy < -5)
					parts[rID].vy = -5;
			}
		}
	}

	// Powered by nearby spark
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (r && TYP(r) == PT_SPRK)
				{
					parts[i].tmp = std::min(parts[i].tmp + 10, 200);
				}
			}
		}
	}

	// Slowly loses power
	if (sim->rng.chance(1, 100))
	{
		parts[i].tmp = std::max(parts[i].tmp - 1, 20);
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int strength = cpart->tmp;

	// Light blue
	*colr = 150;
	*colg = 200;
	*colb = 255;

	// Brighter when stronger
	if (strength > 100)
	{
		int boost = (strength - 100) / 2;
		*colr = std::min(150 + boost, 200);
		*colg = std::min(200 + boost, 240);
		*colb = 255;
	}

	// Floating field glow
	*firea = 50 + strength / 2;
	*firer = 150;
	*fireg = 200;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
