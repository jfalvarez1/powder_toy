#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_ECTM()
{
	Identifier = "DEFAULT_PT_ECTM";
	Name = "ECTM";
	Colour = 0x88FF88_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.5f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.03f;  // Floats upward
	Diffusion = 0.10f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 5;

	DefaultProperties.temp = R_TEMP - 20.0f + 273.15f;  // Cold
	DefaultProperties.life = 1000;  // Existence timer
	DefaultProperties.tmp = 0;      // Phase state
	HeatConduct = 5;  // Barely conducts heat
	Description = "Ectoplasm. Ghostly substance that phases through solids. Chills surroundings.";

	Properties = TYPE_LIQUID | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 400.0f;  // Disperses when heated
	HighTemperatureTransition = PT_WTRV;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// Fade away over time
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Phase through solids!
	parts[i].tmp = (parts[i].tmp + 1) % 100;
	bool phasing = (parts[i].tmp < 30);  // Phase 30% of the time

	// Cool surroundings (ghostly chill)
	sim->pv[y/CELL][x/CELL] -= 0.01f;

	// Random spooky movement
	if (sim->rng.chance(1, 10))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.05f;
		parts[i].vy += sim->rng.between(-10, 10) * 0.05f;
	}

	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
				{
					// Leave a cold trail
					if (sim->rng.chance(1, 100))
					{
						sim->pv[std::max(0,(y+ry)/CELL)][std::max(0,(x+rx)/CELL)] -= 0.1f;
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				// Chill everything nearby
				if (parts[rID].temp > parts[i].temp)
				{
					parts[rID].temp -= 1.0f;
					parts[i].temp += 0.5f;
				}

				switch (rt)
				{
				case PT_FIRE:
				case PT_PLSM:
					// Extinguish fire (ghosts hate fire)
					if (sim->rng.chance(1, 5))
					{
						sim->kill_part(rID);
						parts[i].life -= 50;  // Costs energy
					}
					break;
				case PT_LIGH:
					// Lightning disperses ectoplasm
					parts[i].life -= 100;
					break;
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Spook stickmen (make them cold and scared)
					parts[rID].temp -= 5.0f;
					// Push them away!
					parts[rID].vx += rx * 0.5f;
					parts[rID].vy += ry * 0.5f;
					break;
				case PT_ECTM:
					// Ectoplasm clumps together
					parts[i].vx += rx * 0.02f;
					parts[i].vy += ry * 0.02f;
					// Share life force
					if (parts[rID].life < parts[i].life - 50)
					{
						parts[i].life -= 20;
						parts[rID].life += 20;
					}
					break;
				default:
					// Phase through solids
					if (phasing && (elements[rt].Properties & TYPE_SOLID))
					{
						// Find an empty spot to phase to
						for (int dx = -2; dx <= 2; dx++)
						{
							for (int dy = -2; dy <= 2; dy++)
							{
								if (!pmap[y+dy][x+dx] && sim->rng.chance(1, 10))
								{
									parts[i].x = x + dx;
									parts[i].y = y + dy;
									goto done_phase;
								}
							}
						}
						done_phase:;
					}
					break;
				}
			}
		}
	}

	// Occasionally spawn more ectoplasm if there's enough
	if (parts[i].life > 800 && sim->rng.chance(1, 500))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_ECTM);
		if (np >= 0)
		{
			parts[np].life = 200;
			parts[i].life -= 200;
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int life = cpart->life;
	int phase = cpart->tmp;

	// Ghostly green, more transparent when phasing
	*colr = 136;
	*colg = 255;
	*colb = 136;

	// Fade based on life remaining
	int alpha = 50 + (life * 150 / 1000);
	if (alpha > 200) alpha = 200;

	// More transparent when phasing
	if (phase < 30)
	{
		alpha = alpha / 2;
	}

	*pixel_mode |= PMODE_BLEND;
	*cola = alpha;

	// Ghostly glow
	*firea = 50 + life / 20;
	*firer = 100;
	*fireg = 255;
	*fireb = 100;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
