#include "engine.h"

void ThirdEngine::init()
{
	_vulkanContext.init();
	_renderer.init(_vulkanContext);
}

void ThirdEngine::run()
{

}

void ThirdEngine::cleanup()
{
	_renderer.cleanup();
	_vulkanContext.cleanup();
}