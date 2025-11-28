#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_FRFL()
{
	Identifier = "DEFAULT_PT_FRFL";
	Name = "FRFL";
	Colour = 0x202020_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 20;

	Weight = 40;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Magnetization level
	HeatConduct = 70;
	Description = "Ferrofluid. Magnetic liquid attracted to iron, magnets, and electricity. Forms spiky patterns.";

	Properties = TYPE_LIQUID | PROP_CONDUCTS | PROP_LIFE_DEC;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = 200.0f;  // Freezes at low temp
	LowTemperatureTransition = PT_IRON;  // Becomes solid iron
	HighTemperature = 600.0f;  // Boils
	HighTemperatureTransition = PT_BRMT;  // Becomes broken metal fragments

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Magnetic field strength (accumulates from nearby sources)
	float magX = 0, magY = 0;
	bool nearMetal = false;
	bool nearElec = false;

	// Scan for magnetic sources
	for (auto rx = -4; rx <= 4; rx++)
	{
		for (auto ry = -4; ry <= 4; ry++)
		{
			if (rx || ry)
			{
				int nx = x + rx;
				int ny = y + ry;
				if (nx < 0 || nx >= XRES || ny < 0 || ny >= YRES)
					continue;

				auto r = pmap[ny][nx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				float dist = sqrtf(rx*rx + ry*ry);
				if (dist < 0.1f) dist = 0.1f;
				float strength = 0;

				switch (rt)
				{
				case PT_IRON:
					strength = 3.0f / (dist * dist);
					nearMetal = true;
					break;
				case PT_METL:
				case PT_BMTL:
					strength = 1.5f / (dist * dist);
					nearMetal = true;
					break;
				case PT_TTAN:
				case PT_TUNG:
					strength = 1.0f / (dist * dist);
					nearMetal = true;
					break;
				case PT_GOLD:
				case PT_INWR:
					strength = 0.5f / (dist * dist);
					break;
				case PT_SPRK:
					// Electricity creates magnetic field!
					strength = 5.0f / (dist * dist);
					nearElec = true;
					// Add perpendicular force (electromagnetic effect)
					magX += ry * 0.3f / dist;
					magY -= rx * 0.3f / dist;
					break;
				case PT_FRFL:
					// Ferrofluid slightly attracts itself when magnetized
					if (parts[rID].tmp > 50)
					{
						strength = 0.3f / (dist * dist);
					}
					break;
				case PT_PSCN:
				case PT_NSCN:
					strength = 0.8f / (dist * dist);
					break;
				default:
					break;
				}

				if (strength > 0)
				{
					// Add attraction toward magnetic source
					magX += (rx / dist) * strength;
					magY += (ry / dist) * strength;
				}
			}
		}
	}

	// Apply magnetic forces
	float magStrength = sqrtf(magX * magX + magY * magY);
	if (magStrength > 0.01f)
	{
		// Limit force
		if (magStrength > 2.0f)
		{
			magX *= 2.0f / magStrength;
			magY *= 2.0f / magStrength;
		}

		parts[i].vx += magX * 0.3f;
		parts[i].vy += magY * 0.3f;

		// Update magnetization level
		parts[i].tmp = std::min(parts[i].tmp + (int)(magStrength * 10), 255);
	}
	else
	{
		// Slowly demagnetize
		if (parts[i].tmp > 0 && sim->rng.chance(1, 10))
			parts[i].tmp--;
	}

	// Limit velocity
	float speed = sqrtf(parts[i].vx * parts[i].vx + parts[i].vy * parts[i].vy);
	if (speed > 5.0f)
	{
		parts[i].vx *= 5.0f / speed;
		parts[i].vy *= 5.0f / speed;
	}

	// Interactions with neighbors
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

				switch (rt)
				{
				case PT_WATR:
				case PT_DSTW:
					// Oil and water don't mix - but ferrofluid floats
					if (sim->rng.chance(1, 50))
					{
						// Swap positions (ferrofluid rises in water)
						parts[i].vy -= 0.2f;
					}
					break;
				case PT_OIL:
					// Mixes with oil
					if (sim->rng.chance(1, 1000))
					{
						// Small chance to absorb oil
						sim->kill_part(rID);
					}
					break;
				case PT_FIRE:
				case PT_PLSM:
					// Burns but doesn't ignite easily
					if (parts[i].temp > 500.0f && sim->rng.chance(1, 100))
					{
						sim->part_change_type(i, x, y, PT_FIRE);
						parts[i].life = sim->rng.between(10, 30);
						return 1;
					}
					break;
				default:
					break;
				}
			}
		}
	}

	// Highly magnetized ferrofluid can spark (like static discharge)
	if (parts[i].tmp > 200 && nearElec && sim->rng.chance(1, 100))
	{
		int np = sim->create_part(-1, x + sim->rng.between(-1, 1), y + sim->rng.between(-1, 1), PT_SPRK);
		// Ferrofluid discharges
		parts[i].tmp -= 50;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int mag = cpart->tmp;

	// Base color - dark iron color
	*colr = 32 + mag / 8;
	*colg = 32 + mag / 10;
	*colb = 40 + mag / 6;

	if (*colr > 100) *colr = 100;
	if (*colg > 90) *colg = 90;
	if (*colb > 120) *colb = 120;

	// Magnetized ferrofluid has metallic sheen
	if (mag > 50)
	{
		*firea = mag / 4;
		*firer = 60 + mag / 3;
		*fireg = 60 + mag / 3;
		*fireb = 80 + mag / 2;
		*pixel_mode |= FIRE_ADD;

		// Highly magnetized = slight glow
		if (mag > 150)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}

	// Spiky appearance from velocity (when being attracted)
	float speed = sqrtf(cpart->vx * cpart->vx + cpart->vy * cpart->vy);
	if (speed > 1.0f)
	{
		*colr += (int)(speed * 10);
		*colg += (int)(speed * 10);
		*colb += (int)(speed * 15);
	}

	return 0;
}
