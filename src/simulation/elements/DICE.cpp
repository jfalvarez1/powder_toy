#include "simulation/ElementCommon.h"

void Element::Element_DICE()
{
	Identifier = "DEFAULT_PT_DICE";
	Name = "DICE";
	Colour = 0x80C0EF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	// Deuterium ice (frozen heavy water) properties
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
	Meltable = 0;
	Hardness = 20;

	Weight = 100;

	DefaultProperties.temp = 270.0f;
	HeatConduct = 46;
	Description = "Deuterium Ice. Frozen heavy water, melts into deuterium.";

	Properties = TYPE_SOLID | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	// Heavy water freezes at ~276.97K (3.82C) - slightly higher than regular water
	HighTemperature = 276.97f;
	HighTemperatureTransition = PT_DEUT;
}
