#include "simulation/ElementCommon.h"

void Element::Element_DTRV()
{
	Identifier = "DEFAULT_PT_DTRV";
	Name = "DTRV";
	Colour = 0x00AAFF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	// Deuterium vapor (heavy water steam) properties
	Advection = 1.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 2.5f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	HeatConduct = 80;
	Description = "Deuterium Vapor. Heavy water steam, condenses into deuterium liquid.";

	Properties = TYPE_GAS | PROP_NEUTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Heavy water boils at ~374.5K (101.4C)
	LowTemperature = 374.5f;
	LowTemperatureTransition = PT_DEUT;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
}
