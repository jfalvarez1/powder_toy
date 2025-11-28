#include "simulation/ElementCommon.h"

void Element::Element_MRCV()
{
	Identifier = "DEFAULT_PT_MRCV";
	Name = "MRCV";
	Colour = 0xD0D0E0_rgb;
	MenuVisible = 1;
	MenuSection = SC_GAS;
	Enabled = 1;

	// Mercury vapor properties
	Advection = 1.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = 0.0f;
	Diffusion = 1.5f;  // Lower diffusion than lighter gases
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 2;  // Heavier than most gases

	HeatConduct = 40;
	Description = "Mercury Vapor. Toxic heavy metal gas, condenses back to liquid mercury.";

	Properties = TYPE_GAS | PROP_DEADLY;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Mercury vapor condenses at ~630K (boiling point)
	LowTemperature = 629.88f;
	LowTemperatureTransition = PT_MERC;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;
}
