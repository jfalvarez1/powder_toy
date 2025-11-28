#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_WEBB()
{
	Identifier = "DEFAULT_PT_WEBB";
	Name = "WEBB";
	Colour = 0xEEEEEE_rgb;
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

	Flammable = 50;
	Explosive = 0;
	Meltable = 0;
	Hardness = 1;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Caught particle count
	HeatConduct = 30;
	Description = "Spider Web. Sticky trap that catches and holds particles. Burns easily.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 5.0f;  // Breaks under pressure
	HighPressureTransition = PT_NONE;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 373.0f;  // Burns at 100C
	HighTemperatureTransition = PT_FIRE;

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

				// Don't catch certain things
				if (rt == PT_WEBB || rt == PT_CLNE || rt == PT_VOID || rt == PT_BHOL ||
				    rt == PT_FIRE || rt == PT_PLSM || rt == PT_THDR || rt == PT_PHOT)
					continue;

				// Catch moving particles!
				float speed = fabsf(parts[rID].vx) + fabsf(parts[rID].vy);
				if (speed > 0.1f)
				{
					// Stop the particle
					parts[rID].vx *= 0.1f;
					parts[rID].vy *= 0.1f;

					// Count caught particles
					parts[i].tmp++;

					// Web gets stronger with more catches (up to a point)
					// but breaks if too many
					if (parts[i].tmp > 20 && sim->rng.chance(1, 50))
					{
						sim->kill_part(i);
						return 1;
					}
				}
				// Already caught particles stay stuck
				else if (speed < 0.5f && elements[rt].Falldown > 0)
				{
					parts[rID].vx *= 0.5f;
					parts[rID].vy *= 0.5f;
				}

				// Fire burns web
				if (rt == PT_FIRE || rt == PT_PLSM || rt == PT_LAVA)
				{
					sim->part_change_type(i, x, y, PT_FIRE);
					parts[i].life = sim->rng.between(5, 15);
					return 0;
				}

				// Water weakens web
				if (rt == PT_WATR || rt == PT_DSTW)
				{
					if (sim->rng.chance(1, 500))
					{
						sim->kill_part(i);
						return 1;
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// White/gray semi-transparent
	int catches = cpart->tmp;

	*colr = 238;
	*colg = 238;
	*colb = 238;

	// Gets darker/dirtier with more catches
	if (catches > 5)
	{
		*colr -= catches * 5;
		*colg -= catches * 5;
		*colb -= catches * 3;
	}

	// Semi-transparent
	*pixel_mode |= PMODE_BLEND;
	*cola = 150;

	return 0;
}
