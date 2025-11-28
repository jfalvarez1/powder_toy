#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_PXLD()
{
	Identifier = "DEFAULT_PT_PXLD";
	Name = "PXLD";
	Colour = 0xFF88FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.8f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.90f;
	Collision = 0.0f;
	Gravity = -0.05f;  // Floats up!
	Diffusion = 0.30f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;  // Lightest particle

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Color cycle
	DefaultProperties.tmp2 = 0;  // Magic charge
	HeatConduct = 10;
	Description = "Pixie Dust. Magical dust that makes things float! Sparkles prettily.";

	Properties = TYPE_PART;

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
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	sim->parts[i].tmp = sim->rng.between(0, 100);  // Random color phase
	sim->parts[i].tmp2 = 100;  // Full magic charge
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// Color cycle animation
	parts[i].tmp = (parts[i].tmp + 1) % 100;

	// Magic fades over time
	if (sim->rng.chance(1, 500))
	{
		parts[i].tmp2--;
		if (parts[i].tmp2 <= 0)
		{
			// Lost all magic
			sim->kill_part(i);
			return 1;
		}
	}

	// Float around magically
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.03f;
		parts[i].vy += sim->rng.between(-15, 5) * 0.03f;  // Bias upward
	}

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

				// Make things float!
				if (parts[i].tmp2 > 20)
				{
					float liftForce = parts[i].tmp2 * 0.001f;
					parts[rID].vy -= liftForce;

					// Lighter particles float more
					int weight = elements[rt].Weight;
					if (weight < 50)
					{
						parts[rID].vy -= liftForce * 2;
					}
				}

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
					// Water washes away magic
					parts[i].tmp2 -= 5;
					break;
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Stickmen can fly!
					if (parts[i].tmp2 > 30)
					{
						parts[rID].vy -= 0.3f;
						parts[i].tmp2 -= 2;
					}
					break;
				case PT_PXLD:
					// Pixie dust clumps sparkle more
					parts[i].tmp2 = std::min(parts[i].tmp2 + 1, 100);
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Fire consumes pixie dust
					if (sim->rng.chance(1, 20))
					{
						sim->kill_part(i);
						// Create sparkle burst
						for (int j = 0; j < 3; j++)
						{
							int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_EMBR);
							if (np >= 0)
							{
								parts[np].life = 20;
								parts[np].ctype = 0xFF88FF;
							}
						}
						return 1;
					}
					break;
				case PT_GOLD:
					// Gold recharges pixie dust!
					if (sim->rng.chance(1, 100))
					{
						parts[i].tmp2 = std::min(parts[i].tmp2 + 10, 100);
					}
					break;
				default:
					break;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int phase = cpart->tmp;
	int magic = cpart->tmp2;

	// Rainbow color cycle
	float hue = phase * 3.6f;  // 0-360 degrees
	float r, g, b;

	// Simple HSV to RGB (saturation=1, value=1)
	int hi = (int)(hue / 60) % 6;
	float f = hue / 60 - hi;
	float q = 1 - f;

	switch (hi)
	{
	case 0: r = 1; g = f; b = 0; break;
	case 1: r = q; g = 1; b = 0; break;
	case 2: r = 0; g = 1; b = f; break;
	case 3: r = 0; g = q; b = 1; break;
	case 4: r = f; g = 0; b = 1; break;
	default: r = 1; g = 0; b = q; break;
	}

	// Pastel colors (mix with white)
	*colr = (int)(200 + r * 55);
	*colg = (int)(200 + g * 55);
	*colb = (int)(200 + b * 55);

	// Sparkle glow based on magic
	*firea = 50 + magic;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
