#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_PRSM()
{
	Identifier = "DEFAULT_PT_PRSM";
	Name = "PRSM";
	Colour = 0xDDFFFF_rgb;
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

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 50;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;  // Rainbow animation
	HeatConduct = 100;
	Description = "Prism. Refracts light into rainbow colors! Creates spectacular light shows.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1500.0f;
	HighTemperatureTransition = PT_LAVA;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	parts[i].tmp = (parts[i].tmp + 1) % 180;

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

				// Split light!
				if (rt == PT_PHOT)
				{
					// Get incoming photon direction
					float pvx = parts[rID].vx;
					float pvy = parts[rID].vy;

					// Create rainbow of photons
					for (int c = 0; c < 6; c++)
					{
						// Calculate refracted angle
						float angle = (c - 2.5f) * 0.15f;
						float cos_a = cosf(angle);
						float sin_a = sinf(angle);

						// Find exit point
						int exitX = x - rx;
						int exitY = y - ry;

						if (exitX >= 0 && exitX < XRES && exitY >= 0 && exitY < YRES && !pmap[exitY][exitX])
						{
							int np = sim->create_part(-1, exitX, exitY, PT_PHOT);
							if (np >= 0)
							{
								// Rotate velocity
								float newvx = pvx * cos_a - pvy * sin_a;
								float newvy = pvx * sin_a + pvy * cos_a;
								parts[np].vx = newvx;
								parts[np].vy = newvy;

								// Color based on spectrum position
								switch (c)
								{
								case 0: parts[np].ctype = 0xFF0000; break;  // Red
								case 1: parts[np].ctype = 0xFF8800; break;  // Orange
								case 2: parts[np].ctype = 0xFFFF00; break;  // Yellow
								case 3: parts[np].ctype = 0x00FF00; break;  // Green
								case 4: parts[np].ctype = 0x0088FF; break;  // Blue
								case 5: parts[np].ctype = 0x8800FF; break;  // Violet
								}
							}
						}
					}

					// Original photon continues through
					parts[rID].vx *= 0.5f;
					parts[rID].vy *= 0.5f;
				}

				// Spark creates light show
				if (rt == PT_SPRK)
				{
					// Emit random colored photons
					for (int c = 0; c < 3; c++)
					{
						if (sim->rng.chance(1, 10))
						{
							for (int dx = -1; dx <= 1; dx++)
							{
								for (int dy = -1; dy <= 1; dy++)
								{
									if (!pmap[y+dy][x+dx])
									{
										int np = sim->create_part(-1, x+dx, y+dy, PT_PHOT);
										if (np >= 0)
										{
											parts[np].vx = dx * 3;
											parts[np].vy = dy * 3;
											int hue = sim->rng.between(0, 5);
											int colors[] = {0xFF0000, 0xFF8800, 0xFFFF00, 0x00FF00, 0x0088FF, 0x8800FF};
											parts[np].ctype = colors[hue];
										}
										goto done_emit;
									}
								}
							}
							done_emit:;
						}
					}
				}

				// Fire creates warm glow
				if (rt == PT_FIRE || rt == PT_PLSM)
				{
					if (sim->rng.chance(1, 20))
					{
						for (int dx = -1; dx <= 1; dx++)
						{
							for (int dy = -1; dy <= 1; dy++)
							{
								if (!pmap[y+dy][x+dx])
								{
									int np = sim->create_part(-1, x+dx, y+dy, PT_EMBR);
									if (np >= 0)
									{
										parts[np].life = 20;
										parts[np].ctype = 0xFF8800;
										parts[np].vx = dx * 2;
										parts[np].vy = dy * 2;
									}
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int anim = cpart->tmp;

	// Clear crystal with rainbow shimmer
	*colr = 220;
	*colg = 255;
	*colb = 255;

	// Rainbow cycling on surface
	float hue = anim * 2.0f;
	int hi = (int)(hue / 60) % 6;
	float f = hue / 60 - (int)(hue / 60);

	int shimmerR, shimmerG, shimmerB;
	switch (hi)
	{
	case 0: shimmerR = 255; shimmerG = (int)(f * 255); shimmerB = 0; break;
	case 1: shimmerR = (int)((1-f) * 255); shimmerG = 255; shimmerB = 0; break;
	case 2: shimmerR = 0; shimmerG = 255; shimmerB = (int)(f * 255); break;
	case 3: shimmerR = 0; shimmerG = (int)((1-f) * 255); shimmerB = 255; break;
	case 4: shimmerR = (int)(f * 255); shimmerG = 0; shimmerB = 255; break;
	default: shimmerR = 255; shimmerG = 0; shimmerB = (int)((1-f) * 255); break;
	}

	// Subtle rainbow glow
	*firea = 40;
	*firer = shimmerR;
	*fireg = shimmerG;
	*fireb = shimmerB;
	*pixel_mode |= FIRE_ADD;

	return 0;
}
