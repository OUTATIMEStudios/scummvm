#include "SDL_main.h"
#include <wrl.h>

#include "backends/platform/sdl/winuwp/winuwp.h"
#include "base/main.h"

int CALLBACK WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	if (FAILED(Windows::Foundation::Initialize(RO_INIT_MULTITHREADED))) {
		return 1;
	}
	
	SDL_WinRTRunApp(SDL_main, NULL);
	return 0;
}

int main(int argc, char **argv)
{
	g_system = new OSystem_WinUWP();
	((OSystem_WinUWP *)g_system)->init();
	int res = scummvm_main(0, NULL);
	return res;
}