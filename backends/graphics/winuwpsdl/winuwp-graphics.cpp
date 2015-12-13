#include "winuwp-graphics.h"

void WinUWPGraphicsManager::transformMouseCoordinates(Common::Point & point)
{
	int windowWidth = 1, windowHeight = 1;
	SDL_GetWindowSize(_window->getSDLWindow(), &windowWidth, &windowHeight);
	
	point.x = (int)(((float)point.x / windowWidth) * _videoMode.overlayWidth);
	point.y = (int)(((float)point.y / windowHeight) * _videoMode.overlayHeight);

	if (!_overlayVisible) {
		point.x /= _videoMode.scaleFactor;
		point.y /= _videoMode.scaleFactor;
	}
}
