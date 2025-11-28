#include "PowderToySDL.h"
#include "gui/interface/Engine.h"
#include <iostream>

void MainLoop()
{
	std::cout << "DEBUG: MainLoop() entered, Running()=" << (ui::Engine::Ref().Running() ? "true" : "false") << std::endl;
	int loopCount = 0;
	while (ui::Engine::Ref().Running())
	{
		if (loopCount < 5)
		{
			std::cout << "DEBUG: MainLoop iteration " << loopCount << std::endl;
		}
		loopCount++;
		auto delay = EngineProcess();
		if (delay.has_value())
		{
			SDL_Delay(std::max(*delay, UINT64_C(1)));
		}
	}
	std::cout << "DEBUG: MainLoop() exiting after " << loopCount << " iterations" << std::endl;
}

void ApplyFpsLimit()
{
}
