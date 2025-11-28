#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SWRM()
{
	Identifier = "DEFAULT_PT_SWRM";
	Name = "SWRM";
	Colour = 0x333300_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIFE;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 10;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 0;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.life = 500;  // Lifespan
	DefaultProperties.tmp = 0;     // Target X direction
	DefaultProperties.tmp2 = 0;    // Target Y direction
	HeatConduct = 20;
	Description = "Swarm. Insect swarm that seeks food and attacks! Controlled chaos.";

	Properties = TYPE_PART | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 273.0f;
	LowTemperatureTransition = PT_DUST;
	HighTemperature = 373.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Die when life runs out
	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	int targetX = 0;
	int targetY = 0;
	bool foundFood = false;
	bool foundEnemy = false;

	// Search for food/targets in larger radius
	int searchRadius = 10;
	for (int rx = -searchRadius; rx <= searchRadius; rx++)
	{
		for (int ry = -searchRadius; ry <= searchRadius; ry++)
		{
			if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
				continue;

			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			auto rt = TYP(r);

			// Look for food
			if (rt == PT_PLNT || rt == PT_VINE || rt == PT_WOOD || rt == PT_YEST)
			{
				targetX = rx;
				targetY = ry;
				foundFood = true;
				break;
			}

			// Look for enemies (stickmen)
			if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH)
			{
				targetX = rx;
				targetY = ry;
				foundEnemy = true;
				break;
			}
		}
		if (foundFood || foundEnemy)
			break;
	}

	// Store target for swarming behavior
	if (foundFood || foundEnemy)
	{
		parts[i].tmp = (targetX > 0) ? 1 : ((targetX < 0) ? -1 : 0);
		parts[i].tmp2 = (targetY > 0) ? 1 : ((targetY < 0) ? -1 : 0);
	}

	// Move toward target or swarm randomly
	float moveX = parts[i].tmp * 0.5f;
	float moveY = parts[i].tmp2 * 0.5f;

	// Add some chaos
	moveX += sim->rng.between(-10, 10) * 0.1f;
	moveY += sim->rng.between(-10, 10) * 0.1f;

	// Try to move
	int newX = x + (int)moveX;
	int newY = y + (int)moveY;

	if (newX >= 0 && newX < XRES && newY >= 0 && newY < YRES)
	{
		if (!pmap[newY][newX])
		{
			// Move there
			parts[i].x = newX;
			parts[i].y = newY;
		}
	}

	// Interact with neighbors
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (!r)
				{
					// Reproduce if well fed
					if (parts[i].life > 400 && sim->rng.chance(1, 100))
					{
						int np = sim->create_part(-1, x+rx, y+ry, PT_SWRM);
						if (np >= 0)
						{
							parts[np].life = 300;
							parts[i].life -= 100;
						}
					}
					continue;
				}
				auto rt = TYP(r);
				auto rID = ID(r);

				switch (rt)
				{
				case PT_PLNT:
				case PT_VINE:
				case PT_WOOD:
				case PT_SAWD:
					// Eat plants!
					if (sim->rng.chance(1, 20))
					{
						sim->kill_part(rID);
						parts[i].life = std::min(parts[i].life + 50, 1000);
					}
					break;
				case PT_YEST:
					// Love yeast!
					if (sim->rng.chance(1, 10))
					{
						sim->kill_part(rID);
						parts[i].life = std::min(parts[i].life + 100, 1000);
					}
					break;
				case PT_STKM:
				case PT_STKM2:
				case PT_FIGH:
					// Sting!
					if (sim->rng.chance(1, 30))
					{
						parts[rID].life -= 1;
					}
					break;
				case PT_SWRM:
					// Swarm together - share life
					if (parts[rID].life < parts[i].life - 50)
					{
						parts[i].life -= 20;
						parts[rID].life += 20;
					}
					// Move in similar direction
					parts[i].tmp = (parts[i].tmp + parts[rID].tmp) / 2;
					parts[i].tmp2 = (parts[i].tmp2 + parts[rID].tmp2) / 2;
					break;
				case PT_WATR:
				case PT_DSTW:
					// Water slows swarm
					parts[i].life -= 1;
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Fire kills swarm
					sim->kill_part(i);
					return 1;
				case PT_SOAP:
					// Soap kills insects
					if (sim->rng.chance(1, 5))
					{
						sim->kill_part(i);
						return 1;
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
	int life = cpart->life;

	// Dark brownish-black insects
	*colr = 51 + (life / 20);
	*colg = 51 + (life / 30);
	*colb = 0;

	if (*colr > 80) *colr = 80;
	if (*colg > 70) *colg = 70;

	// Slight buzz effect
	if (life % 3 == 0)
	{
		*colr += 20;
		*colg += 20;
	}

	return 0;
}
