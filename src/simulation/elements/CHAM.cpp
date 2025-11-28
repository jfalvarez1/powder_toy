#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_CHAM()
{
	Identifier = "DEFAULT_PT_CHAM";
	Name = "CHAM";
	Colour = 0x808080_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.3f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 30;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.ctype = 0;  // Currently mimicking
	DefaultProperties.tmp = 0;   // Visual color R
	DefaultProperties.tmp2 = 0;  // Visual color G (B stored in life)
	HeatConduct = 50;
	Description = "Chameleon. Mimics the appearance and some properties of adjacent elements!";

	Properties = TYPE_PART;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 500.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	int mostCommon = 0;
	int counts[10] = {0};
	int types[10] = {0};
	int numTypes = 0;

	// Find what we're touching
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

				if (rt == PT_CHAM || rt == PT_CLNE || rt == PT_VOID)
					continue;

				// Count this type
				bool found = false;
				for (int j = 0; j < numTypes; j++)
				{
					if (types[j] == rt)
					{
						counts[j]++;
						found = true;
						break;
					}
				}
				if (!found && numTypes < 10)
				{
					types[numTypes] = rt;
					counts[numTypes] = 1;
					numTypes++;
				}
			}
		}
	}

	// Find most common neighbor
	int maxCount = 0;
	int mimicType = 0;
	for (int j = 0; j < numTypes; j++)
	{
		if (counts[j] > maxCount)
		{
			maxCount = counts[j];
			mimicType = types[j];
		}
	}

	// Mimic that element!
	if (mimicType > 0)
	{
		parts[i].ctype = mimicType;

		// Copy color from element
		auto col = elements[mimicType].Colour;
		parts[i].tmp = col.Red;
		parts[i].tmp2 = col.Green;
		parts[i].life = col.Blue;  // Store blue in life

		// Copy behavior slightly
		float mimic_gravity = elements[mimicType].Gravity;
		parts[i].vy += (mimic_gravity - 0.1f) * 0.5f;

		// Liquids make chameleon flow
		if (elements[mimicType].Properties & TYPE_LIQUID)
		{
			parts[i].vx *= 1.1f;
		}

		// Copy temperature resistance
		parts[i].temp = parts[i].temp * 0.95f + elements[mimicType].DefaultProperties.temp * 0.05f;
	}
	else
	{
		// Default gray when nothing to mimic
		parts[i].ctype = 0;
		parts[i].tmp = 128;
		parts[i].tmp2 = 128;
		parts[i].life = 128;  // Store blue in life
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int mimicking = cpart->ctype;

	if (mimicking > 0)
	{
		// Use stored colors
		*colr = cpart->tmp;
		*colg = cpart->tmp2;
		*colb = cpart->life;  // Blue stored in life

		// Slight shimmer effect to show it's a chameleon
		*firea = 15;
		*firer = *colr;
		*fireg = *colg;
		*fireb = *colb;
		*pixel_mode |= FIRE_ADD;
	}
	else
	{
		// Gray when not mimicking
		*colr = 128;
		*colg = 128;
		*colb = 128;
	}

	return 0;
}
