#include "simulation/ElementCommon.h"

void Element::Element_AICD()
{
	Identifier = "DEFAULT_PT_AICD";
	Name = "AICD";
	Colour = 0xC040E0_rgb;
	MenuVisible = 0;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	Advection = 0.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.90f;
	Loss = 0.00f;
	Collision = 0.0f;
	Gravity = 0.0f;
	Diffusion = 0.00f;
	HotAir = -0.0003f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 1;
	Hardness = 30;

	Weight = 100;

	// Concentrated acids freeze around -40C (233K)
	DefaultProperties.temp = 230.0f;
	HeatConduct = 150;
	Description = "Frozen Acid. Solid acid, melts into corrosive liquid when warmed.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	// Melts back to liquid acid
	HighTemperature = 233.0f;
	HighTemperatureTransition = PT_ACID;
}
