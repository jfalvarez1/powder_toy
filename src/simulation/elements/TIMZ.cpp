#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_TIMZ()
{
	Identifier = "DEFAULT_PT_TIMZ";
	Name = "TIMZ";
	Colour = 0x9933FF_rgb;
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
	DefaultProperties.tmp = 0;  // Animation frame
	HeatConduct = 0;
	Description = "Time Zone. Slows down all particles in its vicinity dramatically!";

	Properties = TYPE_SOLID;

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
	// Animation counter
	parts[i].tmp = (parts[i].tmp + 1) % 60;

	// Time dilation field - 5 cell radius
	int range = 5;
	for (int rx = -range; rx <= range; rx++)
	{
		for (int ry = -range; ry <= range; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				// Circular field
				float dist = sqrtf(rx*rx + ry*ry);
				if (dist > range)
					continue;

				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Don't affect other time zones or special elements
				if (rt == PT_TIMZ || rt == PT_DMND || rt == PT_CLNE || rt == PT_VOID)
					continue;

				// Slow down particles based on distance
				float slowFactor = 0.3f + (dist / range) * 0.5f;

				parts[rID].vx *= slowFactor;
				parts[rID].vy *= slowFactor;

				// Also slow temperature changes (freeze time on heat)
				float tempDiff = parts[rID].temp - R_TEMP - 273.15f;
				parts[rID].temp = R_TEMP + 273.15f + tempDiff * 0.99f;
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int frame = cpart->tmp;

	// Purple pulsing effect
	int pulse = (frame < 30) ? frame : 60 - frame;

	*colr = 153 + pulse;
	*colg = 51 + pulse / 2;
	*colb = 255;

	// Temporal glow field
	*firea = 80 + pulse;
	*firer = 150;
	*fireg = 50;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
