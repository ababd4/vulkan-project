#include "Core/Engine.h"

int main(int argc, char* argv[])
{
	ThirdEngine engine;

	engine.Init();
	engine.Run();
	engine.Cleanup();

	return 0;
}