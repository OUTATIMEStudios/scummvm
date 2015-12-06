#include <Windows.h>
#include <ppltasks.h>

#include "winuwp-stream.h"

#include "backends/fs/winuwp/winuwp-fs.h"
#include "backends/fs/stdiostream.h"
#include "common/memstream.h"
#include "common/bufferedstream.h"

using namespace concurrency;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Core;
using namespace Windows::Storage;

WinUWPFilesystemNode::WinUWPFilesystemNode() {
	_path = "";
	_isValid = false;
	_isReadonly = true;
	_isDirectory = true;
	_isPseudoRoot = true;
}

WinUWPFilesystemNode::WinUWPFilesystemNode(const Common::String &path) {
	_path = path;
	setFlags();
}

WinUWPFilesystemNode::WinUWPFilesystemNode(const Common::String &path, const Common::String &displayName) {
	_displayName = displayName;
	_isDirectory = true;
	_isValid = true;
	_isPseudoRoot = false;
	_path = path;
}

char* WinUWPFilesystemNode::toAscii(const wchar_t *str) {
	static char asciiString[MAX_PATH];
	WideCharToMultiByte(CP_ACP, 0, str, wcslen(str) + 1, asciiString, sizeof(asciiString), NULL, NULL);
	return asciiString;
}

const wchar_t* WinUWPFilesystemNode::toUnicode(const char *str) {
	static wchar_t unicodeString[MAX_PATH];
	MultiByteToWideChar(CP_ACP, 0, str, strlen(str) + 1, unicodeString, sizeof(unicodeString) / sizeof(wchar_t));
	return unicodeString;
}

void WinUWPFilesystemNode::createDir(StorageFolder^ dir, Platform::String ^subDir) {
	auto t = create_task(dir->TryGetItemAsync(subDir));
	t.then([dir, subDir](IStorageItem^ result) {
		if (result == nullptr) {
			create_task(dir->CreateFolderAsync(subDir)).then([](task<StorageFolder^> t) {
				try { t.get(); }
				catch (...) {}
			});
		}
	});
}

IStorageItem^ WinUWPFilesystemNode::getItemFromAccessList(const char* path)
{
	IStorageItem^ toReturn;
	volatile bool done = false;

	std::string str = std::string(path);
	std::replace(str.begin(), str.end(), '\\', '_');
	std::replace(str.begin(), str.end(), ':', '_');
	auto token = ref new Platform::String(toUnicode(str.c_str()));

	if (AccessCache::StorageApplicationPermissions::FutureAccessList->ContainsItem(token)) {
		auto t = create_task(AccessCache::StorageApplicationPermissions::FutureAccessList->GetItemAsync(token));
		t.then([&done, &toReturn](IStorageItem^ item) {
			toReturn = item;
			done = true;
		});
	}
	else
		done = true;

	auto window = CoreWindow::GetForCurrentThread();
	while (!done) {
		if (window != nullptr && window->Dispatcher->HasThreadAccess)
			window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneIfPresent);
	}

	return toReturn;
}

void WinUWPFilesystemNode::setFlags() {

	_isValid = false;
	_isDirectory = false;
	_isReadonly = false;
	_isPseudoRoot = false;

	if (_path.empty())
		return;

	volatile bool done = false;
	const char *start = _path.c_str();
	const char *end = lastPathComponent(_path, '\\');

	//We are first trying to get the item from FutureAccessList
	IStorageItem^ item = getItemFromAccessList(start);

	//if that fails, we are trying to fetch the item directly
	if (item == nullptr) {
		Common::String p = Common::String(start, end - start);
		Platform::String ^parent = ref new Platform::String(toUnicode(p.c_str()));
		Platform::String ^child = ref new Platform::String(toUnicode(end));

		auto t = create_task(StorageFolder::GetFolderFromPathAsync(parent));
		t.then([this, &done, child](StorageFolder ^folder) -> task < IStorageItem ^ > {
			return create_task(folder->GetItemAsync(child));
		}).then([this, &done, &item](task<IStorageItem ^> t) {
			try {
				item = t.get();
			}
			catch (...) {}
			done = true;
		});

		auto window = CoreWindow::GetForCurrentThread();
		while (!done) {
			if (window != nullptr && window->Dispatcher->HasThreadAccess)
				window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneIfPresent);
		}
	}

	if (item != nullptr) {
		_isValid = true;
		_isDirectory = (item->Attributes & FileAttributes::Directory) == FileAttributes::Directory;
		_isReadonly = (item->Attributes & FileAttributes::ReadOnly) == FileAttributes::ReadOnly;
		_displayName = toAscii(item->Name->Data());
		_isPseudoRoot = false;
	}
}

bool WinUWPFilesystemNode::isReadable() const {
	return _isValid;
}

bool WinUWPFilesystemNode::isWritable() const {
	return !_isReadonly;
}

bool WinUWPFilesystemNode::exists() const {
	return _isValid;
}

AbstractFSNode *WinUWPFilesystemNode::getChild(const Common::String &n) const {
	assert(_isDirectory);

	// Make sure the string contains no slashes
	assert(!n.contains('/'));

	Common::String newPath(_path);
	if (_path.lastChar() != '\\')
		newPath += '\\';
	newPath += n;

	return new WinUWPFilesystemNode(newPath);
}

bool WinUWPFilesystemNode::getChildren(AbstractFSList &myList, ListMode mode, bool hidden) const {

	volatile bool done = false;
	assert(_isDirectory);

	Platform::String ^path = ref new Platform::String(toUnicode(_path.c_str()));

	if (_isPseudoRoot || _path.empty()) {
		showFolderPicker(myList, done);
	}
	else {
		auto t = create_task(StorageFolder::GetFolderFromPathAsync(path));
		t.then([&myList, &done](StorageFolder ^folder) -> task < IVectorView<IStorageItem ^> ^ > {
			return create_task(folder->GetItemsAsync());
		}).then([&myList, &done](task<IVectorView<IStorageItem ^> ^> t) {
			try {
				IVectorView<IStorageItem ^> ^items = t.get();
				for (auto it = items->First(); it->HasCurrent; it->MoveNext()) {
					IStorageItem ^item = it->Current;
					WinUWPFilesystemNode node;
					node._path = toAscii(item->Path->Data());
					node._displayName = toAscii(item->Name->Data());
					node._isValid = true;
					node._isDirectory = (item->Attributes & FileAttributes::Directory) == FileAttributes::Directory;
					node._isReadonly = (item->Attributes & FileAttributes::ReadOnly) == FileAttributes::ReadOnly;
					node._isPseudoRoot = false;

					myList.push_back(new WinUWPFilesystemNode(node));
				}
			}
			catch (...) {
			}
			done = true;
		});
	}

	auto window = CoreWindow::GetForCurrentThread();
	while (!done) {
		if (window != nullptr && window->Dispatcher->HasThreadAccess)
			window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneIfPresent);
	}

	return true;
}

void WinUWPFilesystemNode::showFolderPicker(AbstractFSList &myList, volatile bool &done) const {

	Pickers::FolderPicker^ picker = ref new Pickers::FolderPicker();
	picker->SuggestedStartLocation = Pickers::PickerLocationId::Downloads;
	// Oddly enough, even a Folderpicker needs at least one FileTypeFilter.
	picker->FileTypeFilter->Append(".scummvm");
	picker->ViewMode = Pickers::PickerViewMode::Thumbnail;

	auto t = create_task(picker->PickSingleFolderAsync());
	t.then([this, &myList, &done](StorageFolder ^folder) {
		try {
			if (folder != nullptr) {
				std::string str = std::string(toAscii(folder->Path->Data()));
				std::replace(str.begin(), str.end(), '\\', '_');
				std::replace(str.begin(), str.end(), ':', '_');
				AccessCache::StorageApplicationPermissions::FutureAccessList->AddOrReplace(ref new Platform::String(toUnicode(str.c_str())), folder);
				myList.push_back(new WinUWPFilesystemNode(toAscii(folder->Path->Data()), toAscii(folder->Name->Data())));
			}
		}
		catch (...) {
		}
		done = true;
	});
}

AbstractFSNode *WinUWPFilesystemNode::getParent() const {

	if (_isPseudoRoot)
		return 0;

	// For reasons of simplicity we are going back to root (meaning we are showing the system's filepicker).
	// This should be changed to simulating a file system via the FutureAccessList entries.
	return new WinUWPFilesystemNode();
}

Common::SeekableReadStream *WinUWPFilesystemNode::createReadStream() {
	Common::SeekableReadStream* stream = StdioStream::makeFromPath(getPath(), false);
	//if the stream isn't correctly initialized we are outside the app's sandbox, 
	// thus we have to use the FileStorage API
	if (stream == nullptr)
		// wrapping the stream into a BufferedSeekableReadStream (thanks @wjp for the tip!)
		stream = Common::wrapBufferedSeekableReadStream(WinUWPStream::makeFromPath(getPath(), false), 1500, DisposeAfterUse::YES);
	return stream;
}

Common::WriteStream *WinUWPFilesystemNode::createWriteStream() {
	return StdioStream::makeFromPath(getPath(), true);
}
