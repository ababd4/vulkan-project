#include "Engine.h"

int main(int argc, char* argv[])
{
	ThirdEngine engine;

	engine.init();
	engine.run();
	engine.cleanup();

	return 0;
}