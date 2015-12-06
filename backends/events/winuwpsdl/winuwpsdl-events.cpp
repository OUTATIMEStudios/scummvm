#include "common/util.h"
#include "common/scummsys.h"
#include "backends/events/winuwpsdl/winuwpsdl-events.h"
#include "backends/platform/sdl/sdl.h"
#include "engines/engine.h"
#include "common/events.h"

bool WinUWPSdlEventSource::dispatchSDLEvent(SDL_Event &ev, Common::Event &event)
{
	int x, y = 0;

	switch (ev.type)
	{
	case SDL_MOUSEMOTION:
	case SDL_MOUSEBUTTONDOWN:
	case SDL_MOUSEBUTTONUP:
		TransformInput(x, y, ev);
		break;
	case SDL_FINGERMOTION:
	case SDL_FINGERDOWN:
	case SDL_FINGERUP:
		TransformTouchInput(x, y, ev);
		break;
	default:
		break;
	}

	return SdlEventSource::dispatchSDLEvent(ev, event);
}

void WinUWPSdlEventSource::TransformInput(int &x, int &y, SDL_Event &ev)
{
	SDL_GetWindowSize(SDL_GetWindowFromID(ev.window.windowID), &x, &y);
	ev.motion.x = static_cast<int>(ev.motion.x * ((float)g_system->getOverlayWidth() / x));
	ev.motion.y = static_cast<int>(ev.motion.y * ((float)g_system->getOverlayHeight() / y));
	x = ev.motion.x;
	y = ev.motion.y;
}

void WinUWPSdlEventSource::TransformTouchInput(int &x, int &y, SDL_Event &ev)
{
	x = static_cast<int>(g_system->getOverlayWidth() * ev.tfinger.x);
	y = static_cast<int>(g_system->getOverlayHeight() * ev.tfinger.y);
	ev.motion.x = x;
	ev.motion.y = y;
}