#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MIRR()
{
	Identifier = "DEFAULT_PT_MIRR";
	Name = "MIRR";
	Colour = 0xC0C0C0_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 30;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	HeatConduct = 200;
	Description = "Mirror. Reflects particles back in the opposite direction!";

	Properties = TYPE_SOLID;

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
}

static int update(UPDATE_FUNC_ARGS)
{
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

				// Don't reflect solids or special elements
				if (rt == PT_MIRR || rt == PT_CLNE || rt == PT_VOID || rt == PT_DMND)
					continue;

				// Check if particle is moving toward the mirror
				float vx = parts[rID].vx;
				float vy = parts[rID].vy;
				float speed = sqrtf(vx*vx + vy*vy);

				if (speed > 0.1f)
				{
					// Determine which face was hit and reflect
					if (rx != 0 && fabsf(vx) > 0.05f)
					{
						// Hit vertical face - reflect X
						parts[rID].vx = -vx * 0.95f;
					}
					if (ry != 0 && fabsf(vy) > 0.05f)
					{
						// Hit horizontal face - reflect Y
						parts[rID].vy = -vy * 0.95f;
					}

					// Push particle away from mirror
					parts[rID].x += rx * 0.5f;
					parts[rID].y += ry * 0.5f;

					// Photons reflect perfectly
					if (rt == PT_PHOT || rt == PT_NEUT || rt == PT_PROT || rt == PT_ELEC)
					{
						parts[rID].vx = -vx;
						parts[rID].vy = -vy;
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Shiny silver appearance
	*colr = 192;
	*colg = 192;
	*colb = 200;

	// Reflective shine effect
	*firea = 30;
	*firer = 255;
	*fireg = 255;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
