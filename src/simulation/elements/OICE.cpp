#include "simulation/ElementCommon.h"

void Element::Element_OICE()
{
	Identifier = "DEFAULT_PT_OICE";
	Name = "OICE";
	Colour = 0x80C0FF_rgb;
	MenuVisible = 1;
	MenuSection = SC_SOLIDS;
	Enabled = 1;

	// Solid oxygen properties
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

	DefaultProperties.temp = 50.0f;
	HeatConduct = 46;
	Description = "Solid Oxygen. Very cold, melts into liquid oxygen when heated.";

	Properties = TYPE_SOLID;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	// Solid oxygen melts at ~54K
	HighTemperature = 54.36f;
	HighTemperatureTransition = PT_LO2;
}
