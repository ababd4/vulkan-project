#include "engine.h"

int main()
{
	ThirdEngine engine;

	engine.init();
	engine.run();
	engine.cleanup();

	return 0;
}