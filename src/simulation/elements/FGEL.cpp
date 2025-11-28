#include "simulation/ElementCommon.h"

static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_FGEL()
{
	Identifier = "DEFAULT_PT_FGEL";
	Name = "FGEL";
	Colour = 0xB87000_rgb;
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
	Hardness = 25;

	Weight = 100;

	// Gel is water-based, freezes slightly below water freezing point
	DefaultProperties.temp = 260.0f;
	HeatConduct = 29;
	Description = "Frozen Gel. Solid gel, melts when warmed. Retains water content.";

	Properties = TYPE_SOLID | PROP_NEUTPENETRATE;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	LowTemperature = ITL;
	LowTemperatureTransition = NT;
	// Melts back to gel
	HighTemperature = 263.0f;
	HighTemperatureTransition = PT_GEL;

	Graphics = &graphics;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	// Preserve the gel coloring based on water content (tmp)
	int q = cpart->tmp;
	*colr = q*(32-184)/120+184;
	*colg = q*(48-112)/120+112;
	*colb = q*208/120;
	return 0;
}
