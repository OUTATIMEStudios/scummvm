#include "backends\graphics\surfacesdl\surfacesdl-graphics.h"

class WinUWPGraphicsManager : public SurfaceSdlGraphicsManager {

public:
	WinUWPGraphicsManager(SdlEventSource *sdlEventSource, SdlWindow *window)
		: SurfaceSdlGraphicsManager(sdlEventSource, window) {

	}
	virtual void transformMouseCoordinates(Common::Point &point);
};