#include "simulation/ElementCommon.h"

void Element::Element_ACDV()
{
	Identifier = "DEFAULT_PT_ACDV";
	Name = "ACDV";
	Colour = 0xE080FF_rgb;
	MenuVisible = 0;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.05f;
	Diffusion = 2.0f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	// Concentrated acids boil around 337C (610K)
	DefaultProperties.temp = 620.0f;
	HeatConduct = 50;
	Description = "Acid Vapor. Corrosive gas, condenses into acid when cooled. Deadly to breathe.";

	Properties = TYPE_GAS | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Condenses back to liquid acid
	LowTemperature = 600.0f;
	LowTemperatureTransition = PT_ACID;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
}
