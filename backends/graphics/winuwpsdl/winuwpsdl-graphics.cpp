#include "common/scummsys.h"
#include "winuwpsdl-graphics.h"

WinUWPSdlGraphicsManager::WinUWPSdlGraphicsManager(SdlEventSource *sdlEventSource, SdlWindow *window) :
	SurfaceSdlGraphicsManager::SurfaceSdlGraphicsManager(sdlEventSource, window)
{
}

WinUWPSdlGraphicsManager::~WinUWPSdlGraphicsManager()
{
}
