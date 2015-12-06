#ifndef WINUWP_FILESYSTEM_H
#define WINUWP_FILESYSTEM_H

#include "backends/fs/abstract-fs.h"

#define MAX_PATH 260

using namespace Windows::Storage;


class WinUWPFilesystemNode : public AbstractFSNode {
private:
	bool _isReadonly;
	void setFlags();
	static Platform::String^ _falToken;

	void showFolderPicker(AbstractFSList &myList, volatile bool &done) const;

protected:
	Common::String _displayName;
	Common::String _path;
	bool _isPseudoRoot;
	bool _isDirectory;
	bool _isValid;
public:


	WinUWPFilesystemNode();
	WinUWPFilesystemNode(const Common::String &path);

	/**
	* Constructor to create a pseudo root entry
	*
	* @param path Common::String with the path the new node should point to.
	* @param displayName Common::String with the name of the pseudo root node.
	*/
	WinUWPFilesystemNode(const Common::String &path, const Common::String &displayName);

	virtual bool exists() const;
	virtual Common::String getDisplayName() const { return _displayName; }
	virtual Common::String getName() const { return _displayName; }
	virtual Common::String getPath() const { return _path; }
	virtual bool isDirectory() const { return _isDirectory; }
	virtual bool isReadable() const;
	virtual bool isWritable() const;

	virtual AbstractFSNode *getChild(const Common::String &n) const;
	virtual bool getChildren(AbstractFSList &list, ListMode mode, bool hidden) const;

	virtual AbstractFSNode *getParent() const;

	virtual Common::SeekableReadStream *createReadStream();
	virtual Common::WriteStream *createWriteStream();

	static void createDir(StorageFolder^ dir, Platform::String ^subDir);

	IStorageItem ^ getItemFromAccessList(const char * path);

	/**
	* Converts a Unicode string to Ascii format.
	*
	* @param str Common::String to convert from Unicode to Ascii.
	* @return str in Ascii format.
	*/
	static char *toAscii(const wchar_t *str);

	/**
	* Converts an Ascii string to Unicode format.
	*
	* @param str Common::String to convert from Ascii to Unicode.
	* @return str in Unicode format.
	*/
	static const wchar_t* toUnicode(const char *str);

};

#endif