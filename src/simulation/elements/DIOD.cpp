#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_DIOD()
{
	Identifier = "DEFAULT_PT_DIOD";
	Name = "DIOD";
	Colour = 0x202020_rgb;
	MenuVisible = 1;
	MenuSection = SC_ELEC;
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
	Hardness = 1;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 0;   // Direction: 0=right, 1=down, 2=left, 3=up
	DefaultProperties.tmp2 = 0;  // Current flowing
	DefaultProperties.life = 0;  // Conducting state
	HeatConduct = 251;
	Description = "Diode. Only allows current flow in one direction (right by default). Tmp sets direction.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 500.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int direction = parts[i].tmp % 4;
	if (direction < 0) direction = 0;

	// Direction offsets
	int dx = 0, dy = 0;
	int inputDx = 0, inputDy = 0;
	switch (direction)
	{
		case 0: dx = 1; dy = 0; inputDx = -1; inputDy = 0; break;  // Right
		case 1: dx = 0; dy = 1; inputDx = 0; inputDy = -1; break;  // Down
		case 2: dx = -1; dy = 0; inputDx = 1; inputDy = 0; break;  // Left
		case 3: dx = 0; dy = -1; inputDx = 0; inputDy = 1; break;  // Up
	}

	bool forwardInput = false;
	bool reverseInput = false;

	// Check for inputs
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

				if (rt == PT_SPRK && parts[rID].life >= 3)
				{
					// Check if input is from forward direction (anode side)
					if (rx == inputDx && ry == inputDy)
					{
						forwardInput = true;
					}
					// Check if input is from reverse (cathode side) - blocked
					else if (rx == dx && ry == dy)
					{
						reverseInput = true;
					}
					// Side inputs - allow if they're more towards input side
					else if ((inputDx != 0 && rx == inputDx) || (inputDy != 0 && ry == inputDy))
					{
						forwardInput = true;
					}
				}

				// PSCN always acts as forward input
				if (rt == PT_PSCN)
				{
					for (int ddx = -1; ddx <= 1; ddx++)
					{
						for (int ddy = -1; ddy <= 1; ddy++)
						{
							int nx = x + rx + ddx;
							int ny = y + ry + ddy;
							if (nx >= 0 && nx < XRES && ny >= 0 && ny < YRES)
							{
								auto r2 = pmap[ny][nx];
								if (r2 && TYP(r2) == PT_SPRK)
								{
									forwardInput = true;
								}
							}
						}
					}
				}
			}
		}
	}

	parts[i].tmp2 = forwardInput ? 100 : 0;

	// Forward bias - conduct
	if (forwardInput && parts[i].life == 0)
	{
		parts[i].life = 4;

		// Output spark in forward direction
		int outX = x + dx;
		int outY = y + dy;

		if (outX >= 0 && outX < XRES && outY >= 0 && outY < YRES)
		{
			auto r = pmap[outY][outX];
			if (r)
			{
				auto rt = TYP(r);
				auto rID = ID(r);

				if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN ||
				     rt == PT_NSCN || rt == PT_RESI || rt == PT_DIOD)
				    && parts[rID].life == 0)
				{
					sim->part_change_type(rID, outX, outY, PT_SPRK);
					parts[rID].ctype = rt;
					parts[rID].life = 4;
				}
			}
		}

		// Also output to adjacent forward-direction cells
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if ((rx == dx || (dx == 0 && rx == 0)) &&
				    (ry == dy || (dy == 0 && ry == 0)) &&
				    (rx || ry))
				{
					auto r = pmap[y+ry][x+rx];
					if (r)
					{
						auto rt = TYP(r);
						auto rID = ID(r);

						if ((rt == PT_METL || rt == PT_INWR || rt == PT_NSCN)
						    && parts[rID].life == 0)
						{
							sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
							parts[rID].ctype = rt;
							parts[rID].life = 4;
						}
					}
				}
			}
		}
	}

	// Reverse bias - block (and heat up slightly)
	if (reverseInput)
	{
		parts[i].temp += 0.5f;
	}

	// Decay conducting state
	if (parts[i].life > 0)
	{
		parts[i].life--;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int direction = cpart->tmp % 4;
	int conducting = cpart->life;
	int current = cpart->tmp2;

	// Dark body with stripe indicating cathode
	*colr = 32;
	*colg = 32;
	*colb = 32;

	// Conducting glow
	if (conducting > 0)
	{
		*colr = 100;
		*colg = 100;
		*colb = 80;

		*firea = 30;
		*firer = 200;
		*fireg = 200;
		*fireb = 150;
		*pixel_mode |= FIRE_ADD;
	}

	// Show direction with a subtle tint
	switch (direction)
	{
		case 0: *colr += 20; break;  // Right - red tint
		case 1: *colg += 20; break;  // Down - green tint
		case 2: *colb += 20; break;  // Left - blue tint
		case 3: *colr += 10; *colg += 10; break;  // Up - yellow tint
	}

	return 0;
}
