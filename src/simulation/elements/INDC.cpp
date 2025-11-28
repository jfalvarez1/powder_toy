#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_INDC()
{
	Identifier = "DEFAULT_PT_INDC";
	Name = "INDC";
	Colour = 0xCD853F_rgb;
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
	Meltable = 1;
	Hardness = 1;

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	DefaultProperties.tmp = 50;   // Inductance (1-100)
	DefaultProperties.tmp2 = 0;   // Stored magnetic energy / current
	DefaultProperties.life = 0;   // State timer
	HeatConduct = 200;
	Description = "Inductor. Opposes changes in current, stores energy in magnetic field. Tmp sets inductance.";

	Properties = TYPE_SOLID | PROP_CONDUCTS | PROP_LIFE_DEC;

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
	int inductance = parts[i].tmp;
	if (inductance < 1) inductance = 1;
	if (inductance > 100) inductance = 100;
	parts[i].tmp = inductance;

	int storedEnergy = parts[i].tmp2;
	bool inputDetected = false;
	int inputStrength = 0;

	// Check for spark input
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
					inputDetected = true;
					inputStrength = 100;
				}
			}
		}
	}

	// Inductor behavior: opposes current changes
	// When current starts: slow to conduct (stores energy)
	// When current stops: releases stored energy (back-EMF)

	if (inputDetected)
	{
		// Current is flowing in - build up slowly
		int chargeRate = (100 - inductance) / 10 + 1;
		storedEnergy = std::min(storedEnergy + chargeRate, inductance * 10);

		// Only conduct after building up enough energy
		if (storedEnergy > inductance * 3)
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
		}
	}
	else if (storedEnergy > 0)
	{
		// No input but has stored energy - back-EMF!
		// Release energy gradually (or suddenly if broken)

		parts[i].life++;

		// Release spark (back-EMF)
		if (parts[i].life > 2)
		{
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
							     rt == PT_NSCN)
							    && parts[rID].life == 0)
							{
								// Back-EMF spark
								sim->part_change_type(rID, x+rx, y+ry, PT_SPRK);
								parts[rID].ctype = rt;
								parts[rID].life = 4;
							}
						}
					}
				}
			}

			// Discharge energy
			int dischargeRate = (100 - inductance) / 5 + 5;
			storedEnergy -= dischargeRate;
			if (storedEnergy < 0) storedEnergy = 0;
			parts[i].life = 0;
		}
	}
	else
	{
		parts[i].life = 0;
	}

	parts[i].tmp2 = storedEnergy;

	// Generate small magnetic field (affects nearby MGNT)
	if (storedEnergy > 50)
	{
		for (auto rx = -2; rx <= 2; rx++)
		{
			for (auto ry = -2; ry <= 2; ry++)
			{
				if (rx || ry)
				{
					auto r = pmap[y+ry][x+rx];
					if (r && TYP(r) == PT_MGNT)
					{
						// Attract/repel magnet slightly
						parts[ID(r)].vx += rx * 0.01f * storedEnergy / 100.0f;
						parts[ID(r)].vy += ry * 0.01f * storedEnergy / 100.0f;
					}
				}
			}
		}
	}

	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	int inductance = cpart->tmp;
	int energy = cpart->tmp2;
	int maxEnergy = inductance * 10;

	// Copper/brown color for coil
	*colr = 205;
	*colg = 133;
	*colb = 63;

	// Glow based on stored energy
	if (energy > 0)
	{
		float energyRatio = (float)energy / maxEnergy;
		int brightness = (int)(energyRatio * 80);

		*firea = brightness;
		*firer = 200;
		*fireg = 150;
		*fireb = 50;
		*pixel_mode |= FIRE_ADD;

		// Magnetic field visualization
		if (energyRatio > 0.5f)
		{
			*pixel_mode |= PMODE_GLOW;
		}
	}

	return 0;
}
