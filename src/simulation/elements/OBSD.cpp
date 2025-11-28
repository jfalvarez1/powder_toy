#include "simulation/ElementCommon.h"

static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_OBSD()
{
	Identifier = "DEFAULT_PT_OBSD";
	Name = "OBSD";
	Colour = 0x1A1A2E_rgb;
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
	Meltable = 1;
	Hardness = 0;  // Very hard, doesn't break

	Weight = 100;

	DefaultProperties.temp = R_TEMP + 273.15f;
	HeatConduct = 150;
	Description = "Obsidian. Volcanic glass formed when lava cools rapidly. Extremely hard and sharp.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = 200.0f;  // Shatters under extreme pressure
	HighPressureTransition = PT_PQRT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	HighTemperature = 1700.0f;  // Melts back to lava
	HighTemperatureTransition = PT_LAVA;

	Graphics = &graphics;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Dark with purple/blue sheen
	*colr = 26 + (cpart->tmp % 10);
	*colg = 26 + (cpart->tmp % 8);
	*colb = 46 + (cpart->tmp % 15);

	// Slight reflective sheen
	*pixel_mode |= PMODE_GLOW;

	return 0;
}
