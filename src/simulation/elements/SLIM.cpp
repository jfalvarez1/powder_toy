#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SLIM()
{
	Identifier = "DEFAULT_PT_SLIM";
	Name = "SLIM";
	Colour = 0x40FF40_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.3f;   // Sluggish movement
	AirDrag = 0.05f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.50f;       // Very viscous
	Collision = 0.0f;
	Gravity = 0.15f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 5;
	Explosive = 0;
	Meltable = 0;
	Hardness = 5;

	Weight = 50;

	DefaultProperties.temp = R_TEMP + 273.15f;
	HeatConduct = 20;
	Description = "Slime. Sticky viscous liquid that traps and slows particles. Slightly acidic.";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 250.0f;
	LowTemperatureTransition = PT_ICEI;  // Freezes into ice
	HighTemperature = 450.0f;
	HighTemperatureTransition = PT_GAS;  // Evaporates into gas

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// Slime is very viscous - slow down self
	parts[i].vx *= 0.8f;
	parts[i].vy *= 0.8f;

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

				// Trap effect: slow down particles that touch slime
				if (rt != PT_SLIM && rt != PT_CLNE && rt != PT_PCLN && rt != PT_VOID && rt != PT_BHOL)
				{
					// Drastically slow particles
					parts[rID].vx *= 0.5f;
					parts[rID].vy *= 0.5f;
				}

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
					// Water dilutes slime, eventually killing it
					if (sim->rng.chance(1, 500))
					{
						sim->kill_part(i);
						return 1;
					}
					break;
				case PT_SALT:
					// Salt dissolves slime
					if (sim->rng.chance(1, 100))
					{
						sim->kill_part(i);
						sim->kill_part(rID);
						return 1;
					}
					break;
				case PT_ACID:
					// Acid and slime combine to make more slime!
					if (sim->rng.chance(1, 50))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_SLIM);
					}
					break;
				case PT_DUST:
				case PT_SAND:
				case PT_STNE:
				case PT_BRCK:
					// Slime slowly dissolves powders/stones (acidic)
					if (sim->rng.chance(1, 1000))
					{
						sim->kill_part(rID);
					}
					break;
				case PT_YEST:
					// Yeast makes slime grow!
					if (sim->rng.chance(1, 100))
					{
						sim->part_change_type(rID, x+rx, y+ry, PT_SLIM);
					}
					break;
				case PT_PLNT:
					// Slime feeds on plants
					if (sim->rng.chance(1, 200))
					{
						sim->kill_part(rID);
						// Slime grows a bit
						int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_SLIM);
						if (np >= 0)
						{
							parts[np].temp = parts[i].temp;
						}
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Slime catches fire but also smothers it
					if (sim->rng.chance(1, 10))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = sim->rng.between(20, 40);
						return 1;
					}
					else if (sim->rng.chance(1, 5))
					{
						sim->kill_part(rID);
					}
					break;
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Slime hurts stickmen (very slowly)
					if (sim->rng.chance(1, 500))
					{
						parts[rID].life -= 1;
					}
					// Also slow them down
					parts[rID].vx *= 0.7f;
					parts[rID].vy *= 0.7f;
					break;
				default:
					break;
				}
			}
		}
	}

	// Slime slowly evaporates in hot conditions
	if (parts[i].temp > 400.0f && sim->rng.chance(1, 200))
	{
		sim->create_part(i, x, y, PT_WTRV);
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Semi-transparent green glow
	*colr = 64;
	*colg = 255;
	*colb = 64;

	// Slight transparency
	*pixel_mode |= PMODE_BLEND;
	*cola = 200;

	// Subtle glow
	*firea = 20;
	*firer = 50;
	*fireg = 150;
	*fireb = 50;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
