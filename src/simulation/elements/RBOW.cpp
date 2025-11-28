#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_RBOW()
{
	Identifier = "DEFAULT_PT_RBOW";
	Name = "RBOW";
	Colour = 0xFF0000_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
	Enabled = 1;

	Advection = 0.5f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = -0.02f;  // Floats up
	Diffusion = 0.10f;
	HotAir = 0.000f * CFDS;
	Falldown = 1;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Color phase
	DefaultProperties.tmp2 = 0;  // Trail counter
	HeatConduct = 0;
	Description = "Rainbow. Colorful particles that leave beautiful trails everywhere!";

	Properties = TYPE_PART;

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
	// Cycle through colors
	parts[i].tmp = (parts[i].tmp + 1) % 360;
	parts[i].tmp2++;

	// Graceful floating
	if (sim->rng.chance(1, 3))
	{
		parts[i].vx += sim->rng.between(-10, 10) * 0.03f;
		parts[i].vy += sim->rng.between(-10, 5) * 0.03f;
	}

	// Leave rainbow trail!
	if (parts[i].tmp2 % 5 == 0)
	{
		// Find trail position behind movement
		int trailX = x - (int)(parts[i].vx * 2);
		int trailY = y - (int)(parts[i].vy * 2);

		if (trailX >= 0 && trailX < XRES && trailY >= 0 && trailY < YRES)
		{
			if (!pmap[trailY][trailX])
			{
				int np = sim->create_part(-1, trailX, trailY, PT_RBOW);
				if (np >= 0)
				{
					parts[np].tmp = (parts[i].tmp + 60) % 360;  // Next color
					parts[np].tmp2 = -50;  // Negative = fading trail
					parts[np].vx = 0;
					parts[np].vy = 0;
				}
			}
		}
	}

	// Fading trails disappear
	if (parts[i].tmp2 < 0)
	{
		if (parts[i].tmp2 > -100)
		{
			parts[i].tmp2--;
		}
		else
		{
			sim->kill_part(i);
			return 1;
		}
	}

	// Spread joy!
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

				// Rainbow heals living things
				if (rt == PT_STKM || rt == PT_STKM2 || rt == PT_FIGH)
				{
					if (sim->rng.chance(1, 100))
					{
						parts[rID].life = std::min(parts[rID].life + 1, 100);
					}
				}

				// Rainbow puts out fire
				if (rt == PT_FIRE)
				{
					if (sim->rng.chance(1, 30))
					{
						sim->kill_part(rID);
					}
				}

				// Cools lava
				if (rt == PT_LAVA)
				{
					parts[rID].temp -= 10.0f;
				}
			}
		}
	}

	// Active rainbow fades eventually
	if (parts[i].tmp2 > 0 && sim->rng.chance(1, 1000))
	{
		sim->kill_part(i);
		return 1;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int hue = cpart->tmp;
	int age = cpart->tmp2;

	// HSV to RGB conversion
	float h = hue / 60.0f;
	int hi = (int)h % 6;
	float f = h - (int)h;
	float q = 1 - f;

	float r, g, b;
	switch (hi)
	{
	case 0: r = 1; g = f; b = 0; break;
	case 1: r = q; g = 1; b = 0; break;
	case 2: r = 0; g = 1; b = f; break;
	case 3: r = 0; g = q; b = 1; break;
	case 4: r = f; g = 0; b = 1; break;
	default: r = 1; g = 0; b = q; break;
	}

	*colr = (int)(r * 255);
	*colg = (int)(g * 255);
	*colb = (int)(b * 255);

	// Fading trails are transparent
	int alpha = 255;
	if (age < 0)
	{
		alpha = 255 + age * 2;
		if (alpha < 0) alpha = 0;
	}

	// Rainbow glow
	int glowIntensity = (age >= 0) ? 100 : 50 + age;
	if (glowIntensity < 0) glowIntensity = 0;

	*firea = glowIntensity;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	if (age < 0)
	{
		*pixel_mode |= PMODE_BLEND;
		*cola = alpha;
	}

	return 0;
}
