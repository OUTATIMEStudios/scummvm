#define FORBIDDEN_SYMBOL_ALLOW_ALL
#include <string>
#include <algorithm>
#include <Windows.h>
#include "common/scummsys.h"
#include "common/config-manager.h"
#include "backends\platform\sdl\winuwp\winuwp.h"
#include "backends\events\winuwpsdl\winuwpsdl-events.h"
#include "backends\fs\winuwp\winuwp-fs.h"
#include "backends\fs\winuwp\winuwp-fs-factory.h"
#include "backends\mixer\sdl\sdl-mixer.h"

using namespace Windows::Storage;

OSystem_WinUWP::OSystem_WinUWP() : _virtualKbd(false) {
	_fsFactory = new WinUWPFilesystemFactory();
}

void OSystem_WinUWP::initBackend() {
	StorageFolder ^installedLocation = Windows::ApplicationModel::Package::Current->InstalledLocation;
	StorageFolder ^local = ApplicationData::Current->LocalFolder;

	WinUWPFilesystemNode::createDir(local, "saves");

	ConfMan.set("vkeybdpath", Common::String(WinUWPFilesystemNode::toAscii(installedLocation->Path->Data())) + "\\Assets\\vkeybd\\");
	ConfMan.set("themepath", Common::String(WinUWPFilesystemNode::toAscii(installedLocation->Path->Data())) + "\\Assets\\themes\\");
	ConfMan.set("gui_theme", Common::String(WinUWPFilesystemNode::toAscii(installedLocation->Path->Data())) + "\\Assets\\themes\\scummmodern.zip");
	ConfMan.set("savepath", Common::String(WinUWPFilesystemNode::toAscii(local->Path->Data())) + "\\saves\\");

	if (_mixer == 0) {
		_mixerManager = new SdlMixerManager();
		_mixerManager->init();
	}

	if (_eventSource == 0)
		_eventSource = new WinUWPSdlEventSource();

	OSystem_SDL::initBackend();
	_gesture = ref new WinUWPGesture();

	SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
	SDL_ShowCursor(SDL_TRUE);	
}

Common::String OSystem_WinUWP::getDefaultConfigFileName() {
	StorageFolder ^local = ApplicationData::Current->LocalFolder;
	return Common::String(WinUWPFilesystemNode::toAscii(local->Path->Data())) + "\\scummvm.ini";
}

Common::String OSystem_WinUWP::getSystemLanguage() const
{
	auto language = Windows::System::UserProfile::GlobalizationPreferences::Languages->First()->Current->Data();
	static char asciiString[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, language, wcslen(language) + 1, asciiString, sizeof(asciiString), NULL, NULL);
	std::string str = std::string(asciiString);
	std::replace(str.begin(), str.end(), '-', '_');
	return Common::String(str.c_str());
}
