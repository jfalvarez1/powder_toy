#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_CAPA()
{
	Identifier = "DEFAULT_PT_CAPA";
	Name = "CAPA";
	Colour = 0x4682B4_rgb;
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
	DefaultProperties.tmp = 0;    // Stored charge (0-1000)
	DefaultProperties.tmp2 = 100; // Capacitance (1-200, higher = more storage)
	DefaultProperties.life = 0;   // Discharge timer
	HeatConduct = 100;
	Description = "Capacitor. Stores charge and releases it. Blocks DC, passes AC. Tmp2 sets capacitance.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 400.0f;  // Electrolytic caps don't like heat
	HighTemperatureTransition = PT_BRMT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	int charge = parts[i].tmp;
	int capacitance = parts[i].tmp2;

	if (capacitance < 1) capacitance = 1;
	if (capacitance > 200) capacitance = 200;
	parts[i].tmp2 = capacitance;

	int maxCharge = capacitance * 10;
	bool inputDetected = false;
	int inputStrength = 0;

	// Check for input
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

				// Charge from spark
				if (rt == PT_SPRK && parts[rID].life >= 3)
				{
					inputDetected = true;
					inputStrength = 100;
				}

				// Charge from battery (continuous)
				if (rt == PT_BTRY)
				{
					inputDetected = true;
					inputStrength = 50;
				}

				// Charge from other capacitors
				if (rt == PT_CAPA && parts[rID].tmp > charge + 50)
				{
					// Charge flows from higher to lower
					int transfer = (parts[rID].tmp - charge) / 10;
					charge += transfer;
					parts[rID].tmp -= transfer;
				}
			}
		}
	}

	// Charge up
	if (inputDetected && charge < maxCharge)
	{
		int chargeRate = capacitance / 10 + 1;
		charge = std::min(charge + chargeRate * inputStrength / 50, maxCharge);
		parts[i].life = 0;  // Reset discharge timer while charging
	}

	// AC behavior: rapid charge changes cause output
	static int lastCharge = 0;
	int chargeChange = abs(charge - lastCharge);
	lastCharge = charge;

	// Discharge when charged and no input (or AC detected)
	if (charge > 100 && (!inputDetected || chargeChange > 20))
	{
		parts[i].life++;

		// Discharge delay based on capacitance (bigger cap = slower discharge)
		int dischargeDelay = capacitance / 20 + 1;

		if (parts[i].life >= dischargeDelay)
		{
			// Output spark
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

							if ((rt == PT_METL || rt == PT_INWR || rt == PT_PSCN ||
							     rt == PT_NSCN || rt == PT_RESI)
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

			// Discharge some charge
			charge -= 50 + (1000 - capacitance * 5);
			if (charge < 0) charge = 0;
			parts[i].life = 0;
		}
	}

	// Slow natural discharge (leakage)
	if (charge > 0 && sim->rng.chance(1, 100))
	{
		charge--;
	}

	parts[i].tmp = charge;

	// Overcharge explosion!
	if (charge > maxCharge * 1.5f)
	{
		sim->part_change_type(i, x, y, PT_PLSM);
		parts[i].life = 50;
		sim->pv[y/CELL][x/CELL] += 5.0f;
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int charge = cpart->tmp;
	int capacitance = cpart->tmp2;
	int maxCharge = capacitance * 10;

	// Base steel blue
	*colr = 70;
	*colg = 130;
	*colb = 180;

	// Brighter when charged
	if (charge > 0)
	{
		float chargeRatio = (float)charge / maxCharge;
		int brightness = (int)(chargeRatio * 100);

		*colr = std::min(70 + brightness, 200);
		*colg = std::min(130 + brightness / 2, 200);
		*colb = std::min(180 + brightness / 2, 255);

		// Glow effect when highly charged
		if (chargeRatio > 0.5f)
		{
			*firea = (int)(chargeRatio * 50);
			*firer = 100;
			*fireg = 150;
			*fireb = 255;
			*pixel_mode |= FIRE_ADD;
		}
	}

	return 0;
}
