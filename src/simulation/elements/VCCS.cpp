#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_VCCS()
{
	Identifier = "DEFAULT_PT_VCCS";
	Name = "VCCS";
	Colour = 0xCC0000_rgb;
	MenuVisible = 1;
	MenuSection = SC_ELEC;
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
	Hardness = 1;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 100;  // Voltage output level (1-100)
	DefaultProperties.tmp2 = 0;   // Current output counter
	DefaultProperties.life = 0;   // Spark timer
	HeatConduct = 0;
	Description = "VCC/Power supply. Provides constant voltage. Tmp sets voltage level (1-100).";

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
	int voltage = parts[i].tmp;
	if (voltage < 1) voltage = 1;
	if (voltage > 100) voltage = 100;
	parts[i].tmp = voltage;

	// Output power periodically (not every frame to avoid overheating)
	parts[i].life++;
	int outputInterval = 10 - voltage / 15;  // Higher voltage = slightly faster (but still slow)
	if (outputInterval < 5) outputInterval = 5;

	if (parts[i].life > outputInterval)
	{
		parts[i].life = 0;

		// Output spark to adjacent conductors (wires only, not directly to components)
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (r)
					{
						auto rt = TYP(r);
						auto rID = ID(r);

						// Only output to basic conductors - let them carry to components
						if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN || rt == PT_NSCN)
						    && parts[rID].life == 0)
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
							parts[rID].ctype = rt;
							parts[rID].life = 4;
							parts[i].tmp2++;
						}
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int voltage = cpart->tmp;

	// Red color, brightness based on voltage
	*colr = 150 + voltage;
	*colg = 0;
	*colb = 0;

	// Always glowing (power is always on)
	*firea = 30 + voltage / 3;
	*firer = 255;
	*fireg = 50;
	*fireb = 50;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
