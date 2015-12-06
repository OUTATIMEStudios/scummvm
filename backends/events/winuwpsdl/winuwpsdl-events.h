#if !defined(BACKEND_EVENTS_WINUWP_H) && !defined(DISABLE_DEFAULT_EVENTMANAGER)
#define BACKEND_EVENTS_WINUWP_H

#include "backends/events/sdl/sdl-events.h"

class WinUWPSdlEventSource : public SdlEventSource {
protected:
	bool dispatchSDLEvent(SDL_Event &ev, Common::Event &event);
private:
	void TransformInput(int &x, int &y, SDL_Event &ev);
	void TransformTouchInput(int & x, int & y, SDL_Event & ev);
};
#endif