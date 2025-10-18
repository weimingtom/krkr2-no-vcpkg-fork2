//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Universal Storage System
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

// must before with Platform.h because marco will replece `st_atime` symbol!
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>
#include <cocos/cocos2d.h>

#include "MsgIntf.h"

#include "StorageImpl.h"
#include "WindowImpl.h"
#include "SysInitIntf.h"
#include "DebugIntf.h"
#include "Random.h"
#include "XP3Archive.h"
#include "FileSelector.h"

#include "Application.h"
#include "StringUtil.h"
#include "FilePathUtil.h"
#include "Platform.h"
#include "dirent.h"
#include "TickCount.h"
#include "combase.h"

#include "spdlog/spdlog.h"

#if defined(WIN32) && !defined(__MINGW32__)
#include <io.h>
// posix io api
inline unsigned int lseek64(int fileHandle, __int64 offset, int origin) {
    return _lseeki64(fileHandle, offset, origin);
}
// extern void *valloc(int n);
// extern void vfree(void *p);
// }
#endif
#ifdef CC_TARGET_OS_IPHONE
#define lseek64 lseek
#endif

#if defined(__APPLE__) || defined(__MACH__) || defined(__FreeBSD__) || defined(__MINGW32__)
#define lseek64 lseek
#endif

//---------------------------------------------------------------------------
// tTVPFileMedia
//---------------------------------------------------------------------------
class tTVPFileMedia : public iTVPStorageMedia {
    tjs_uint RefCount;

public:
    tTVPFileMedia() { RefCount = 1; }

    ~tTVPFileMedia() override {}

    void AddRef() override { RefCount++; }

    void Release() override {
        if(RefCount == 1)
            delete this;
        else
            RefCount--;
    }

    void GetName(ttstr &name) override { name = TJS_W("file"); }

    void NormalizeDomainName(ttstr &name) override;

    void NormalizePathName(ttstr &name) override;

    bool CheckExistentStorage(const ttstr &name) override;

    tTJSBinaryStream *Open(const ttstr &name, tjs_uint32 flags) override;

    void GetListAt(const ttstr &name, iTVPStorageLister *lister) override;

    void GetLocallyAccessibleName(ttstr &name) override;

    void GetLocalName(ttstr &name);
};

//---------------------------------------------------------------------------
void tTVPFileMedia::NormalizeDomainName(ttstr &name) {
    // normalize domain name
    // make all characters small
    tjs_char *p = name.Independ();
    while(*p) {
        if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
            *p += TJS_W('a') - TJS_W('A');
        p++;
    }
}

//---------------------------------------------------------------------------
void tTVPFileMedia::NormalizePathName(ttstr &name) {
    // normalize path name
    // make all characters small
    tjs_char *p = name.Independ();
    while(*p) {
        if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
            *p += TJS_W('a') - TJS_W('A');
        p++;
    }
}

//---------------------------------------------------------------------------
bool tTVPFileMedia::CheckExistentStorage(const ttstr &name) {
    if(name.IsEmpty())
        return false;

    ttstr _name(name);
    GetLocalName(_name);

    return TVPCheckExistentLocalFile(_name);
}

//---------------------------------------------------------------------------
tTJSBinaryStream *tTVPFileMedia::Open(const ttstr &name, tjs_uint32 flags) {
    // open storage named "name".
    // currently only local/network(by OS) storage systems are
    // supported.
    if(name.IsEmpty())
        TVPThrowExceptionMessage(TVPCannotOpenStorage, TJS_W("\"\""));

    ttstr origname = name;
    ttstr _name(name);
    GetLocalName(_name);

    return new tTVPLocalFileStream(origname, _name, flags);
}

void TVPListDir(const std::string &u8folder,
                std::function<void(const std::string &, int)> cb) {

#ifdef _WIN32
    // ---------------- Windows 分支 ----------------
    namespace fs = std::filesystem;

    int wlen =
        MultiByteToWideChar(CP_UTF8, 0, u8folder.c_str(), -1, nullptr, 0);
    if(wlen <= 0)
        return;
    std::wstring wfolder(wlen - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, u8folder.c_str(), -1, wfolder.data(),
                        wlen - 1);
    try {
        auto begin = std::filesystem::directory_iterator(wfolder);
        for(const auto &entry : begin) {
            // 文件名（UTF-8）
            std::string name = entry.path().filename().u8string();
            // 文件类型
            auto st = entry.status();
            int mode = 0;
            if(fs::is_directory(st))
                mode = S_IFDIR;
            else if(fs::is_regular_file(st))
                mode = S_IFREG;
#if !defined(__MINGW32__)				
            else if(fs::is_symlink(st))
                mode = S_IFLNK;
#endif				
            cb(name, mode);
        }
    } catch(const fs::filesystem_error &) {
        int len = WideCharToMultiByte(CP_ACP, 0, wfolder.c_str(), -1, nullptr,
                                      0, nullptr, nullptr);
        if(len <= 0)
            spdlog::error("Failed to list directory: {}", u8folder);
        std::string local(len - 1, '\0');
        WideCharToMultiByte(CP_ACP, 0, wfolder.c_str(), -1, local.data(),
                            len - 1, nullptr, nullptr);
        // 目录不存在或无权限，静默返回
        spdlog::error("Failed to list directory: {}", local);
    }

#else
    // ---------------- Linux/macOS 分支 ----------------

    DIR *dirp = opendir(u8folder.c_str());
    if(!dirp)
        return;

    dirent *dp;
    while((dp = readdir(dirp))) {
        std::string name = dp->d_name;
        if(name.empty() || name[0] == '.')
            continue;

        std::string full = u8folder + "/" + name;
        struct stat st{};
        if(stat(full.c_str(), &st) == 0) {
            cb(name, st.st_mode);
        }
    }
    closedir(dirp);
#endif
}

void TVPGetLocalFileListAt(
    const ttstr &name,
    const std::function<void(const ttstr &, tTVPLocalFileInfo *)> &cb) {
    DIR *dirp;
    dirent *direntp;
    tTVP_stat stat_buf;
    std::string folder(name.AsStdString());
    if((dirp = opendir(folder.c_str()))) {
        while((direntp = readdir(dirp)) != nullptr) {
            std::string fullpath = folder + "/" + direntp->d_name;
            if(!TVP_stat(fullpath.c_str(), stat_buf))
                continue;
            ttstr file(direntp->d_name);
            if(file.length() <= 2) {
                if(file == TJS_W(".") || file == TJS_W(".."))
                    continue;
            }
            tjs_char *p = file.Independ();
            while(*p) {
                // make all characters small
                if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
                    *p += TJS_W('a') - TJS_W('A');
                p++;
            }
            tTVPLocalFileInfo info;
            info.NativeName = direntp->d_name;
            info.Mode = stat_buf.st_mode;
            info.Size = stat_buf.st_size;
            info.AccessTime = stat_buf.st_atime;
            info.ModifyTime = stat_buf.st_mtime;
            info.CreationTime = stat_buf.st_ctime;
            cb(file, &info);
        }
        closedir(dirp);
    }
}

//---------------------------------------------------------------------------
void tTVPFileMedia::GetListAt(const ttstr &_name, iTVPStorageLister *lister) {
    ttstr name(_name);
    GetLocalName(name);
    TVPGetLocalFileListAt(name,
                          [lister](const ttstr &name, tTVPLocalFileInfo *s) {
                              if(s->Mode & (S_IFREG)) {
                                  lister->Add(name);
                              }
                          });
}

static int _utf8_strcasecmp(const char *a, const char *b) {
    for(; *a && *b; ++a, ++b) {
        int ca = *a, cb = *b;
        if('A' <= ca && ca <= 'Z')
            ca += 'a' - 'A';
        if('A' <= cb && cb <= 'Z')
            cb += 'a' - 'A';
        int ret = ca - cb;
        if(ret)
            return ret;
    }
    return *a - *b;
}

#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
const std::vector<std::string> &TVPGetApplicationHomeDirectory();
const std::vector<ttstr> &_getPrefixPath() {
    static std::vector<ttstr> ret;
    if(ret.empty()) {
        for(const std::string &path : TVPGetApplicationHomeDirectory()) {
            ret.emplace_back(path);
        }
    }
    return ret;
}
const std::vector<std::string> &_getHomeDir() {
    static std::vector<std::string> ret;
    if(ret.empty()) {
        for(const std::string &path : TVPGetApplicationHomeDirectory()) {
            ret.emplace_back(path + "/");
        }
    }
    return ret;
}
#endif

//---------------------------------------------------------------------------
void tTVPFileMedia::GetLocallyAccessibleName(ttstr &name) {
    ttstr newname;

    const tjs_char *ptr = name.c_str();

#ifdef WIN32
    if(TJS_strncmp(ptr, TJS_W("./"), 2)) {
        // differs from "./",
        // this may be a UNC file name.
        // UNC first two chars must be "\\\\" ?
        // AFAIK 32-bit version of Windows assumes that '/' can be
        // used as a path delimiter. Can UNC "\\\\" be replaced by
        // "//" though ?

        newname = ttstr(TJS_W("\\\\")) + ptr;
    } else {
        ptr += 2; // skip "./"
        if(!*ptr) {
            newname = TJS_W("");
        } else {
            tjs_char dch = *ptr;
            if(*ptr < TJS_W('a') || *ptr > TJS_W('z')) {
                newname = TJS_W("");
            } else {
                ptr++;
                if(*ptr != TJS_W('/')) {
                    newname = TJS_W("");
                } else {
                    newname = ttstr(dch) + TJS_W(":") + ptr;
                }
            }
        }
    }

    // change path delimiter to '\\'
    // tjs_char *pp = newname.Independ();
    // while(*pp) {
    //     if(*pp == TJS_W('/'))
    //         *pp = TJS_W('\\');
    //     pp++;
    // }
#else // posix
    if(!TJS_strncmp(ptr, TJS_W("./"), 2)) {
        ptr += 2; // skip "./"
        newname.Clear();
    }
#if CC_TARGET_PLATFORM == CC_PLATFORM_IOS
    {
        std::string prefix = "/";
        prefix += tTJSNarrowStringHolder(ptr).Buf;
        static const std::vector<ttstr> &prefixPath = _getPrefixPath();
        static const std::vector<std::string> &homeDir = _getHomeDir();
        for(int i = 0; i < prefixPath.size(); ++i) {
            const std::string &dir = homeDir[i];
            if(prefix.length() < dir.length())
                continue;
            std::string actualPrefix = prefix.substr(0, dir.length());
            if(!_utf8_strcasecmp(actualPrefix.c_str(), dir.c_str())) {
                newname = prefixPath[i];
                ptr += prefixPath[i].length();
                while(*ptr && *ptr == TJS_W('/'))
                    ++ptr;
                break;
            }
        }
    }
#endif
    while(*ptr) {
        const tjs_char *ptr_end = ptr;
        while(*ptr_end && *ptr_end != TJS_W('/'))
            ++ptr_end;
        if(ptr_end == ptr)
            break;
        const tjs_char *ptr_cur = ptr;
        tTJSNarrowStringHolder walker(ttstr(ptr, ptr_end - ptr).c_str());
        while(*ptr_end && *ptr_end == TJS_W('/'))
            ++ptr_end;
        ptr = ptr_end;

        DIR *dirp;
        struct dirent *direntp;
        newname += "/";
        if((dirp = opendir(tTJSNarrowStringHolder(newname.c_str())))) {
            bool found = false;
            while((direntp = readdir(dirp)) != nullptr) {
                if(!_utf8_strcasecmp(walker, direntp->d_name)) {
                    newname += direntp->d_name;
                    found = true;
                    break;
                }
            }
            closedir(dirp);
            if(!found) {
                newname += ptr_cur;
                break;
            }
        } else {
            newname += ptr_cur;
            break;
        }
    }

#endif
    name = newname;
}

//---------------------------------------------------------------------------
void tTVPFileMedia::GetLocalName(ttstr &name) {
    ttstr tmp = name;
    GetLocallyAccessibleName(tmp);
    if(tmp.IsEmpty())
        TVPThrowExceptionMessage(TVPCannotGetLocalName, name);
    name = tmp;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
iTVPStorageMedia *TVPCreateFileMedia() { return new tTVPFileMedia; }
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPPreNormalizeStorageName
//---------------------------------------------------------------------------
void TVPPreNormalizeStorageName(ttstr &name) {
    // if the name is an OS's native expression, change it according
    // with the TVP storage system naming rule.
    tjs_int namelen = name.GetLen();
    if(namelen == 0)
        return;
#ifdef WIN32
    if(namelen >= 2) {
        if((name[0] >= TJS_W('a') && name[0] <= TJS_W('z') ||
            name[0] >= TJS_W('A') && name[0] <= TJS_W('Z')) &&
           name[1] == TJS_W(':')) {
            // Windows drive:path expression
            ttstr newname(TJS_W("file://./"));
            newname += name[0];
            newname += (name.c_str() + 2);
            name = newname;
            return;
        }
    }

    if(namelen >= 3) {
        if(name[0] == TJS_W('\\') && name[1] == TJS_W('\\') ||
           name[0] == TJS_W('/') && name[1] == TJS_W('/')) {
            // unc expression
            name = ttstr(TJS_W("file:")) + name;
            return;
        }
    }
#else // posix
    if(namelen >= 1) {
        if(name[0] == TJS_W('/')) {
            name = ttstr(TJS_W("file://.")) + name;
            return;
        }
    }
#endif
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPGetTemporaryName
//---------------------------------------------------------------------------
static tjs_int TVPProcessID;

ttstr TVPGetTemporaryName() {
    static tjs_int TVPTempUniqueNum =
        static_cast<tjs_int>(TVPGetRoughTickCount32());
    tjs_int num = TVPTempUniqueNum++;
    ttstr TVPTempPath = TVPGetAppPath();

    unsigned char buf[16];
    TVPGetRandomBits128(buf);
#if !MY_USE_MINLIB	
    ttstr random{ fmt::format("{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}", buf[0],
                              buf[1], buf[2], buf[3], buf[4], buf[5]) };

    return TVPTempPath + TJS_W("krkr_") + random + TJS_W("_") + ttstr(num) +
        TJS_W("_") + ttstr(TVPProcessID);
#else
	tjs_char random[128];
	swprintf((wchar_t *)random, sizeof(random)/sizeof(tjs_char), L"%02x%02x%02x%02x%02x%02x",
		buf[0], buf[1], buf[2], buf[3],
		buf[4], buf[5]);

    return TVPTempPath + TJS_W("krkr_") + ttstr(random) + TJS_W("_") + ttstr(num) +
        TJS_W("_") + ttstr(TVPProcessID);
#endif
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPRemoveFile
//---------------------------------------------------------------------------
bool TVPRemoveFile(const ttstr &name) {
    tTJSNarrowStringHolder holder(name.c_str());
    return !remove(holder);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPRemoveFolder
//---------------------------------------------------------------------------
bool TVPRemoveFolder(const ttstr &name) {
    tTJSNarrowStringHolder holder(name.c_str());
    return !unlink(holder);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPGetAppPath
//---------------------------------------------------------------------------
ttstr TVPGetAppPath() {
    static ttstr apppath(TVPExtractStoragePath(TVPProjectDir));
    return apppath;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPOpenStream
//---------------------------------------------------------------------------
tTJSBinaryStream *TVPOpenStream(const ttstr &_name, tjs_uint32 flags) {
    // open storage named "name".
    // currently only local/network(by OS) storage systems are
    // supported.
    if(_name.IsEmpty())
        TVPThrowExceptionMessage(TVPCannotOpenStorage, TJS_W("\"\""));

    const ttstr &origname = _name;
    ttstr name(_name);
    TVPGetLocalName(name);

    return new tTVPLocalFileStream(origname, name, flags);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPCheckExistantLocalFile
//---------------------------------------------------------------------------
bool TVPCheckExistentLocalFile(const ttstr &name) {
#if 0
    DWORD attrib = ::GetFileAttributes(name.c_str());
    if(attrib == 0xffffffff || (attrib & FILE_ATTRIBUTE_DIRECTORY))
        return false; // not a file
    else
        return true; // a file
#endif
    tTVP_stat s{};
    if(!TVP_stat(name.c_str(), s)) {
        return false; // not exist
    }
    return s.st_mode & S_IFREG;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPCheckExistantLocalFolder
//---------------------------------------------------------------------------
bool TVPCheckExistentLocalFolder(const ttstr &name) {
    tTVP_stat s{};
    if(!TVP_stat(name.c_str(), s)) {
        return false; // not exist
    }

    return s.st_mode & S_IFDIR;
}
//---------------------------------------------------------------------------

tTVPArchive *TVPOpenZIPArchive(const ttstr &name, tTJSBinaryStream *st,
                               bool normalizeFileName);

tTVPArchive *TVPOpen7ZArchive(const ttstr &name, tTJSBinaryStream *st,
                              bool normalizeFileName);

tTVPArchive *TVPOpenTARArchive(const ttstr &name, tTJSBinaryStream *st,
                               bool normalizeFileName);

static tTVPArchive *(*ArchiveCreators[])(
    const ttstr &name, tTJSBinaryStream *st,
    bool normalizeFileName) = { TVPOpenZIPArchive, 
#if MY_USE_LIB7ZIP
TVPOpen7ZArchive,
#endif
                                TVPOpenTARArchive, tTVPXP3Archive::Create };

//---------------------------------------------------------------------------
// TVPOpenArchive
//---------------------------------------------------------------------------
tTVPArchive *TVPOpenArchive(const ttstr &name, bool normalizeFileName) {
    tTJSBinaryStream *st = TVPCreateStream(name);
    if(!st)
        return nullptr;
    for(auto creator : ArchiveCreators) {
        tTVPArchive *archive = creator(name, st, normalizeFileName);
        if(archive)
            return archive;
        st->SetPosition(0);
    }
    delete st;
    return nullptr;
}

//---------------------------------------------------------------------------
int TVPCheckArchive(const ttstr &localname) {
    tTVPArchive *arc = nullptr;
    int validArchive = 2; // archive but no startup.tjs
    try {
        arc = TVPOpenArchive(TVPNormalizeStorageName(localname), false);
        if(arc) {
            tjs_uint count = arc->GetCount();
            ttstr str_startup_tjs = TJS_W("startup.tjs");
            // ttstr str_sys_init_tjs =
            // TJS_W("system/initialize.tjs");
            for(int i = 0; i < count; ++i) {
                ttstr name = arc->GetName(i);
                if(name.length() == str_startup_tjs.length()) {
                    arc->NormalizeInArchiveStorageName(name);
                    if(name == str_startup_tjs) {
                        validArchive = 1;
                        break;
                    }
                }
                // 				else if (name.length() ==
                // str_sys_init_tjs.length()) {
                // 					arc->NormalizeInArchiveStorageName(name);
                // 					if (name ==
                // str_sys_init_tjs)
                // { 						validArchive = true;
                // break;
                // 					}
                // 				}
            }
        }
    } catch(eTJSError e) {
        spdlog::error("Error opening archive {}", localname.toString());
    }
    if(arc) {
        delete arc;
        return validArchive;
    }
    return 0; // not archive
}

//---------------------------------------------------------------------------
// TVPLocalExtrectFilePath
//---------------------------------------------------------------------------
ttstr TVPLocalExtractFilePath(const ttstr &name) {
    // this extracts given name's path under local filename rule
    const tjs_char *p = name.c_str();
    tjs_int i = name.GetLen() - 1;
    for(; i >= 0; i--) {
        if(p[i] == TJS_W(':') || p[i] == TJS_W('/') || p[i] == TJS_W('\\'))
            break;
    }
    return { p, static_cast<size_t>(i + 1) };
}

//---------------------------------------------------------------------------
// tTVPLocalFileStream
//---------------------------------------------------------------------------
tTVPLocalFileStream::tTVPLocalFileStream(const ttstr &origname,
                                         const ttstr &localname,
                                         tjs_uint32 flag) :
    MemBuffer(nullptr), FileName(localname), Handle(-1) {
    tjs_uint32 access = flag & TJS_BS_ACCESS_MASK;
    if(access == TJS_BS_WRITE) {
        if(TVPCheckExistentLocalFile(localname)) {
        } else {
            ttstr dirpath = TVPLocalExtractFilePath(localname);
            const tjs_char *p = dirpath.c_str();
            tjs_int i = dirpath.GetLen();
            if(p[i - 1] == TJS_W('/') || p[i - 1] == TJS_W('\\'))
                i--;
            dirpath = dirpath.SubString(0, i);
            if(!TVPCheckExistentLocalFolder(dirpath) &&
               !TVPCreateFolders(dirpath)) {
                TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
            }
            //			_lastFileSystemChanged = true;
        }
        MemBuffer = new tTVPMemoryStream();
        return;
    }

    unsigned int rw = 0;
    switch(access) {
        case TJS_BS_READ:
            rw |= O_RDONLY;
            break;
        case TJS_BS_WRITE:
            rw |= O_RDWR | O_CREAT | O_TRUNC;
            break;
        case TJS_BS_APPEND:
            rw |= O_APPEND;
            break;
        case TJS_BS_UPDATE:
            rw |= O_RDWR;
            break;
    }
#ifdef _WIN32
    rw |= O_BINARY;
#endif

#ifdef _WIN32
    std::wstring wpath(reinterpret_cast<const wchar_t *>(localname.c_str()),
                       localname.GetLen());
    Handle = _wopen(wpath.c_str(), rw, 0666);
    if(Handle < 0) {
        if(access == TJS_BS_APPEND || access == TJS_BS_UPDATE) {
            Handle = _wopen(wpath.c_str(), O_RDONLY, 0666);
            if(Handle >= 0) {
                tjs_uint64 size = tTVPLocalFileStream::GetSize();
                if(size < 4 * 1024 * 1024) {
                    MemBuffer = new tTVPMemoryStream();
                    MemBuffer->SetSize(size);
                    read(Handle, MemBuffer->GetInternalBuffer(), size);
                }
                close(Handle);
                Handle = -1;
            }
        }
        if(!MemBuffer)
            TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
    }
#else
    tTJSNarrowStringHolder holder(localname.c_str());
    Handle = open(holder, rw, 0666);
    if(Handle < 0) {
        if(access == TJS_BS_APPEND || access == TJS_BS_UPDATE) {
            // use whole file writing
            Handle = open(holder, O_RDONLY, 0666);
            if(Handle >= 0) {
                tjs_uint64 size = tTVPLocalFileStream::GetSize();
                if(size < 4 * 1024 * 1024) { // only support file size <= 4M
                    MemBuffer = new tTVPMemoryStream();
                    MemBuffer->SetSize(size);
                    read(Handle, MemBuffer->GetInternalBuffer(), size);
                }
                close(Handle);
                Handle = -1;
            }
        }
        if(!MemBuffer)
            TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
    }
#endif
    // push current tick as an environment noise
    uint32_t tick = TVPGetRoughTickCount32();
    TVPPushEnvironNoise(&tick, sizeof(tick));
}

//---------------------------------------------------------------------------
bool TVPWriteDataToFile(const ttstr &filepath, const void *data,
                        unsigned int len);

tTVPLocalFileStream::~tTVPLocalFileStream() {
    if(MemBuffer) {
        if(!TVPWriteDataToFile(FileName, MemBuffer->GetInternalBuffer(),
                               MemBuffer->GetSize())) {
            delete MemBuffer;
            ttstr filename(FileName);
            FileName.~tTJSString();
            free(this);
            TVPThrowExceptionMessage(TJS_W("File Writing Error: %1"), filename);
        }
        delete MemBuffer;
    }
    if(Handle >= 0) {
        close(Handle);
    }

    // push current tick as an environment noise
    // (timing information from file accesses may be good noises)
    uint32_t tick = TVPGetRoughTickCount32();
    TVPPushEnvironNoise(&tick, sizeof(tick));
}

//---------------------------------------------------------------------------
tjs_uint64 tTVPLocalFileStream::Seek(tjs_int64 offset, tjs_int whence) {
    if(MemBuffer) {
        return MemBuffer->Seek(offset, whence);
    }
    return lseek64(Handle, offset, whence);
}

//---------------------------------------------------------------------------
tjs_uint tTVPLocalFileStream::Read(void *buffer, tjs_uint read_size) {
    if(MemBuffer) {
        return MemBuffer->Read(buffer, read_size);
    }
    return read(Handle, buffer, read_size);
}

//---------------------------------------------------------------------------
tjs_uint tTVPLocalFileStream::Write(const void *buffer, tjs_uint write_size) {
    if(MemBuffer) {
        return MemBuffer->Write(buffer, write_size);
    }
    return write(Handle, buffer, write_size);
}

//---------------------------------------------------------------------------
void tTVPLocalFileStream::SetEndOfStorage() {
    if(MemBuffer) {
        return MemBuffer->SetEndOfStorage();
    }
    lseek64(Handle, 0, SEEK_END);
}

//---------------------------------------------------------------------------
tjs_uint64 tTVPLocalFileStream::GetSize() {
    if(MemBuffer) {
        return MemBuffer->GetSize();
    }

    tjs_int64 curpos = lseek64(Handle, 0, SEEK_CUR);
    tjs_uint64 ret = lseek64(Handle, 0, SEEK_END);
    lseek64(Handle, curpos, SEEK_SET);
    return ret;
}
/*
this class provides COM's IStream adapter for tTJSBinaryStream
*/
class tTVPIStreamAdapter : public IStream {
    tTJSBinaryStream *Stream;
    ULONG RefCount;

public:
    tTVPIStreamAdapter(tTJSBinaryStream *ref);

    /*
    the stream passed by argument here is freed by this instance'
    destruction.
    */

    virtual ~tTVPIStreamAdapter();

    // IUnknown
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid,
                                             void **ppvObject) override;

    ULONG STDMETHODCALLTYPE AddRef() override;

    ULONG STDMETHODCALLTYPE Release() override;

    // ISequentialStream
    HRESULT STDMETHODCALLTYPE Read(void *pv, ULONG cb, ULONG *pcbRead) override;

    HRESULT STDMETHODCALLTYPE Write(const void *pv, ULONG cb,
                                    ULONG *pcbWritten) override;

    // IStream
    HRESULT STDMETHODCALLTYPE Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin,
                                   ULARGE_INTEGER *plibNewPosition) override;

    HRESULT STDMETHODCALLTYPE SetSize(ULARGE_INTEGER libNewSize) override;

    HRESULT STDMETHODCALLTYPE CopyTo(IStream *pstm, ULARGE_INTEGER cb,
                                     ULARGE_INTEGER *pcbRead,
                                     ULARGE_INTEGER *pcbWritten) override;

    HRESULT STDMETHODCALLTYPE Commit(DWORD grfCommitFlags) override;

    HRESULT STDMETHODCALLTYPE Revert() override;

    HRESULT STDMETHODCALLTYPE LockRegion(ULARGE_INTEGER libOffset,
                                         ULARGE_INTEGER cb,
                                         DWORD dwLockType) override;

    HRESULT STDMETHODCALLTYPE UnlockRegion(ULARGE_INTEGER libOffset,
                                           ULARGE_INTEGER cb,
                                           DWORD dwLockType) override;

    HRESULT STDMETHODCALLTYPE Stat(STATSTG *pstatstg,
                                   DWORD grfStatFlag) override;

    HRESULT STDMETHODCALLTYPE Clone(IStream **ppstm) override;

    void ClearStream() { Stream = nullptr; }
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPIStreamAdapter
//---------------------------------------------------------------------------
/*
        this class provides adapter for COM's IStream
*/
tTVPIStreamAdapter::tTVPIStreamAdapter(tTJSBinaryStream *ref) {
    Stream = ref;
    RefCount = 1;
}

//---------------------------------------------------------------------------
tTVPIStreamAdapter::~tTVPIStreamAdapter() { delete Stream; }
//---------------------------------------------------------------------------
extern "C" const IID IID_IUnknown;
extern "C" const IID IID_IStream;
extern "C" const IID IID_ISequentialStream;

HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::QueryInterface(REFIID riid,
                                                             void **ppvObject) {
    if(!ppvObject)
        return E_INVALIDARG;

    *ppvObject = nullptr;
    if(!memcmp(&riid, &IID_IUnknown, 16))
        *ppvObject = reinterpret_cast<IUnknown *>(this);
    else if(!memcmp(&riid, &IID_ISequentialStream, 16))
        *ppvObject = reinterpret_cast<ISequentialStream *>(this);
    else if(!memcmp(&riid, &IID_IStream, 16))
        *ppvObject = reinterpret_cast<IStream *>(this);

    if(*ppvObject) {
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter::AddRef() { return ++RefCount; }

//---------------------------------------------------------------------------
ULONG STDMETHODCALLTYPE tTVPIStreamAdapter::Release() {
    if(RefCount == 1) {
        delete this;
        return 0;
    }
    return --RefCount;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Read(void *pv, ULONG cb,
                                                   ULONG *pcbRead) {
    try {
        ULONG read = Stream->Read(pv, cb);
        if(pcbRead)
            *pcbRead = read;
    } catch(...) {
        return E_FAIL;
    }
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Write(const void *pv, ULONG cb,
                                                    ULONG *pcbWritten) {
    try {
        ULONG written = Stream->Write(pv, cb);
        if(pcbWritten)
            *pcbWritten = written;
    } catch(...) {
        return E_FAIL;
    }
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Seek(
    LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition) {
    try {
        switch(dwOrigin) {
            case STREAM_SEEK_SET:
                if(plibNewPosition)
                    (*plibNewPosition).QuadPart =
                        Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
                else
                    Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_SET);
                break;
            case STREAM_SEEK_CUR:
                if(plibNewPosition)
                    (*plibNewPosition).QuadPart =
                        Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
                else
                    Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_CUR);
                break;
            case STREAM_SEEK_END:
                if(plibNewPosition)
                    (*plibNewPosition).QuadPart =
                        Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
                else
                    Stream->Seek(dlibMove.QuadPart, TJS_BS_SEEK_END);
                break;
            default:
                return E_FAIL;
        }
    } catch(...) {
        return E_FAIL;
    }
    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
tTVPIStreamAdapter::SetSize(ULARGE_INTEGER libNewSize) {
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::CopyTo(
    IStream *pstm, ULARGE_INTEGER cb, ULARGE_INTEGER *pcbRead,
    ULARGE_INTEGER *pcbWritten) {
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Commit(DWORD grfCommitFlags) {
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Revert() { return E_NOTIMPL; }

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::LockRegion(
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::UnlockRegion(
    ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Stat(STATSTG *pstatstg,
                                                   DWORD grfStatFlag) {
    // This method imcompletely fills the target structure, because
    // some informations like access mode or stream name are already
    // lost at this point.

    if(pstatstg) {
        memset(pstatstg, 0, sizeof(*pstatstg));

        // pwcsName
        // this object's storage pointer does not have a name ...
        if(!(grfStatFlag & STATFLAG_NONAME)) {
            // anyway returns an empty string
            pstatstg->pwcsName = (LPOLESTR)TJS_W("");
        }

        // type
        pstatstg->type = STGTY_STREAM;

        // cbSize
        pstatstg->cbSize.QuadPart = Stream->GetSize();

        // mtime, ctime, atime unknown

        // grfMode unknown
        pstatstg->grfMode =
            STGM_DIRECT | STGM_READWRITE | STGM_SHARE_DENY_WRITE;
        // Note that this method always returns flags above,
        // regardless of the actual mode. In the return value, the
        // stream is to be indicated that the stream can be written,
        // but of cource, the Write method will fail if the stream is
        // read-only.

        // grfLockSuppoted
        pstatstg->grfLocksSupported = 0;

        // grfStatBits unknown
    } else {
        return E_INVALIDARG;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE tTVPIStreamAdapter::Clone(IStream **ppstm) {
    return E_NOTIMPL;
}
//---------------------------------------------------------------------------

IStream *TVPCreateIStream(tTJSBinaryStream *s) {
    return new tTVPIStreamAdapter(s);
}

//---------------------------------------------------------------------------
// IStream creator
//---------------------------------------------------------------------------
IStream *TVPCreateIStream(const ttstr &name, tjs_uint32 flags) {
    // convert tTJSBinaryStream to IStream thru TStream

    tTJSBinaryStream *stream0 = nullptr;
    try {
        stream0 = TVPCreateStream(name, flags);
    } catch(...) {
        delete stream0;
        return nullptr;
    }

    IStream *istream = new tTVPIStreamAdapter(stream0);

    return istream;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPBinaryStreamAdapter
//---------------------------------------------------------------------------
tTVPBinaryStreamAdapter::tTVPBinaryStreamAdapter(IStream *ref) {
    Stream = ref;
    Stream->AddRef();
}

//---------------------------------------------------------------------------
tTVPBinaryStreamAdapter::~tTVPBinaryStreamAdapter() { Stream->Release(); }

//---------------------------------------------------------------------------
tjs_uint64 tTVPBinaryStreamAdapter::Seek(tjs_int64 offset, tjs_int whence) {
    DWORD origin;

    switch(whence) {
        case TJS_BS_SEEK_SET:
            origin = STREAM_SEEK_SET;
            break;
        case TJS_BS_SEEK_CUR:
            origin = STREAM_SEEK_CUR;
            break;
        case TJS_BS_SEEK_END:
            origin = STREAM_SEEK_END;
            break;
        default:
            origin = STREAM_SEEK_SET;
            break;
    }

    LARGE_INTEGER ofs;
    ULARGE_INTEGER newpos;

    ofs.QuadPart = 0;
    HRESULT hr = Stream->Seek(ofs, STREAM_SEEK_CUR, &newpos);
    bool orgpossaved;
    LARGE_INTEGER orgpos;
    if(FAILED(hr)) {
        orgpossaved = false;
    } else {
        orgpossaved = true;
        *(LARGE_INTEGER *)&orgpos = *(LARGE_INTEGER *)&newpos;
    }

    ofs.QuadPart = offset;

    hr = Stream->Seek(ofs, origin, &newpos);
    if(FAILED(hr)) {
        if(orgpossaved) {
            Stream->Seek(orgpos, STREAM_SEEK_SET, &newpos);
        } else {
            TVPThrowExceptionMessage(TVPSeekError);
        }
    }

    return newpos.QuadPart;
}

//---------------------------------------------------------------------------
tjs_uint tTVPBinaryStreamAdapter::Read(void *buffer, tjs_uint read_size) {
    ULONG cb = read_size;
    ULONG read;
    HRESULT hr = Stream->Read(buffer, cb, &read);
    if(FAILED(hr))
        read = 0;
    return read;
}

//---------------------------------------------------------------------------
tjs_uint tTVPBinaryStreamAdapter::Write(const void *buffer,
                                        tjs_uint write_size) {
    ULONG cb = write_size;
    ULONG written;
    HRESULT hr = Stream->Write(buffer, cb, &written);
    if(FAILED(hr))
        written = 0;
    return written;
}

//---------------------------------------------------------------------------
tjs_uint64 tTVPBinaryStreamAdapter::GetSize() {
    HRESULT hr;
    STATSTG stg;

    hr = Stream->Stat(&stg, STATFLAG_NONAME);
    if(FAILED(hr)) {
        return inherited::GetSize(); // use default routine
    }

    return stg.cbSize.QuadPart;
}

//---------------------------------------------------------------------------
void tTVPBinaryStreamAdapter::SetEndOfStorage() {
    ULARGE_INTEGER pos;
    pos.QuadPart = GetPosition();
    HRESULT hr = Stream->SetSize(pos);
    if(FAILED(hr))
        TVPThrowExceptionMessage(TVPTruncateError);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPCreateBinaryStreamAdapter
//---------------------------------------------------------------------------
tTJSBinaryStream *TVPCreateBinaryStreamAdapter(IStream *refstream) {
    return new tTVPBinaryStreamAdapter(refstream);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTVPPluginHolder
//---------------------------------------------------------------------------
tTVPPluginHolder::tTVPPluginHolder(const ttstr &aname) {
    LocalTempStorageHolder = nullptr;

    // search in TVP storage system
    ttstr place(TVPGetPlacedPath(aname));
    if(!place.IsEmpty()) {
        LocalTempStorageHolder = new tTVPLocalTempStorageHolder(place);
    } else {
        // not found in TVP storage system; search exepath,
        // exepath\system, exepath\plugin
#if 0
        ttstr exepath =
            IncludeTrailingBackslash(ExtractFileDir(ExePath()));
        ttstr pname = exepath + aname;
        if(TVPCheckExistentLocalFile(pname))
        {
            LocalName = pname;
            return;
        }

        pname = exepath + TJS_W("system\\") + aname;
        if(TVPCheckExistentLocalFile(pname))
        {
            LocalName = pname;
            return;
        }

#ifdef TJS_64BIT_OS
        pname = exepath + TJS_W("plugin64\\") + aname;
#else
        pname = exepath + TJS_W("plugin\\") + aname;
#endif
        if(TVPCheckExistentLocalFile(pname))
        {
            LocalName = pname;
            return;
        }
#endif
    }
}

//---------------------------------------------------------------------------
tTVPPluginHolder::~tTVPPluginHolder() {
    if(LocalTempStorageHolder) {
        delete LocalTempStorageHolder;
    }
}

//---------------------------------------------------------------------------
const ttstr &tTVPPluginHolder::GetLocalName() const {
    if(LocalTempStorageHolder)
        return LocalTempStorageHolder->GetLocalName();
    return LocalName;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPSearchCD
//---------------------------------------------------------------------------
ttstr TVPSearchCD(const ttstr &name) {
    // search CD which has specified volume label name.
    // return drive letter ( such as 'A' or 'B' )
    // return empty string if not found.
#if 0
    std::wstring narrow_name = name.AsStdString();

    wchar_t dr[4];
    for(dr[0]=L'A',dr[1]=L':',dr[2]=L'\\',dr[3]=0;dr[0]<=L'Z';dr[0]++)
    {
        if(::GetDriveType(dr) == DRIVE_CDROM)
        {
            wchar_t vlabel[256];
            wchar_t fs[256];
            DWORD mcl = 0,sfs = 0;
            GetVolumeInformation(dr, vlabel, 255, nullptr, &mcl, &sfs, fs, 255);
            if( icomp(std::wstring(vlabel),narrow_name) )
            //if(std::string(vlabel).AnsiCompareIC(narrow_name)==0)
                return ttstr((tjs_char)dr[0]);
        }
    }
#endif
    return {};
}
//---------------------------------------------------------------------------

tTJSBinaryStream *TVPGetCachedArchiveHandle(void *pointer, const ttstr &name);

void TVPReleaseCachedArchiveHandle(void *pointer, tTJSBinaryStream *stream);

TArchiveStream::TArchiveStream(tTVPArchive *owner, tjs_uint64 off,
                               tjs_uint64 len) :
    Owner(owner), StartPos(off), DataLength(len) {
    Owner->AddRef();
    _instr = TVPGetCachedArchiveHandle(Owner, Owner->ArchiveName);
    CurrentPos = 0;
    _instr->SetPosition(off);
}

TArchiveStream::~TArchiveStream() {
    TVPReleaseCachedArchiveHandle(Owner, _instr);
    Owner->Release();
}

//---------------------------------------------------------------------------
// TVPCreateNativeClass_Storages
//---------------------------------------------------------------------------
tTJSNativeClass *TVPCreateNativeClass_Storages() {
    auto *cls = new tTJSNC_Storages();

    // setup some platform-specific members
    //----------------------------------------------------------------------

    //-- methods

    //----------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ searchCD) {
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        if(result)
            *result = TVPSearchCD(*param[0]);

        return TJS_S_OK;
    }
    TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(
        /*object to register*/ cls,
        /*func. name*/ searchCD)
    //----------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ getLocalName) {
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        if(result) {
            ttstr str(TVPNormalizeStorageName(*param[0]));
            TVPGetLocalName(str);
            *result = str;
        }

        return TJS_S_OK;
    }
    TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(
        /*object to register*/ cls,
        /*func. name*/ getLocalName)
    //----------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(/*func. name*/ selectFile) {
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        iTJSDispatch2 *dsp = param[0]->AsObjectNoAddRef();

        bool res = TVPSelectFile(dsp);

        if(result)
            *result = static_cast<tjs_int>(res);

        return TJS_S_OK;
    }
    TJS_END_NATIVE_STATIC_METHOD_DECL_OUTER(
        /*object to register*/ cls,
        /*func. name*/ selectFile)
    //----------------------------------------------------------------------

    return cls;
}
//---------------------------------------------------------------------------

static FILE *_fileopen(ttstr path) {
    std::string strpath = path.AsStdString();
    FILE *fp = fopen(strpath.c_str(), "wb");
    if(!fp) { // make dirs
        path = TVPExtractStoragePath(path);
        TVPCreateFolders(path);
        fp = fopen(strpath.c_str(), "wb");
    }
    return fp;
}

bool TVPSaveStreamToFile(tTJSBinaryStream *st, tjs_uint64 offset,
                         tjs_uint64 size, const ttstr &outpath) {
    FILE *fp = _fileopen(outpath);
    if(!fp)
        return false;
    tjs_uint64 origpos = st->GetPosition();
    st->SetPosition(offset);
    std::vector<char> buffer;
    buffer.resize(2 * 1024 * 1024);
    while(size > 0) {
        unsigned int readsize = size > buffer.size() ? buffer.size() : size;
        readsize = st->Read(&buffer.front(), readsize);
        fwrite(&buffer.front(), 1, readsize, fp);
        size -= readsize;
    }
    fclose(fp);
    st->SetPosition(origpos);
    return true;
}
