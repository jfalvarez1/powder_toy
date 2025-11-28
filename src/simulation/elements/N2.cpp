#include "simulation/ElementCommon.h"

void Element::Element_N2()
{
	Identifier = "DEFAULT_PT_N2";
	Name = "NTRG";
	Colour = 0x80A0DF_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	// Nitrogen gas properties - similar to oxygen but inert
	Advection = 2.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 3.0f;
	HotAir = 0.000f * CFDS;
	Falldown = 0;

	Flammable = 0;  // Nitrogen is inert, non-flammable
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	HeatConduct = 70;
	Description = "Nitrogen gas. Inert, non-flammable. Liquefies at low temperatures.";

	Properties = TYPE_GAS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Nitrogen condenses to liquid nitrogen at 77K
	LowTemperature = 77.0f;
	LowTemperatureTransition = PT_LNTG;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
}
