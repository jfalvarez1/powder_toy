#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_LEDS()
{
	Identifier = "DEFAULT_PT_LEDS";
	Name = "LEDS";
	Colour = 0xFF0000_rgb;
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
	DefaultProperties.tmp = 0;   // Color: 0=red, 1=green, 2=blue, 3=yellow, 4=white
	DefaultProperties.tmp2 = 0;  // Brightness level
	DefaultProperties.life = 0;  // On timer
	HeatConduct = 251;
	Description = "LED. Lights up when powered. Tmp sets color (0-4: R,G,B,Y,W). Acts as diode.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 400.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int color = parts[i].tmp % 5;
	if (color < 0) color = 0;
	parts[i].tmp = color;

	bool powered = false;
	int brightness = 0;

	// Check for power input
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

				// Spark powers LED
				if (rt == PT_SPRK && parts[rID].life >= 3)
				{
					powered = true;
					brightness = std::max(brightness, parts[rID].life * 25);
				}

				// PSCN input (anode)
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
									powered = true;
									brightness = 100;
								}
							}
						}
					}
				}

				// Battery powers directly
				if (rt == PT_BTRY)
				{
					powered = true;
					brightness = 80;
				}
			}
		}
	}

	if (powered)
	{
		parts[i].tmp2 = brightness;
		parts[i].life = 8;  // Stay lit briefly

		// Pass current through (diode behavior) - output to NSCN side
		for (auto rx = -1; rx <= 1; rx++)
		{
			for (auto ry = -1; ry <= 1; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (r)
					{
						auto rt = TYP(r);
						auto rID = ID(r);

						if ((rt == PT_NSCN || rt == PT_METL || rt == PT_INWR)
						    && parts[rID].life == 0)
						{
							// Lower chance than direct wire (LED has voltage drop)
							if (sim->rng.chance(1, 3))
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
	}
	else
	{
		// Fade out
		if (parts[i].life > 0)
		{
			parts[i].life--;
			parts[i].tmp2 = parts[i].life * 12;
		}
		else
		{
			parts[i].tmp2 = 0;
		}
	}

	// Emit light particles occasionally when very bright
	if (parts[i].tmp2 > 80 && sim->rng.chance(1, 20))
	{
		int np = sim->create_part(-1, x, y - 1, PT_PHOT);
		if (np >= 0)
		{
			parts[np].vy = -2.0f;
			parts[np].vx = sim->rng.between(-10, 10) / 10.0f;
			// Set photon color based on LED color
			switch (color)
			{
				case 0: parts[np].ctype = 0x00FF0000; break;  // Red
				case 1: parts[np].ctype = 0x0000FF00; break;  // Green
				case 2: parts[np].ctype = 0x000000FF; break;  // Blue
				case 3: parts[np].ctype = 0x00FFFF00; break;  // Yellow
				case 4: parts[np].ctype = 0x00FFFFFF; break;  // White
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int color = cpart->tmp % 5;
	int brightness = cpart->tmp2;

	// Base colors for different LED types
	int baseR, baseG, baseB;
	switch (color)
	{
		case 0: baseR = 255; baseG = 0; baseB = 0; break;      // Red
		case 1: baseR = 0; baseG = 255; baseB = 0; break;      // Green
		case 2: baseR = 0; baseG = 0; baseB = 255; break;      // Blue
		case 3: baseR = 255; baseG = 255; baseB = 0; break;    // Yellow
		case 4: baseR = 255; baseG = 255; baseB = 255; break;  // White
		default: baseR = 255; baseG = 0; baseB = 0; break;
	}

	// Dim when off
	if (brightness == 0)
	{
		*colr = baseR / 4;
		*colg = baseG / 4;
		*colb = baseB / 4;
	}
	else
	{
		// Bright when on
		float bright = brightness / 100.0f;
		*colr = (int)(baseR * (0.25f + 0.75f * bright));
		*colg = (int)(baseG * (0.25f + 0.75f * bright));
		*colb = (int)(baseB * (0.25f + 0.75f * bright));

		// Glow effect
		*firea = brightness;
		*firer = baseR;
		*fireg = baseG;
		*fireb = baseB;
		*pixel_mode |= FIRE_ADD;
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
