#include "simulation/ElementCommon.h"

static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_RKVP()
{
	Identifier = "DEFAULT_PT_RKVP";
	Name = "RKVP";
	Colour = 0xFF6030_rgb;
	MenuVisible = 0;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 1.0f;
	AirDrag = 0.01f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.1f;
	Diffusion = 0.75f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	// Rock vaporizes at extremely high temperatures (around 3500K)
	DefaultProperties.temp = 3500.0f;
	HeatConduct = 100;
	Description = "Rock Vapor. Hot vaporized rock and minerals, condenses into lava when cooled.";

	Properties = TYPE_GAS;
	CarriesTypeIn = 1U << FIELD_CTYPE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Condenses back to lava below vaporization point
	LowTemperature = 3200.0f;
	LowTemperatureTransition = PT_LAVA;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Graphics = &graphics;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*colr = 0xFF;
	*colg = 0x60 + int((cpart->temp - 3000) / 50);
	*colb = 0x30 + int((cpart->temp - 3000) / 100);
	if (*colg > 0xFF) *colg = 0xFF;
	if (*colb > 0xFF) *colb = 0xFF;
	*firea = 80;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD;
	*pixel_mode |= PMODE_BLUR;
	return 0;
}
