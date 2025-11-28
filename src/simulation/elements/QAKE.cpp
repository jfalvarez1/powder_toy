#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_QAKE()
{
	Identifier = "DEFAULT_PT_QAKE";
	Name = "QAKE";
	Colour = 0x886644_rgb;
	MenuVisible = 1;
	MenuSection = SC_FORCE;
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
	Meltable = 0;
	Hardness = 80;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Quake timer
	DefaultProperties.tmp2 = 50; // Power level
	HeatConduct = 100;
	Description = "Quake. Causes earthquakes! Shakes nearby particles violently.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_KILL_DEC;

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
	bool activated = false;

	// Check for activation (spark)
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (r && TYP(r) == PT_SPRK)
				{
					activated = true;
					parts[i].tmp = 30;  // Start quake
				}
			}
		}
	}

	// Also trigger from nearby quakes
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (r && TYP(r) == PT_QAKE)
				{
					int rID = ID(r);
					if (parts[rID].tmp > 20 && parts[i].tmp == 0)
					{
						parts[i].tmp = 25;  // Chain reaction!
					}
				}
			}
		}
	}

	// QUAKE IN PROGRESS
	if (parts[i].tmp > 0)
	{
		parts[i].tmp--;
		int power = parts[i].tmp2;
		int intensity = parts[i].tmp;

		// Shake everything nearby!
		int range = 5 + power / 20;

		for (int rx = -range; rx <= range; rx++)
		{
			for (int ry = -range; ry <= range; ry++)
			{
				if (rx || ry)
				{
					if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
						continue;

					auto r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					auto rt = TYP(r);
					auto rID = ID(r);

					if (rt == PT_QAKE || rt == PT_DMND || rt == PT_CLNE || rt == PT_VOID)
						continue;

					float dist = sqrtf(rx*rx + ry*ry);
					float shakePower = (intensity / 30.0f) * (power / 50.0f) * (1 - dist / range);

					// Random violent shaking
					parts[rID].vx += sim->rng.between(-100, 100) * 0.01f * shakePower;
					parts[rID].vy += sim->rng.between(-100, 100) * 0.01f * shakePower;

					// Break apart brittle things
					if (rt == PT_GLAS || rt == PT_BRCK || rt == PT_CNCT)
					{
						if (sim->rng.chance((int)(shakePower * 10), 100))
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_DUST);
							parts[rID].vx = sim->rng.between(-5, 5);
							parts[rID].vy = sim->rng.between(-5, 5);
						}
					}

					// Shake loose powder
					if (rt == PT_SAND || rt == PT_DUST || rt == PT_STNE)
					{
						parts[rID].vy += shakePower * 2;
					}
				}
			}
		}

		// Pressure waves
		sim->pv[y/CELL][x/CELL] += intensity * power / 100.0f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int quaking = cpart->tmp;
	int power = cpart->tmp2;

	// Brown/gray stone color
	*colr = 136;
	*colg = 102;
	*colb = 68;

	if (quaking > 0)
	{
		// Red glow when active
		int glow = quaking * power / 30;
		*colr = std::min(136 + glow, 255);
		*colg = std::min(102 + glow / 3, 150);
		*colb = 68;

		*firea = glow;
		*firer = 255;
		*fireg = 100;
		*fireb = 50;
		*pixel_mode |= FIRE_ADD;

		// Visible shaking
		if (quaking > 15)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}

	return 0;
}
