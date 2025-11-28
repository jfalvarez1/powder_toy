#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_GOST()
{
	Identifier = "DEFAULT_PT_GOST";
	Name = "GOST";
	Colour = 0xDDDDFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 1.00f;
	Loss = 1.00f;
	Collision = 0.0f;
	Gravity = -0.01f;
	Diffusion = 0.05f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP - 30.0f + 273.15f;  // Cold
	DefaultProperties.life = 500;  // Existence time
	DefaultProperties.tmp = 0;     // Waver animation
	HeatConduct = 0;
	Description = "Ghost. Spooky! Passes through walls, haunts stickmen, fears light.";

	Properties = TYPE_PART | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	auto &sd = SimulationData::CRef();
	auto &elements = sd.elements;

	if (parts[i].life <= 0)
	{
		sim->kill_part(i);
		return 1;
	}

	// Waver animation
	parts[i].tmp = (parts[i].tmp + 1) % 40;

	// Spooky drifting
	if (sim->rng.chance(1, 5))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.03f;
		parts[i].vy += sim->rng.between(-10, 5) * 0.03f;
	}

	// Chill the air
	sim->pv[y/CELL][x/CELL] -= 0.005f;

	// Search for things to haunt or flee from
	for (int rx = -5; rx <= 5; rx++)
	{
		for (int ry = -5; ry <= 5; ry++)
		{
			if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
				continue;

			auto r = pmap[y+ry][x+rx];
			if (!r)
				continue;
			auto rt = TYP(r);
			auto rID = ID(r);

			// Fear light!
			if (rt == PT_GLOW || rt == PT_PHOT || rt == PT_LIGH)
			{
				// Flee!
				parts[i].vx -= rx * 0.2f;
				parts[i].vy -= ry * 0.2f;
				parts[i].life -= 2;  // Light damages ghosts
			}

			// Fire hurts
			if (rt == PT_FIRE || rt == PT_PLSM)
			{
				parts[i].vx -= rx * 0.1f;
				parts[i].vy -= ry * 0.1f;
				parts[i].life -= 1;
			}

			// Haunt stickmen!
			if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH)
			{
				// Chase them
				parts[i].vx += rx * 0.05f;
				parts[i].vy += ry * 0.05f;

				// Scare them (nearby)
				float dist = sqrtf(rx*rx + ry*ry);
				if (dist < 3)
				{
					// BOO! Push them away
					parts[rID].vx -= rx * 0.3f;
					parts[rID].vy -= ry * 0.3f;
					// Chill them
					parts[rID].temp -= 5.0f;
				}
			}
		}
	}

	// Pass through solids!
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

				// Phase through solid walls
				if (elements[rt].Properties & TYPE_SOLID)
				{
					// Find empty space on other side
					int checkX = x + rx * 2;
					int checkY = y + ry * 2;
					if (checkX >= 0 && checkX < XRES && checkY >= 0 && checkY < YRES)
					{
						if (!pmap[checkY][checkX] && sim->rng.chance(1, 5))
						{
							parts[i].x = checkX;
							parts[i].y = checkY;
							goto done_phase;
						}
					}
				}
			}
		}
	}
	done_phase:

	// Ghosts multiply in darkness
	if (parts[i].life > 400 && sim->rng.chance(1, 1000))
	{
		bool darkHere = true;
		// Check for light
		for (int rx = -3; rx <= 3; rx++)
		{
			for (int ry = -3; ry <= 3; ry++)
			{
				auto r = pmap[y+ry][x+rx];
				if (r && (TYP(r) == PT_GLOW || TYP(r) == PT_PHOT || TYP(r) == PT_FIRE))
				{
					darkHere = false;
					break;
				}
			}
		}

		if (darkHere)
		{
			int np = sim->create_part(-1, x + sim->rng.between(-2, 2), y + sim->rng.between(-2, 2), PT_GOST);
			if (np >= 0)
			{
				parts[np].life = 200;
				parts[i].life -= 100;
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int waver = cpart->tmp;
	int life = cpart->life;

	// Pale ghostly blue-white
	*colr = 220;
	*colg = 220;
	*colb = 255;

	// Wavering transparency
	int wave = (waver < 20) ? waver : 40 - waver;
	int alpha = 50 + wave * 3 + (life / 10);
	if (alpha > 200) alpha = 200;

	*pixel_mode |= PMODE_BLEND;
	*cola = alpha;

	// Spooky glow
	*firea = 30 + wave;
	*firer = 200;
	*fireg = 200;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
