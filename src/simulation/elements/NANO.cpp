#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_NANO()
{
	Identifier = "DEFAULT_PT_NANO";
	Name = "NANO";
	Colour = 0x808080_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.4f;
	AirDrag = 0.02f * CFDS;
	AirLoss = 0.97f;
	Loss = 0.80f;
	Collision = 0.0f;
	Gravity = 0.05f;
	Diffusion = 0.10f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 30;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 500;   // Power/energy level
	DefaultProperties.tmp = 0;      // State: 0=dormant, 1=active, 2=swarming
	DefaultProperties.tmp2 = 0;     // Metal consumed counter
	HeatConduct = 200;
	Description = "Nanobots. Self-replicating machines that consume metal. Controlled by electricity. EMP disables them.";

	Properties = TYPE_PART | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1500.0f;  // Melts at high temp
	HighTemperatureTransition = PT_BRMT;  // Becomes broken metal

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	// Nanobots need energy to function
	if (parts[i].life <= 0)
	{
		parts[i].tmp = 0;  // Dormant when no power
		// Very slow power recovery
		if (sim->rng.chance(1, 500))
			parts[i].life = 1;
		return 0;
	}

	// Active nanobots consume power
	if (parts[i].tmp > 0)
	{
		if (sim->rng.chance(1, 20))
			parts[i].life--;
	}

	// Movement based on state
	if (parts[i].tmp == 2)  // Swarming
	{
		// Swarm toward other nanobots
		for (auto rx = -3; rx <= 3; rx++)
		{
			for (auto ry = -3; ry <= 3; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (!r) continue;
					if (TYP(r) == PT_NANO)
					{
						parts[i].vx += rx * 0.02f;
						parts[i].vy += ry * 0.02f;
					}
				}
			}
		}
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
					// Replicate if we have enough metal consumed
					if (parts[i].tmp2 >= 10 && parts[i].tmp > 0 && sim->rng.chance(1, 50))
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_NANO);
						if (np >= 0)
						{
							parts[np].life = parts[i].life / 2;
							parts[np].tmp = parts[i].tmp;
							parts[np].tmp2 = 0;
							parts[np].temp = parts[i].temp;
							parts[i].tmp2 -= 10;
							parts[i].life -= 50;
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				switch (rt)
				{
				// METALS - consume for replication
				case PT_METL:
				case PT_IRON:
				case PT_BMTL:
				case PT_BRMT:
				case PT_TTAN:
				case PT_TUNG:
				case PT_GOLD:
					if (parts[i].tmp > 0 && sim->rng.chance(1, 30))
					{
						sim->kill_part(rID);
						parts[i].tmp2 += 5;  // Consumed metal
						parts[i].life += 20;  // Energy from metal
						if (parts[i].life > 1000) parts[i].life = 1000;
					}
					// Attracted to metal
					parts[i].vx += rx * 0.1f;
					parts[i].vy += ry * 0.1f;
					break;

				case PT_INWR:
				case PT_PSCN:
				case PT_NSCN:
					// Circuit components - slower consumption
					if (parts[i].tmp > 0 && sim->rng.chance(1, 100))
					{
						sim->kill_part(rID);
						parts[i].tmp2 += 3;
						parts[i].life += 10;
						if (parts[i].life > 1000) parts[i].life = 1000;
					}
					break;

				case PT_SPRK:
					// Electricity activates nanobots!
					parts[i].tmp = 2;  // Swarming mode
					parts[i].life += 100;
					if (parts[i].life > 1000) parts[i].life = 1000;
					break;

				case PT_BTRY:
					// Batteries power nanobots
					if (sim->rng.chance(1, 50))
					{
						parts[i].life += 200;
						if (parts[i].life > 1000) parts[i].life = 1000;
						parts[i].tmp = 1;  // Active
					}
					break;

				case PT_NANO:
					// Share energy between nanobots
					if (parts[i].life > parts[rID].life + 50)
					{
						int transfer = (parts[i].life - parts[rID].life) / 4;
						parts[i].life -= transfer;
						parts[rID].life += transfer;
					}
					// Share state (swarm behavior)
					if (parts[rID].tmp > parts[i].tmp)
						parts[i].tmp = parts[rID].tmp;
					break;

				case PT_WATR:
				case PT_DSTW:
				case PT_SLTW:
					// Water damages nanobots
					if (sim->rng.chance(1, 100))
					{
						parts[i].life -= 10;
						if (parts[i].life <= 0)
						{
							sim->kill_part(i);
							return 1;
						}
					}
					break;

				case PT_ACID:
					// Acid destroys nanobots
					if (sim->rng.chance(1, 20))
					{
						sim->kill_part(i);
						return 1;
					}
					break;

				case PT_FIRE:
				case PT_PLSM:
				case PT_LAVA:
					// Heat damages nanobots
					parts[i].life -= 5;
					break;

				default:
					break;
				}
			}
		}
	}

	// EMP effect (check for EMP particles nearby)
	// High temperature also disables
	if (parts[i].temp > 1000.0f)
	{
		parts[i].tmp = 0;
		parts[i].life -= 10;
	}

	// Limit velocity
	float speed = sqrtf(parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy);
	if (speed > 3.0f)
	{
		parts[i].vx *= 3.0f / speed;
		parts[i].vy *= 3.0f / speed;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int state = cpart->tmp;
	int power = cpart->life;

	if (state == 0 || power <= 0)
	{
		// Dormant - gray
		*colr = 100;
		*colg = 100;
		*colb = 100;
	}
	else if (state == 1)
	{
		// Active - silver with subtle glow
		*colr = 150;
		*colg = 150;
		*colb = 170;
		*firea = 20;
		*firer = 100;
		*fireg = 100;
		*fireb = 150;
		*pixel_mode |= FIRE_ADD;
	}
	else
	{
		// Swarming - blue glow
		*colr = 100;
		*colg = 150;
		*colb = 255;
		*firea = 50;
		*firer = 50;
		*fireg = 100;
		*fireb = 255;
		*pixel_mode |= FIRE_ADD | PMODE_GLOW;
	}

	// Flash when consuming metal
	if (cpart->tmp2 > 5 && cpart->life % 3 == 0)
	{
		*colr = 255;
		*colg = 200;
		*colb = 100;
	}

	return 0;
}
