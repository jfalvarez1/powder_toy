#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);
static void create(ELEMENT_CREATE_FUNC_ARGS);

void Element::Element_WRMH()
{
	Identifier = "DEFAULT_PT_WRMH";
	Name = "WRMH";
	Colour = 0xFF00FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SPECIAL;
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
	Hardness = 0;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Pair ID
	DefaultProperties.tmp2 = 0;  // Animation
	HeatConduct = 0;
	Description = "Wormhole. Teleports particles to another wormhole with the same tmp value!";

	Properties = TYPE_SOLID;

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
	Create = &create;
}

static void create(ELEMENT_CREATE_FUNC_ARGS)
{
	// Assign a random pair ID (can be changed by user)
	sim->parts[i].tmp = sim->rng.between(0, 9);
	sim->parts[i].tmp2 = 0;
}

static int update(UPDATE_FUNC_ARGS)
{
	int myPairID = parts[i].tmp;
	parts[i].tmp2 = (parts[i].tmp2 + 1) % 40;  // Animation

	// Find paired wormhole
	int pairedX = -1, pairedY = -1;

	for (int py = 0; py < YRES; py += CELL)
	{
		for (int px = 0; px < XRES; px += CELL)
		{
			auto r = pmap[py][px];
			if (r && TYP(r) == PT_WRMH)
			{
				int rID = ID(r);
				if (rID != i && parts[rID].tmp == myPairID)
				{
					pairedX = (int)parts[rID].x;
					pairedY = (int)parts[rID].y;
					break;
				}
			}
		}
		if (pairedX >= 0)
			break;
	}

	// If no pair found, do a more thorough search
	if (pairedX < 0)
	{
		for (int j = 0; j < NPART; j++)
		{
			if (parts[j].type == PT_WRMH && j != i && parts[j].tmp == myPairID)
			{
				pairedX = (int)parts[j].x;
				pairedY = (int)parts[j].y;
				break;
			}
		}
	}

	// Teleport nearby particles
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

				// Don't teleport wormholes or special elements
				if (rt == PT_WRMH || rt == PT_CLNE || rt == PT_VOID || rt == PT_DMND)
					continue;

				// Check if moving toward the wormhole
				float speed = fabsf(parts[rID].vx) + fabsf(parts[rID].vy);
				if (speed < 0.3f)
					continue;

				// Teleport!
				if (pairedX >= 0 && sim->rng.chance(1, 3))
				{
					// Find empty spot near destination
					for (int dx = -2; dx <= 2; dx++)
					{
						for (int dy = -2; dy <= 2; dy++)
						{
							int destX = pairedX + dx;
							int destY = pairedY + dy;
							if (destX >= 0 && destX < XRES && destY >= 0 && destY < YRES)
							{
								if (!pmap[destY][destX])
								{
									// Teleport!
									parts[rID].x = destX;
									parts[rID].y = destY;
									// Preserve velocity
									goto done_teleport;
								}
							}
						}
					}
					done_teleport:;
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int pairID = cpart->tmp;
	int anim = cpart->tmp2;

	// Color based on pair ID
	int colors[10][3] = {
		{255, 0, 255},    // 0: Magenta
		{255, 0, 0},      // 1: Red
		{0, 255, 0},      // 2: Green
		{0, 0, 255},      // 3: Blue
		{255, 255, 0},    // 4: Yellow
		{0, 255, 255},    // 5: Cyan
		{255, 128, 0},    // 6: Orange
		{128, 0, 255},    // 7: Purple
		{255, 255, 255},  // 8: White
		{128, 128, 128}   // 9: Gray
	};

	*colr = colors[pairID % 10][0];
	*colg = colors[pairID % 10][1];
	*colb = colors[pairID % 10][2];

	// Swirling portal effect
	int pulse = (anim < 20) ? anim : 40 - anim;
	*firea = 100 + pulse * 3;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD | PMODE_GLOW;

	return 0;
}
