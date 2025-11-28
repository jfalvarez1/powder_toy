#include "simulation/ElementCommon.h"

static int update(UPDATE_FUNC_ARGS);
static int graphics(GRAPHICS_FUNC_ARGS);

void Element::Element_ISVP()
{
	Identifier = "DEFAULT_PT_ISVP";
	Name = "ISVP";
	Colour = 0xCC50E0_rgb;
	MenuVisible = 0;
	MenuSection = SC_GAS;
	Enabled = 1;

	Advection = 2.0f;
	AirDrag = 0.00f * CFDS;
	AirLoss = 0.99f;
	Loss = 0.30f;
	Collision = -0.1f;
	Gravity = -0.05f;
	Diffusion = 2.5f;
	HotAir = 0.001f * CFDS;
	Falldown = 0;

	Flammable = 0;
	Explosive = 0;
	Meltable = 0;
	Hardness = 0;

	Weight = 1;

	// Isotope-Z boils at around 400K (fictional)
	DefaultProperties.temp = 420.0f;
	HeatConduct = 29;
	Description = "Isotope-Z Vapor. Radioactive gas, decays into photons. Condenses into ISOZ when cooled.";

	Properties = TYPE_GAS | PROP_NEUTPENETRATE | PROP_PHOTPASS;

	LowPressure = IPL;
	LowPressureTransition = NT;
	HighPressure = IPH;
	HighPressureTransition = NT;
	// Condenses back to liquid ISOZ
	LowTemperature = 380.0f;
	LowTemperatureTransition = PT_ISOZ;
	HighTemperature = ITH;
	HighTemperatureTransition = NT;

	Update = &update;
	Graphics = &graphics;
}

static int update(UPDATE_FUNC_ARGS)
{
	// Similar radioactive decay as liquid ISOZ, but more likely to decay as gas
	float rr, rrr;
	if (sim->rng.chance(1, 150) && sim->rng.chance(int(-4.0f * sim->pv[y/CELL][x/CELL]), 1000))
	{
		sim->create_part(i, x, y, PT_PHOT);
		rr = sim->rng.between(128, 355) / 127.0f;
		rrr = sim->rng.between(0, 359) * 3.14159f / 180.0f;
		parts[i].vx = rr*cosf(rrr);
		parts[i].vy = rr*sinf(rrr);
	}
	return 0;
}

static int graphics(GRAPHICS_FUNC_ARGS)
{
	*firea = 40;
	*firer = *colr;
	*fireg = *colg;
	*fireb = *colb;
	*pixel_mode |= FIRE_ADD;
	*pixel_mode |= PMODE_BLUR;
	return 0;
}
