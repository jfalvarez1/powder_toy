#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_QSND()
{
	Identifier = "DEFAULT_PT_QSND";
	Name = "QSND";
	Colour = 0xC2B280_rgb;
	MenuVisible = 1;
	MenuSection = SC_POWDERS;
	Enabled = 1;

	Advection = 0.5f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.94f;
	Loss = 0.90f;
	Collision = 0.0f;
	Gravity = 0.2f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;  // Like liquid but heavier

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 90;  // Very heavy, pulls things down

	DefaultProperties.temp = R_TEMP + 273.15f;
	HeatConduct = 60;
	Description = "Quicksand. Sucks particles down and traps them. Struggles make it worse!";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1700.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

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

				// Don't affect other quicksand or walls
				if (rt == PT_QSND || rt == PT_CLNE || rt == PT_PCLN || rt == PT_VOID)
					continue;

				// Pull particles down into the quicksand
				if (elements[rt].Falldown > 0 || (elements[rt].Properties & TYPE_PART))
				{
					// Add downward velocity
					parts[rID].vy += 0.3f;

					// Slow horizontal movement (trapped!)
					parts[rID].vx *= 0.7f;

					// If particle is struggling (moving fast), make it worse
					float speed = fabsf(parts[rID].vx) + fabsf(parts[rID].vy);
					if (speed > 1.0f)
					{
						parts[rID].vy += 0.2f;
						parts[rID].vx *= 0.5f;
					}
				}

				// Stickmen sink slowly
				if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH)
				{
					parts[rID].vy += 0.1f;
					parts[rID].vx *= 0.8f;
				}

				// Water makes quicksand more dangerous
				if (rt == PT_WATR || rt == PT_DSTW)
				{
					if (sim->rng.chance(1, 100))
					{
						// Absorb water, become more viscous
						sim->kill_part(rID);
						parts[i].tmp = std::min(parts[i].tmp + 1, 100);
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Tan/brown color with variation
	int wet = cpart->tmp;

	*colr = 194 - wet;
	*colg = 178 - wet;
	*colb = 128 - wet/2;

	// Wetter quicksand is darker
	if (wet > 50)
	{
		*colr -= 30;
		*colg -= 30;
		*colb -= 20;
	}

	return 0;
}
