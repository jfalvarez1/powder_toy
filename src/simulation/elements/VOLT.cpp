#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_VOLT()
{
	Identifier = "DEFAULT_PT_VOLT";
	Name = "VOLT";
	Colour = 0x00AA00_rgb;
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
	DefaultProperties.tmp = 0;    // Measured voltage (0-500)
	DefaultProperties.tmp2 = 0;   // Peak voltage recorded
	DefaultProperties.life = 0;   // Sample counter
	HeatConduct = 50;
	Description = "Voltmeter. Measures voltage from nearby VCCS or spark intensity. Green=low, Yellow=med, Red=high.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 600.0f;
	HighTemperatureTransition = PT_FIRE;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int voltage = 0;
	int sparkCount = 0;

	// Scan for voltage sources and sparks
	for (auto rx = -2; rx <= 2; rx++)
	{
		for (auto ry = -2; ry <= 2; ry++)
		{
			if (rx || ry)
			{
				if (x + rx < 0 || x + rx >= XRES || y + ry < 0 || y + ry >= YRES)
					continue;

				auto r = pmap[y+ry][x+rx];
				if (!r)
					continue;
				auto rt = TYP(r);
				auto rID = ID(r);

				// Measure voltage from VCCS
				if (rt == PT_VCCS)
				{
					voltage = std::max(voltage, (int)parts[rID].tmp2);
				}

				// Measure from BTRY
				if (rt == PT_BTRY)
				{
					voltage = std::max(voltage, 50);
				}

				// Measure spark intensity
				if (rt == PT_SPRK && parts[rID].life >= 2)
				{
					sparkCount++;
					// Estimate voltage from spark origin
					int srcType = parts[rID].ctype;
					if (srcType == PT_PSCN || srcType == PT_NSCN)
						voltage = std::max(voltage, 30 + sparkCount * 10);
					else
						voltage = std::max(voltage, 20 + sparkCount * 10);
				}

				// Measure from signal generator
				if (rt == PT_SGNL)
				{
					voltage = std::max(voltage, 50);
				}

				// Measure from capacitor charge
				if (rt == PT_CAPA)
				{
					int charge = parts[rID].tmp;
					voltage = std::max(voltage, charge / 10);
				}
			}
		}
	}

	// Smooth reading (moving average)
	parts[i].tmp = (parts[i].tmp * 3 + voltage) / 4;

	// Track peak
	if (voltage > parts[i].tmp2)
		parts[i].tmp2 = voltage;

	// Slowly decay peak
	if (sim->rng.chance(1, 50) && parts[i].tmp2 > 0)
		parts[i].tmp2--;

	// Sample counter for averaging
	parts[i].life = (parts[i].life + 1) % 100;

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int voltage = cpart->tmp;
	int peak = cpart->tmp2;

	// Color based on voltage level:
	// Green (0-50), Yellow (50-150), Orange (150-300), Red (300+)
	if (voltage < 50)
	{
		*colr = voltage * 2;
		*colg = 170;
		*colb = 0;
	}
	else if (voltage < 150)
	{
		*colr = 100 + voltage;
		*colg = 170;
		*colb = 0;
	}
	else if (voltage < 300)
	{
		*colr = 255;
		*colg = 170 - (voltage - 150);
		*colb = 0;
	}
	else
	{
		*colr = 255;
		*colg = 20;
		*colb = 0;
	}

	// Glow when reading voltage
	if (voltage > 0)
	{
		*firea = std::min(voltage / 3, 80);
		*firer = *colr;
		*fireg = *colg;
		*fireb = 0;
		*pixel_mode |= FIRE_ADD;
	}

	// Extra glow for peak readings
	if (peak > 100)
	{
		*pixel_mode |= PMODE_GLOW;
	}

	return 0;
}
