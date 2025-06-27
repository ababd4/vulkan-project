#include "engine.h"

void ThirdEngine::init()
{
	m_renderWindow.init();
	m_vulkanContext.init();
	m_renderer.init(m_vulkanContext);
}

void ThirdEngine::run()
{

}

void ThirdEngine::cleanup()
{
	m_renderer.cleanup();
	m_vulkanContext.cleanup();
}