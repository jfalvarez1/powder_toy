#include "simulation/ElementCommon.h"

void Element::Element_LH2()
{
	Identifier = "DEFAULT_PT_LH2";
	Name = "LH2";
	Colour = 0xCCCCFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	// Liquid hydrogen properties - extremely cold cryogenic liquid
	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f * CFDS;
	Falldown = 2;

	Flammable = 0;  // Liquid itself doesn't burn, the gas does
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 25;  // Very light liquid

	DefaultProperties.temp = 18.0f;
	HeatConduct = 70;
	Description = "Liquid Hydrogen. Extremely cold, evaporates into flammable hydrogen gas.";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Hydrogen freezes at ~14K (solid hydrogen not implemented, would need another element)
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	// Hydrogen boils at ~20K
	HighTemperature = 20.28f;
	HighTemperatureTransition = PT_H2;
}
