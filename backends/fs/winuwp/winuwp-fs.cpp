#include <Windows.h>
#include <ppltasks.h>

#include "winuwp-stream.h"

#include "backends/fs/winuwp/winuwp-fs.h"
#include "backends/fs/stdiostream.h"
#include "common/memstream.h"
#include "common/bufferedstream.h"

using namespace concurrency;
using namespace Windows::Foundation;
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

Platform::String^ WinUWPFilesystemNode::getAccessListToken(const char* path) const {
	std::string str = std::string(path);
	std::replace(str.begin(), str.end(), '\\', '_');
	std::replace(str.begin(), str.end(), ':', '_');
	return ref new Platform::String(toUnicode(str.c_str()));
}

IStorageItem^ WinUWPFilesystemNode::getItemFromAccessList(Platform::String^ token) const {
	IStorageItem^ toReturn;
	volatile bool done = false;

	if (AccessCache::StorageApplicationPermissions::FutureAccessList->ContainsItem(token)) {
		auto t = create_task(AccessCache::StorageApplicationPermissions::FutureAccessList->GetItemAsync(token));
		t.then([&done, &toReturn](IStorageItem^ item) {
			toReturn = item;
			done = true;
		});
	}
	else {
		done = true;
	}

	auto window = CoreWindow::GetForCurrentThread();

	while (!done) {
		if (window != nullptr && window->Dispatcher->HasThreadAccess)
			window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneIfPresent);
	}
	return toReturn;
}

IStorageItem^ WinUWPFilesystemNode::getItemFromAccessList(const char* path) const {
	return getItemFromAccessList(getAccessListToken(path));
}

void WinUWPFilesystemNode::addStorageItem(AbstractFSList &myList, IStorageItem^ item) const {
	WinUWPFilesystemNode node;
	node._path = toAscii(item->Path->Data());
	node._displayName = toAscii(item->Name->Data());
	node._isValid = true;
	node._isDirectory = (item->Attributes & FileAttributes::Directory) == FileAttributes::Directory;
	node._isReadonly = (item->Attributes & FileAttributes::ReadOnly) == FileAttributes::ReadOnly;
	node._isPseudoRoot = false;

	myList.push_back(new WinUWPFilesystemNode(node));
}

void WinUWPFilesystemNode::addStorageItems(AbstractFSList &myList, IVectorView<IStorageItem^>^ items) const {
	for (auto it = items->First(); it->HasCurrent; it->MoveNext()) {
		addStorageItem(myList, it->Current);
	}
}

void WinUWPFilesystemNode::getAccessList(AbstractFSList &myList) const {
	auto entries = AccessCache::StorageApplicationPermissions::FutureAccessList->Entries;
	for (auto it = entries->First(); it->HasCurrent; it->MoveNext()) {
		auto item = getItemFromAccessList(it->Current.Token);
		addStorageItem(myList, item);
	}
	addStorageItem(myList, ApplicationData::Current->LocalFolder);
}

void WinUWPFilesystemNode::addItemsFromFolderPicker(Common::String & path) const {

	volatile bool done = false;

	Pickers::FolderPicker^ picker = ref new Pickers::FolderPicker();

	// Oddly enough, even a Folderpicker needs at least one FileTypeFilter.
	picker->FileTypeFilter->Append(".scummvm");
	picker->ViewMode = Pickers::PickerViewMode::Thumbnail;
	picker->SuggestedStartLocation = Pickers::PickerLocationId::Downloads;

	auto t = create_task(picker->PickSingleFolderAsync());
	t.then([this, &path, &done](StorageFolder^ folder) {
		if (folder == nullptr) {
			done = true;
			cancel_current_task();
			path = "";
		}
		char* p = toAscii(folder->Path->Data());
		AccessCache::StorageApplicationPermissions::FutureAccessList->AddOrReplace(getAccessListToken(p), folder);
		path = Common::String(p);
		done = true;
	});

	auto window = CoreWindow::GetForCurrentThread();
	while (!done) {
		if (window != nullptr && window->Dispatcher->HasThreadAccess)
			window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneIfPresent);
	}
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

	//We are first trying to get the item from FutureAccessList
	IStorageItem^ item = getItemFromAccessList(start);

	//if that fails, we are trying to fetch the item directly
	if (item == nullptr) {
		if (_path.lastChar() == '\\') {
			auto t = create_task(StorageFolder::GetFolderFromPathAsync(ref new Platform::String(toUnicode(start))));
			t.then([this, &item, &done](task<StorageFolder^> t1) {
				try
				{
					item = t1.get();
					done = true;
				}
				catch (...)
				{
					_path = "";
					_isValid = false;
					_isReadonly = true;
					_isDirectory = true;
					_isPseudoRoot = true;
					done = true;
				}
			});
		}
		else {
			auto t = create_task(StorageFile::GetFileFromPathAsync(ref new Platform::String(toUnicode(start))));
			t.then([&item, &done](task<StorageFile^> t1) {
				try
				{
					item = t1.get();
					done = true;
				}
				catch (...)
				{
					done = true;
				}
			});
		}

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
		return;
	}

	_path = "";
}

Common::String WinUWPFilesystemNode::getPath() const
{
	if (_isDirectory && !_path.empty() && _path.lastChar() != '\\')
		return _path + '\\';
	return _path;
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
	
	if(_isPseudoRoot)
		getAccessList(myList);
	else
	{
		auto t = create_task(StorageFolder::GetFolderFromPathAsync(path));
		t.then([](StorageFolder ^folder) -> task < IVectorView<IStorageItem ^> ^ > {
			return create_task(folder->GetItemsAsync());
		}).then([this, &myList, &done](task<IVectorView<IStorageItem ^> ^> t) {
			try {
				addStorageItems(myList, t.get());
				done = true;
			}
			catch (...) {
				done = true;
			}
		});

		auto window = CoreWindow::GetForCurrentThread();
		while (!done) {
			if (window != nullptr && window->Dispatcher->HasThreadAccess)
				window->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneIfPresent);
		}
	}	

	return true;
}

AbstractFSNode *WinUWPFilesystemNode::getParent() const {
	if (_isPseudoRoot) {
		Common::String path;
		addItemsFromFolderPicker(path);
		if (!path.empty()) {
			new WinUWPFilesystemNode(path);
		}			
	} else if (!_path.empty()) {
		const char *start = _path.c_str();
		const char *end = lastPathComponent(_path, '\\');
		Common::String path = Common::String(start, end - start);
		return new WinUWPFilesystemNode(path);
	}

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
