#ifndef PLATFORM_SDL_WINUWP_H
#define PLATFORM_SDL_WINUWP_H

#include "backends/platform/sdl/sdl.h"
#include "winuwp-gesture.h"

class OSystem_WinUWP : public OSystem_SDL {

private:
	bool _virtualKbd;
	WinUWPGesture^ _gesture;
public:
	OSystem_WinUWP();
	virtual ~OSystem_WinUWP() {};
	virtual void initBackend();
	virtual Common::String getDefaultConfigFileName();
	virtual Common::String getSystemLanguage() const;
};

#endif