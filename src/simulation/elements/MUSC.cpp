#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_MUSC()
{
	Identifier = "DEFAULT_PT_MUSC";
	Name = "MUSC";
	Colour = 0xCC6666_rgb;
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

	Flammable = 20;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Contraction state (0=relaxed, 1=contracting)
	DefaultProperties.tmp2 = 0;  // Contraction timer
	HeatConduct = 50;
	Description = "Muscle. Contracts when sparked! Can push particles around.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_KILL_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_ICEI;
	HighTemperature = 373.0f;  // Cooks
	HighTemperatureTransition = PT_DUST;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	bool sparked = false;

	// Check for electrical signal
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

				if (rt == PT_SPRK)
				{
					sparked = true;
				}
			}
		}
	}

	// Trigger contraction
	if (sparked && parts[i].tmp == 0)
	{
		parts[i].tmp = 1;  // Start contracting
		parts[i].tmp2 = 20;  // Contraction duration
	}

	// Contract and relax cycle
	if (parts[i].tmp == 1)
	{
		parts[i].tmp2--;

		// PUSH nearby particles during contraction!
		int pushForce = (parts[i].tmp2 > 10) ? 2 : -1;  // Push then relax

		for (auto rx = -2; rx <= 2; rx++)
		{
			for (auto ry = -2; ry <= 2; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					auto rt = TYP(r);
					auto rID = ID(r);

					// Don't push other muscle or solids that are anchored
					if (rt == PT_MUSC || rt == PT_DMND || rt == PT_CLNE)
						continue;

					// Push particles away
					float dist = sqrtf(rx*rx + ry*ry);
					if (dist > 0)
					{
						float force = pushForce * 0.3f / dist;
						parts[rID].vx += rx * force;
						parts[rID].vy += ry * force;
					}
				}
			}
		}

		// Propagate signal to adjacent muscle
		if (parts[i].tmp2 == 15)  // Early in contraction
		{
			for (auto rx = -1; rx <= 1; rx++)
			{
				for (auto ry = -1; ry <= 1; ry++)
				{
					if (rx || ry)
					{
						auto r = pmap[y+ry][x+rx];
						if (r && TYP(r) == PT_MUSC)
						{
							int rID = ID(r);
							if (parts[rID].tmp == 0)
							{
								parts[rID].tmp = 1;
								parts[rID].tmp2 = 18;  // Slight delay
							}
						}
					}
				}
			}
		}

		// End contraction
		if (parts[i].tmp2 <= 0)
		{
			parts[i].tmp = 0;
			parts[i].tmp2 = 0;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int contracting = cpart->tmp;
	int phase = cpart->tmp2;

	// Pink/red muscle color
	*colr = 204;
	*colg = 102;
	*colb = 102;

	// Darker and bulging when contracted
	if (contracting)
	{
		int intensity = (phase > 10) ? (20 - phase) : phase;
		*colr = 180 + intensity * 3;
		*colg = 80;
		*colb = 80;

		// Pulse effect
		*firea = intensity * 5;
		*firer = 200;
		*fireg = 100;
		*fireb = 100;
		*pixel_mode |= FIRE_ADD;
	}

	return 0;
}
