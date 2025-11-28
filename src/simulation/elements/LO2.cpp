#include "simulation/ElementCommon.h"

void Element::Element_LO2()
{
	Identifier = "DEFAULT_PT_LO2";
	Name = "LOXY";
	Colour = 0x80A0EF_rgb;
	MenuVisible = 1;
	MenuSection = SC_LIQUID;
	Enabled = 1;

	Advection = 0.6f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.98f;
	Loss = 0.95f;
	Collision = 0.0f;
	Gravity = 0.1f;
	Diffusion = 0.00f;
	HotAir = 0.000f	* CFDS;
	Falldown = 2;

	Flammable = 5000;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 30;

	DefaultProperties.temp = 80.0f;
	HeatConduct = 70;
	Description = "Liquid Oxygen. Very cold. Reacts with fire. Freezes to solid oxygen.";

	Properties = TYPE_LIQUID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Oxygen freezes at ~54.36K
	LowTemperature = 54.36f;
	LowTemperatureTransition = PT_OICE;
	// Oxygen boils at ~90.2K
	HighTemperature = 90.1f;
	HighTemperatureTransition = PT_O2;
}
