#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_SONC()
{
	Identifier = "DEFAULT_PT_SONC";
	Name = "SONC";
	Colour = 0x66FFFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_FORCE;
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

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Frequency/animation
	DefaultProperties.tmp2 = 50; // Amplitude
	HeatConduct = 50;
	Description = "Sonic. Emits sound/pressure waves! Can shatter glass and push particles.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_KILL_DEC;

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
	bool powered = false;

	// Check for power
	for (auto rx = -1; rx <= 1; rx++)
	{
		for (auto ry = -1; ry <= 1; ry++)
		{
			if (rx || ry)
			{
				auto r = pmap[y+ry][x+rx];
				if (r && TYP(r) == PT_SPRK)
				{
					powered = true;
				}
			}
		}
	}

	if (powered)
	{
		parts[i].tmp = (parts[i].tmp + 10) % 360;  // Fast oscillation
		int amplitude = parts[i].tmp2;

		// Generate pressure wave
		float phase = parts[i].tmp * 3.14159f / 180.0f;
		float waveStrength = sinf(phase) * amplitude / 50.0f;

		sim->pv[y/CELL][x/CELL] += waveStrength;

		// Push particles in wave direction
		int range = 8;
		for (int rx = -range; rx <= range; rx++)
		{
			for (int ry = -range; ry <= range; ry++)
			{
				if (rx || ry)
				{
					if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
						continue;

					float dist = sqrtf(rx*rx + ry*ry);
					if (dist > range)
						continue;

					auto r = pmap[y+ry][x+rx];
					if (!r)
						continue;
					auto rt = TYP(r);
					auto rID = ID(r);

					if (rt == PT_SONC || rt == PT_DMND)
						continue;

					// Oscillating push
					float pushForce = waveStrength * (1 - dist / range) * 0.3f;

					// Push outward in wave
					if (dist > 0)
					{
						parts[rID].vx += (rx / dist) * pushForce;
						parts[rID].vy += (ry / dist) * pushForce;
					}

					// SHATTER GLASS with high amplitude
					if (rt == PT_GLAS && amplitude > 70)
					{
						if (sim->rng.chance(amplitude - 70, 200))
						{
							// Shatter!
							sim->part_change_type(rID, x+rx, y+ry, PT_DUST);
							parts[rID].vx = sim->rng.between(-5, 5);
							parts[rID].vy = sim->rng.between(-5, 5);
						}
					}

					// Disrupt liquids
					if (rt == PT_WATR || rt == PT_DSTW || rt == PT_SLTW)
					{
						parts[rID].vx += sim->rng.between(-20, 20) * pushForce * 0.1f;
						parts[rID].vy += sim->rng.between(-20, 20) * pushForce * 0.1f;
					}
				}
			}
		}
	}
	else
	{
		// Idle - slow animation
		parts[i].tmp = (parts[i].tmp + 1) % 360;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int phase = cpart->tmp;
	int amplitude = cpart->tmp2;

	// Cyan color
	*colr = 100;
	*colg = 255;
	*colb = 255;

	// Pulsing with sound waves
	float pulse = sinf(phase * 3.14159f / 180.0f);
	int glow = (int)(50 + pulse * 30 * amplitude / 50);

	*firea = glow;
	*firer = 100;
	*fireg = 255;
	*fireb = 255;
	*pixel_mode |= FIRE_ADD;

	// Strong glow when outputting
	if (amplitude > 50)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
