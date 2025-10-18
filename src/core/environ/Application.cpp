#include "tjsCommHead.h"

#include <algorithm>
#include <string>
#include <vector>
#include <assert.h>

#include "tjsError.h"
#include "tjsDebug.h"

#include "Application.h"
#include "SysInitIntf.h"
#include "SysInitImpl.h"
#include "DebugIntf.h"
#include "MsgIntf.h"
#include "ScriptMgnIntf.h"
#include "tjsError.h"
#include "PluginImpl.h"
#include "SystemIntf.h"

#include "Exception.h"
// #include "Resource.h"
#include "SystemControl.h"
// #include "MouseCursor.h"
#include "SystemImpl.h"
#include "WaveImpl.h"
#include "GraphicsLoadThread.h"
#include "Platform.h"
#include "EventIntf.h"
#include <thread>
#include "ConfigManager/LocaleConfigManager.h"
#include "StorageIntf.h"
#if MY_USE_FFMPEG
extern "C" {
#include <libavutil/avstring.h>
}
#else
#include <libgen.h> //dirname
#endif
#include "TVPColor.h"
#include "FontImpl.h"

#if defined(__MINGW32__)
#undef GetMessage
#endif

tTVPApplication *Application = new tTVPApplication;
std::thread::id TVPMainThreadID;
static tTJSCriticalSection _NoMemCallBackCS;
static void *_reservedMem = malloc(1024 * 1024 * 4); // 4M reserved mem
static bool _project_startup = false;

static void _do_compact() { TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX); }

static void _no_memory_cb() {
    tTJSCSH lock(_NoMemCallBackCS);
    free(_reservedMem);
    if(TVPMainThreadID == std::this_thread::get_id()) {
        _do_compact();
    } else {
        Application->PostUserMessage(_do_compact);
    }
    _reservedMem = realloc(nullptr, 1024 * 1024 * 4);
}

static std::string _title, _msg, _retry, _cancel;
static tTJSCriticalSection _cs;
typedef void *F_alloc_t(void *, size_t);
static void *__do_alloc_func(F_alloc_t *f, void *p, size_t c) {
    void *ptr = f(p, c);

    if(!ptr) {
        _no_memory_cb();
        ptr = f(p, c);
        if(!ptr) {
            tTJSCSH lock(_cs);
            const char *btns[2] = { _retry.c_str(), _cancel.c_str() };
            while(!ptr &&
                  TVPShowSimpleMessageBox(_msg.c_str(), _title.c_str(), 2,
                                          btns) == 0) {
                ptr = f(p, c);
            }
            // TVPExitApplication(-1);
        }
    }
    return ptr;
}

ttstr TVPGetErrorDialogTitle() {
    const ttstr &title = Application->GetTitle();
    if(title.IsEmpty()) {
        return TVPGetPackageVersionString() + " Error";
    } else {
        return ttstr(TVPGetPackageVersionString()) + " " + title;
    }
}

ttstr ExePath() { return TVPNativeProjectDir; }

bool TVPCheckAbout();
bool TVPCheckPrintDataPath();
void TVPOnError();
void TVPLockSoundMixer();
void TVPUnlockSoundMixer();

static bool _warnLowMem = true;
void TVPCheckMemory() {
#if defined(_DEBUG)
    if(_warnLowMem) {
        tjs_int freeMem = TVPGetSystemFreeMemory();
        if(freeMem < 24) {
            char buf[256];
            sprintf(buf,
                    "Insufficient memory (%dMB available)\nYou can "
                    "diable this "
                    "notice in global preference.",
                    freeMem);
            const char *btn = "OK";
            TVPShowSimpleMessageBox(buf, "No Memory Warning", 1, &btn);
            _warnLowMem = false;
        }
    }
#endif
}

int TVPShowSimpleMessageBox(const ttstr &text, const ttstr &caption) {
    std::vector<ttstr> normal;
    normal.emplace_back(
        LocaleConfigManager::GetInstance()->GetText("msgbox_ok"));
    return TVPShowSimpleMessageBox(text, caption, normal);
}

int TVPShowSimpleMessageBoxYesNo(const ttstr &text, const ttstr &caption) {
    std::vector<ttstr> normal;
    LocaleConfigManager *mgr = LocaleConfigManager::GetInstance();
    normal.emplace_back(mgr->GetText("msgbox_yes"));
    normal.emplace_back(mgr->GetText("msgbox_no"));
    return TVPShowSimpleMessageBox(text, caption, normal);
}

ttstr TVPGetMessageByLocale(const std::string &key) {
    return LocaleConfigManager::GetInstance()->GetText(key);
}

int _argc;
char **_argv;
#if 0
extern void TVPInitCompatibleNativeFunctions();
extern void TVPLoadMessage();
AcceleratorKeyTable::AcceleratorKeyTable() {
	// デフォルトを読み込む
	hAccel_ = ::LoadAccelerators( (HINSTANCE)GetModuleHandle(0), MAKEINTRESOURCE(IDC_TVPWIN32));
}
AcceleratorKeyTable::~AcceleratorKeyTable() {
	std::map<HWND,AcceleratorKey*>::iterator i = keys_.begin();
	for( ; i != keys_.end(); i++ ) {
		delete (i->second);
	}
}
void AcceleratorKeyTable::AddKey( HWND hWnd, WORD id, WORD key, BYTE virt ) {
	std::map<HWND,AcceleratorKey*>::iterator i = keys_.find(hWnd);
	if( i != keys_.end() ) {
		i->second->AddKey(id,key,virt);
	} else {
		AcceleratorKey* acc = new AcceleratorKey();
		acc->AddKey( id, key, virt );
		keys_.insert( std::map<HWND, AcceleratorKey*>::value_type( hWnd, acc ) );
	}
}
void AcceleratorKeyTable::DelKey( HWND hWnd, WORD id ) {
	std::map<HWND,AcceleratorKey*>::iterator i = keys_.find(hWnd);
	if( i != keys_.end() ) {
		i->second->DelKey(id);
	}
}

void AcceleratorKeyTable::DelTable( HWND hWnd ) {
	std::map<HWND,AcceleratorKey*>::iterator i = keys_.find(hWnd);
	if( i != keys_.end() ) {
		delete (i->second);
		keys_.erase(i);
	}
}
AcceleratorKey::AcceleratorKey() : hAccel_(nullptr), keys_(nullptr), key_count_(0) {
}
AcceleratorKey::~AcceleratorKey() {
	if( hAccel_ != nullptr ) ::DestroyAcceleratorTable( hAccel_ );
	delete[] keys_;
}
void AcceleratorKey::AddKey( WORD id, WORD key, BYTE virt ) {
	// まずは存在するかチェックする
	bool found = false;
	int index = 0;
	for( int i = 0; i < key_count_; i++ ) {
		if( keys_[i].cmd == id ) {
			index = i;
			found = true;
			break;
		}
	}
	if( found ) {
		// 既に登録されているコマンドなのでキー情報の更新を行う
		if( keys_[index].key == key && keys_[index].fVirt == virt ) {
			// 変更されていない
			return;
		}
		keys_[index].key = key;
		keys_[index].fVirt = virt;
		HACCEL hAccel = ::CreateAcceleratorTable( keys_, key_count_ );
		if( hAccel_ != nullptr ) ::DestroyAcceleratorTable( hAccel_ );
		hAccel_ = hAccel;
	} else {
		ACCEL* table = new ACCEL[key_count_+1];
		for( int i = 0; i < key_count_; i++ ) {
			table[i] = keys_[i];
		}
		table[key_count_].cmd = id;
		table[key_count_].key = key;
		table[key_count_].fVirt = virt;
		key_count_++;
		HACCEL hAccel = ::CreateAcceleratorTable( table, key_count_ );
		if( hAccel_ != nullptr ) ::DestroyAcceleratorTable( hAccel_ );
		hAccel_ = hAccel;
		delete[] keys_;
		keys_ = table;
	}

}
void AcceleratorKey::DelKey( WORD id ) {
	// まずは存在するかチェックする
	bool found = false;
	for( int i = 0; i < key_count_; i++ ) {
		if( keys_[i].cmd == id ) {
			found = true;
			break;
		}
	}
	if( found == false ) return;

	// 存在した場合作り直し
	ACCEL* table = new ACCEL[key_count_-1];
	int dest = 0;
	for( int i = 0; i < key_count_; i++ ) {
		if( keys_[i].cmd != id ) {
			table[dest] = keys_[i];
			dest++;
		}
	}
	key_count_--;
	HACCEL hAccel = ::CreateAcceleratorTable( table, key_count_ );
	if( hAccel_ != nullptr ) ::DestroyAcceleratorTable( hAccel_ );
	hAccel_ = hAccel;
	delete[] keys_;
	keys_ = table;
}

int APIENTRY WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nCmdShow ) {
	try {
		CheckMemoryLeaksStart();
		// ウォッチで _crtBreakAlloc にセットする

		// XP より後で使えるAPIを動的に読み込んで互換性を取る
		TVPInitCompatibleNativeFunctions();

		// メッセージ文字列をリソースから読込み
		TVPLoadMessage();

		_argc = __argc;
		_argv = __argv;

		MouseCursor::Initialize();
		Application = new tTVPApplication();
		Application->StartApplication( __argc, __argv );
	
		// delete application and exit forcely
		// this prevents ugly exception message on exit
		// アプリケーションを削除し強制終了させる。
		// これは終了時の醜い例外メッセージを抑止する
		delete Application;

#ifndef _DEBUG
//		::ExitProcess(TVPTerminateCode); // ここで終了させるとメモリリーク表示が行われない
#endif
	} catch (...) {
		return 2;
	}
	return TVPTerminateCode;
}
#endif
tTVPApplication::tTVPApplication() :
    is_attach_console_(false), tarminate_(false), application_activating_(true),
    image_load_thread_(nullptr), has_map_report_process_(false) {}
tTVPApplication::~tTVPApplication() {
    // 	while( windows_list_.size() ) {
    // 		std::vector<TTVPWindowForm*>::iterator i =
    // windows_list_.begin(); 		delete (*i);
    // 		// TTVPWindowForm
    // のデストラクタ内でリストから削除されるはず
    // 	}
    // 	windows_list_.clear();
    delete image_load_thread_;
}

bool tTVPApplication::StartApplication(ttstr path) {
    //	_set_se_translator(se_translator_function);

    ArgC = 0;
    ArgV = nullptr;
    TVPTerminateCode = 0;
    LocaleConfigManager *mgr = LocaleConfigManager::GetInstance();
    _retry = mgr->GetText("retry");
    _cancel = mgr->GetText("cancel");
    _msg = mgr->GetText("err_no_memory");
    _title = mgr->GetText("err_occured");
    TVPNativeProjectDir = path;

    CheckConsole();

    // try starting the program!
    try {
        //		if(TVPCheckProcessLog()) return true; // sub-process
        // for
        // processing object hash map log

        TVPProjectDir = TVPNormalizeStorageName(path);

        TVPInitScriptEngine();
        TVPInitFontNames();

        // banner
        TVPAddImportantLog(TVPFormatMessage(TVPProgramStartedOn, TVPGetOSName(),
                                            TVPGetPlatformName()));

        // TVPInitializeBaseSystems
        TVPInitializeBaseSystems();

        Initialize();

        if(TVPCheckPrintDataPath())
            return true;
        if(TVPExecuteUserConfig())
            return true;

        image_load_thread_ = new tTVPAsyncImageLoader();

        tvpLoadPlugins(); // load plugin module *.tpm
        TVPSystemInit();

        if(TVPCheckAbout())
            return true; // version information dialog box;

        SetTitle(TVPKirikiri.operator const tjs_char *());
        TVPSystemControl = new tTVPSystemControl();
        // Check digitizer
        CheckDigitizer();

        // start image load thread
        image_load_thread_->Resume();

        TVPInitializeStartupScript();
        _project_startup = true;
    } catch(const EAbort &) {
        // nothing to do
    } catch(const Exception &exception) {
        TVPOnError();
        if(!TVPSystemUninitCalled)
            ShowException(exception.what());
    } catch(const TJS::eTJSScriptError &e) {
        TVPOnError();
        if(!TVPSystemUninitCalled) {
            ttstr msg;
            if(!title_.IsEmpty()) {
                msg += title_;
                msg += "\n";
            }
            msg += e.GetMessage();
            const tjs_char *pszBlockName = e.GetBlockName();
            if(pszBlockName && *pszBlockName) {
                msg += TJS_W("\n@line(");
                tjs_char tmp[34];
                msg += TJS_int_to_str(e.GetSourceLine(), tmp);
                msg += TJS_W(") ");
                msg += pszBlockName;
            }
            msg += TJS_W("\n");
            msg += e.GetTrace();
            ShowException(msg);
        }
    } catch(const TJS::eTJS &e) {
        TVPOnError();
        if(!TVPSystemUninitCalled)
            ShowException(e.GetMessage());
    } catch(const std::exception &e) {
        ShowException(e.what());
    } catch(const char *e) {
        ShowException(e);
    } catch(const tjs_char *e) {
        ShowException(e);
    } catch(...) {
        ShowException((const tjs_char *)TVPUnknownError);
    }

    return false;
}
/**
 * コンソールからの起動か確認し、コンソールからの起動の場合は、標準出力を割り当てる
 */
void tTVPApplication::CheckConsole() {
#ifdef TVP_LOG_TO_COMMANDLINE_CONSOLE
    if(has_map_report_process_)
        return; // 書き出し用子プロセスして起動されていた時はコンソール接続しない
    HANDLE hin = ::GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hout = ::GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE herr = ::GetStdHandle(STD_ERROR_HANDLE);

    DWORD curProcId = ::GetCurrentProcessId();
    DWORD processList[256];
    DWORD count = ::GetConsoleProcessList(processList, 256);
    bool thisProcHasConsole = false;
    for(DWORD i = 0; i < count; i++) {
        if(processList[i] == curProcId) {
            thisProcHasConsole = true;
            break;
        }
    }
    bool attachedConsole = true;
    if(thisProcHasConsole == false) {
        attachedConsole = ::AttachConsole(ATTACH_PARENT_PROCESS) != 0;
    }

    if((hin == 0 || hout == 0 || herr == 0) && attachedConsole) {
        wchar_t console[256];
        ::GetConsoleTitle(console, 256);
        console_title_ = std::wstring(console);
        // 元のハンドルを再割り当て
        if(hin)
            ::SetStdHandle(STD_INPUT_HANDLE, hin);
        if(hout)
            ::SetStdHandle(STD_OUTPUT_HANDLE, hout);
        if(herr)
            ::SetStdHandle(STD_ERROR_HANDLE, herr);
    }
    is_attach_console_ = attachedConsole;
#endif
}

void tTVPApplication::CloseConsole() {
#if 0
	wchar_t buf[100];
	DWORD len = TJS_snprintf(buf, 100, TVPExitCode, TVPTerminateCode);
	PrintConsole(buf, len);
	if( is_attach_console_ ) {
		::SetConsoleTitle( console_title_.c_str() );
		::FreeConsole();
		is_attach_console_ = false;
	}
#endif
}
void TVPConsoleLog(const ttstr &mes, bool important);
void tTVPApplication::PrintConsole(const ttstr &mes, bool important) {
    TVPConsoleLog(mes, important);
}
#if 0
HWND tTVPApplication::GetHandle() {
	if( windows_list_.size() > 0 ) {
		return windows_list_[0]->GetHandle();
	} else {
		return INVALID_HANDLE_VALUE;
	}
}
void tTVPApplication::Minimize() {
	size_t size = windows_list_.size();
	for( size_t i = 0; i < size; i++ ) {
		if( windows_list_[i]->GetVisible() ) {
			::ShowWindow( windows_list_[i]->GetHandle(), SW_MINIMIZE );
		}
	}
}
void tTVPApplication::Restore() {
	size_t size = windows_list_.size();
	for( size_t i = 0; i < size; i++ ) {
		if( windows_list_[i]->GetVisible() ) {
			::ShowWindow( windows_list_[i]->GetHandle(), SW_RESTORE );
		}
	}
}
void tTVPApplication::BringToFront() {
	size_t size = windows_list_.size();
	for( size_t i = 0; i < size; i++ ) {
		windows_list_[i]->BringToFront();
	}
}
#endif
void tTVPApplication::ShowException(const ttstr &e) {
    TVPShowSimpleMessageBox(e, TVPGetErrorDialogTitle());
    TVPSystemUninit();
    TVPExitApplication(0);
}
void tTVPApplication::Run() {
    try {
        if(TVPTerminated) {
            TVPSystemUninit();
            TVPExitApplication(TVPTerminateCode);
        }
        //	TVPBreathe();
        ProcessMessages();
        if(TVPSystemControl)
            TVPSystemControl->SystemWatchTimerTimer();
        //		TVPDeliverWindowUpdateEvents(); // from
        // SystemWatchTimerTimer
    } catch(const EAbort &) {
        // nothing to do
#if !(defined(_MSC_VER) && defined(_DEBUG))
    } catch(const Exception &exception) {
        TVPOnError();
        if(!TVPSystemUninitCalled)
            ShowException(exception.what());
    } catch(const TJS::eTJSScriptError &e) {
        TVPOnError();
        if(!TVPSystemUninitCalled) {
            ttstr msg;
            if(!title_.IsEmpty()) {
                msg += title_;
                msg += "\n";
            }
            msg += e.GetMessage();
            const tjs_char *pszBlockName = e.GetBlockName();
            if(pszBlockName && *pszBlockName) {
                msg += TJS_W("\n@line(");
                tjs_char tmp[34];
                msg += TJS_int_to_str(e.GetSourceLine(), tmp);
                msg += TJS_W(") ");
                msg += pszBlockName;
            }
            msg += TJS_W("\n");
            msg += e.GetTrace();
            ShowException(msg);
        }
    } catch(const TJS::eTJS &e) {
        TVPOnError();
        if(!TVPSystemUninitCalled)
            ShowException(e.GetMessage());
    } catch(const std::exception &e) {
        ShowException(e.what());
    } catch(const char *e) {
        ShowException(e);
    } catch(const tjs_char *e) {
        ShowException(e);
    } catch(...) {
        ShowException((const tjs_char *)TVPUnknownError);
#endif
    }
}

void tTVPApplication::ProcessMessages() {
    std::vector<std::tuple<void *, int, tMsg>> lstUserMsg;
    {
        std::lock_guard<std::mutex> cs(m_msgQueueLock);
        m_lstUserMsg.swap(lstUserMsg);
    }
    for(std::tuple<void *, int, tMsg> &it : lstUserMsg) {
        std::get<2>(it)();
    }
    TVPTimer::ProgressAllTimer();
}

#if 0
bool tTVPApplication::ProcessMessage( MSG &msg ) {
	bool result = false;
	if( ::PeekMessage( &msg, nullptr, 0, 0, PM_NOREMOVE) ) {
		BOOL msgExists = ::PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE);
		if( msgExists == 0 ) {
			return result;
		}
		result = true;
		if( msg.message != WM_QUIT ) {
			HACCEL hAccelTable = accel_key_.GetHandle(msg.hwnd);
			if( !TranslateAccelerator(msg.hwnd, hAccelTable, &msg) ) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		} else {
			TVPTerminateCode = (int)msg.wParam;
			tarminate_ = true;
		}
	}
	return result;
}
void tTVPApplication::ProcessMessages() {
	MSG msg = {0};
	while(ProcessMessage(msg));
}
void tTVPApplication::HandleMessage() {
	MSG msg = {0};
	if( !ProcessMessage(msg) ) {
		HandleIdle(msg);
	}
}
void tTVPApplication::HandleIdle(MSG &) {
	bool done = true;
	if( TVPSystemControl ) {
		done = TVPSystemControl->ApplicationIdle();
	}
	if( done ) ::WaitMessage();
}
#endif
void tTVPApplication::SetTitle(const ttstr &caption) {
    title_ = caption;
#if 0
	if( windows_list_.size() > 0 ) {
		windows_list_[0]->SetCaption( caption );
	}
	if( is_attach_console_ ) {
		::SetConsoleTitle( caption.c_str() );
	}
#endif
}

void tTVPApplication::Terminate() {
    //::PostQuitMessage(0);
    tarminate_ = true;
    TVPTerminated = true;
}
#if 0
HWND tTVPApplication::GetMainWindowHandle() const {
	if( windows_list_.size() > 0 ) {
		return windows_list_[0]->GetHandle();
	}
	return INVALID_HANDLE_VALUE;
}

void tTVPApplication::RemoveWindow( TTVPWindowForm* win ) {
	std::vector<class TTVPWindowForm*>::iterator it = std::remove( windows_list_.begin(), windows_list_.end(), win );
	if( it != windows_list_.end() ) {
		windows_list_.erase( it, windows_list_.end() );
	}
}
void tTVPApplication::PostMessageToMainWindow(UINT message, WPARAM wParam, LPARAM lParam) {
	if( windows_list_.size() > 0 ) {
		::PostMessage( windows_list_[0]->GetHandle(), message, wParam, lParam );
	}
}
void tTVPApplication::GetDisableWindowList( std::vector<class TTVPWindowForm*>& win ) {
	size_t count = windows_list_.size();
	for( size_t i = 0; i < count; i++ ) {
		if( windows_list_[i]->GetEnable() == false ) {
			win.push_back( windows_list_[i] );
		}
	}
}
void tTVPApplication::GetEnableWindowList( std::vector<class TTVPWindowForm*>& win, class TTVPWindowForm* activeWindow ) {
	size_t count = windows_list_.size();
	for( size_t i = 0; i < count; i++ ) {
		if( activeWindow != windows_list_[i] && windows_list_[i]->GetEnable() ) {
			win.push_back( windows_list_[i] );
		}
	}
}

void tTVPApplication::DisableWindows() {
	size_t count = windows_list_.size();
	for( size_t i = 0; i < count; i++ ) {
		windows_list_[i]->SetEnable( false );
	}
}
void tTVPApplication::EnableWindows( const std::vector<TTVPWindowForm*>& win ) {
	size_t count = win.size();
	for( size_t i = 0; i < count; i++ ) {
		win[i]->SetEnable( true );
	}
	/*
	size_t count = windows_list_.size();
	for( size_t i = 0; i < count; i++ ) {
		TTVPWindowForm* win = windows_list_[i];
		std::vector<TTVPWindowForm*>::const_iterator f = std::find( ignores.begin(), ignores.end(), win );
		if( f == ignores.end() ) {
			windows_list_[i]->SetEnable( true );
		}
	}
	*/
}
void tTVPApplication::FreeDirectInputDeviceForWindows() {
	size_t count = windows_list_.size();
	for( size_t i = 0; i < count; i++ ) {
		windows_list_[i]->FreeDirectInputDevice();
	}
}

void tTVPApplication::RegisterAcceleratorKey(HWND hWnd, char virt, short key, short cmd) {
	accel_key_.AddKey( hWnd, cmd, key, virt );
}
void tTVPApplication::UnregisterAcceleratorKey(HWND hWnd, short cmd) {
	accel_key_.DelKey( hWnd, cmd );
}
void tTVPApplication::DeleteAcceleratorKeyTable( HWND hWnd ) {
	accel_key_.DelTable( hWnd );
}
#endif
void tTVPApplication::CheckDigitizer() {
    // Windows 7 以降でのみ有効
#if 0
	OSVERSIONINFOEX ovi;
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	::GetVersionEx((OSVERSIONINFO*)&ovi);
	if( ovi.dwPlatformId == VER_PLATFORM_WIN32_NT &&
		ovi.dwMajorVersion >= 6 && ovi.dwMinorVersion >= 1 ) {

		int value = ::GetSystemMetrics(SM_DIGITIZER);
		if( value == 0 ) return;

		TVPAddLog( (const tjs_char*)TVPEnableDigitizer );
		if( value & NID_INTEGRATED_TOUCH ) {
			TVPAddLog( (const tjs_char*)TVPTouchIntegratedTouch );
		}
		if( value & NID_EXTERNAL_TOUCH ) {
			TVPAddLog( (const tjs_char*)TVPTouchExternalTouch );
		}
		if( value & NID_INTEGRATED_PEN ) {
			TVPAddLog( (const tjs_char*)TVPTouchIntegratedPen );
		}
		if( value & NID_EXTERNAL_PEN ) {
			TVPAddLog( (const tjs_char*)TVPTouchExternalPen );
		}
		if( value & NID_MULTI_INPUT ) {
			TVPAddLog( (const tjs_char*)TVPTouchMultiInput );
		}
		if( value & NID_READY ) {
			TVPAddLog( (const tjs_char*)TVPTouchReady );
		}
	}
#endif
}

void tTVPApplication::PostUserMessage(const std::function<void()> &func,
                                      void *host, int msg) {
    std::lock_guard<std::mutex> cs(m_msgQueueLock);
    m_lstUserMsg.emplace_back(host, msg, func);
}

void tTVPApplication::FilterUserMessage(
    const std::function<void(std::vector<std::tuple<void *, int, tMsg>> &)>
        &func) {
    std::lock_guard<std::mutex> cs(m_msgQueueLock);
    func(m_lstUserMsg);
}

void tTVPApplication::OnActivate() {
    application_activating_ = true;
    if(!_project_startup)
        return;

    //	TVPRestoreFullScreenWindowAtActivation();
    TVPResetVolumeToAllSoundBuffer();
    TVPUnlockSoundMixer();

    // trigger System.onActivate event
    TVPPostApplicationActivateEvent();
    for(auto &it : m_activeEvents) {
        it.second(it.first, eTVPActiveEvent::onActive);
    }
}
void tTVPApplication::OnDeactivate() {
    application_activating_ = false;
    if(!_project_startup)
        return;

    //	TVPMinimizeFullScreenWindowAtInactivation();

    // fire compact event
    TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_DEACTIVATE);

    // set sound volume
    TVPResetVolumeToAllSoundBuffer();
    TVPLockSoundMixer();

    // trigger System.onDeactivate event
    TVPPostApplicationDeactivateEvent();
    for(auto &it : m_activeEvents) {
        it.second(it.first, eTVPActiveEvent::onDeactive);
    }
}

void tTVPApplication::OnExit() {
    TVPUninitScriptEngine();

    delete TVPSystemControl;
    TVPSystemControl = nullptr;

    CloseConsole();
}

void tTVPApplication::OnLowMemory() {
    if(!_project_startup)
        return;
    TVPDeliverCompactEvent(TVP_COMPACT_LEVEL_MAX);
}

bool tTVPApplication::GetNotMinimizing() const {
    return !application_activating_;
#if 0
	HWND hWnd = GetMainWindowHandle();
	if( hWnd != INVALID_HANDLE_VALUE && hWnd != nullptr ) {
		return ::IsIconic( hWnd ) == 0;
	}
	return true; // メインがない時は最小化されているとみなす
#endif
}
#if 0
void tTVPApplication::OnActiveAnyWindow() {
	if( modal_window_stack_.empty() != true ) {
		tTVPWindow* win = modal_window_stack_.top();
		if( win->GetVisible() && win->GetEnable() ) {
			win->BringToFront();
		}
	}
}
void tTVPApplication::ModalFinished() {
	modal_window_stack_.pop();
	if( modal_window_stack_.empty() != true ) {
		tTVPWindow* win = modal_window_stack_.top();
		if( win->GetVisible() && win->GetEnable() ) {
			win->BringToFront();
		}
	}
}
#endif
void tTVPApplication::LoadImageRequest(class iTJSDispatch2 *owner,
                                       class tTJSNI_Bitmap *bmp,
                                       const ttstr &name) {
    if(image_load_thread_) {
        image_load_thread_->LoadRequest(owner, bmp, name);
    }
}

void tTVPApplication::RegisterActiveEvent(
    void *host,
    const std::function<void(void *, eTVPActiveEvent)>
        &func /*empty = unregister*/) {
    if(func)
        m_activeEvents.emplace(host, func);
    else
        m_activeEvents.erase(host);
}

#if 0
std::vector<std::string>* LoadLinesFromFile( const std::wstring& path ) {
	FILE *fp = nullptr;
	_wfopen_s( &fp, path.c_str(), L"r");
    if( fp == nullptr ) {
		return nullptr;
    }
	char buff[1024];
	std::vector<std::string>* ret = new std::vector<std::string>();
    while( fgets(buff, 1024, fp) != nullptr ) {
		ret->push_back( std::string(buff) );
    }
    fclose(fp);
	return ret;
}

void TVPRegisterAcceleratorKey(HWND hWnd, char virt, short key, short cmd) {
	if( Application ) Application->RegisterAcceleratorKey( hWnd, virt, key, cmd );
}
void TVPUnregisterAcceleratorKey(HWND hWnd, short cmd) {
	if( Application ) Application->UnregisterAcceleratorKey( hWnd, cmd );
}
void TVPDeleteAcceleratorKeyTable( HWND hWnd ) {
	if( Application ) Application->DeleteAcceleratorKeyTable( hWnd );
}
#endif

void TVPInitWindowOptions() {}

std::string ExtractFileDir(const std::string &FileName) {
#if MY_USE_FFMPEG
    return av_dirname((char *)FileName.c_str());
#else
    return dirname((char *)FileName.c_str());
#endif
}

unsigned long ColorToRGB(unsigned int col) {
    // 0xBBGGRR
    switch(col) {
        case clScrollBar:
            return 0xc8c8c8;
        case clBackground:
            return 0;
        case clActiveCaption:
            return 0xd1b499;
        case clInactiveCaption:
            return 0xdbcdbf;
        case clMenu:
            return 0xf0f0f0;
        case clWindow:
            return 0xffffff;
        case clWindowFrame:
            return 0x646464;
        case clMenuText:
        case clWindowText:
            return 0;
        case clCaptionText:
            return 0;
        case clActiveBorder:
            return 0xb4b4b4;
        case clInactiveBorder:
            return 0xfcf7f4;
        case clAppWorkSpace:
            return 0xababab;
        case clHighlight:
            return 0xff9933;
        case clHighlightText:
            return 0xffffff;
        case clBtnFace:
            return 0xf0f0f0;
        case clBtnShadow:
            return 0xa0a0a0;
        case clGrayText:
            return 0x6d6d6d;
        case clBtnText:
            return 0;
        case clInactiveCaptionText:
            return 0x544e43;
        case clBtnHighlight:
            return 0xffffff;
        case cl3DDkShadow:
            return 0x696969;
        case cl3DLight:
            return 0xe3e3e3;
        case clInfoText:
            return 0;
        case clInfoBk:
            return 0xe1ffff;
        case clUnknown:
            return 0;
        case clHotLight:
            return 0xcc6600;
        case clGradientActiveCaption:
            return 0xead1b9;
        case clGradientInactiveCaption:
            return 0xf2e4d7;
        case clMenuLight:
            return 0xff9933;
        case clMenuBar:
            return 0xf0f0f0;
        default:
            return col & 0xFFFFFF;
    }
}
