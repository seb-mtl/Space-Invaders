#include "GameEngine.h"

void EngineMain()
{
	GameEngine engine;

	double end;
	double start = engine.getStopwatchElapsedSeconds();

	while (engine.startFrame())
	{
		engine.handleEvents();
		engine.update();
		engine.draw();

		// I am not up to date with the ways how game loops today deal to
		// limit fps (60fps). Sleeps are a possible way to slow down the
		// execution or rendering, but e.g. on Windows for example the scheduler has
		// a a turnaround time of 15ms per thread (at least in user mode), except
		// for specific interrupts. Therefore I keep the following code
		// unimplemented
		/*
    usleep(time to limit to 60 or 30 fps);
    */
	}
}
