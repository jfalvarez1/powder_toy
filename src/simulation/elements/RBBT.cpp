#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_RBBT()
{
	Identifier = "DEFAULT_PT_RBBT";
	Name = "RBBT";
	Colour = 0x2F2F2F_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.90f;
	Collision = -0.9f;  // VERY bouncy!
	Gravity = 0.3f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 30;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Weight = 40;

	DefaultProperties.temp = R_TEMP + 273.15f;
	HeatConduct = 20;  // Poor conductor (rubber insulates)
	Description = "Rubber. Extremely bouncy! Insulates electricity and heat.";

	Properties = TYPE_PART | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 200.0f;  // Becomes brittle when frozen
	LowTemperatureTransition = PT_BRCK;
	HighTemperature = 500.0f;  // Melts/burns
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Rubber gets bouncier when compressed
	float pressure = sim->pv[y/CELL][x/CELL];
	if (pressure > 2.0f)
	{
		// Store potential energy
		parts[i].tmp = std::min((int)(pressure * 10), 100);
	}

	// Release stored energy as bounce
	if (parts[i].tmp > 0 && sim->pv[y/CELL][x/CELL] < 1.0f)
	{
		float boost = parts[i].tmp * 0.01f;
		parts[i].vy -= boost;  // Bounce up!
		parts[i].tmp = 0;
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

				// Block electricity (insulator)
				if (rt == PT_SPRK)
				{
					// Absorb spark, don't conduct
					continue;
				}

				// Transfer momentum on collision (bouncy!)
				if (rt == PT_RBBT)
				{
					// Two rubber balls = super bounce!
					float dvx = parts[i].vx - parts[rID].vx;
					float dvy = parts[i].vy - parts[rID].vy;

					parts[i].vx -= dvx * 0.5f;
					parts[i].vy -= dvy * 0.5f;
					parts[rID].vx += dvx * 0.5f;
					parts[rID].vy += dvy * 0.5f;
				}

				// Bounce other particles too
				else if (TYP(r) != PT_CLNE && TYP(r) != PT_VOID)
				{
					float speed = fabsf(parts[rID].vx) + fabsf(parts[rID].vy);
					if (speed > 0.5f)
					{
						// Bounce them away
						parts[rID].vx *= -0.8f;
						parts[rID].vy *= -0.8f;
					}
				}

				// Oil/gasoline damages rubber
				if (rt == PT_OIL || rt == PT_DESL)
				{
					if (sim->rng.chance(1, 1000))
					{
						sim->kill_part(i);
						return 1;
					}
				}
			}
		}
	}

	// Ensure minimum bounce
	if (fabsf(parts[i].vy) < 0.5f && fabsf(parts[i].vy) > 0.01f)
	{
		// Dampen very small bounces
		parts[i].vy *= 0.9f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int stored = cpart->tmp;

	// Darker base, lighter when compressed
	*colr = 47 + stored / 2;
	*colg = 47 + stored / 2;
	*colb = 47 + stored / 3;

	// Slight sheen
	if (stored > 20)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
