// loader.cpp - 改进版
// 主要改进：
// 1. 添加延迟加载，等待 CEF 初始化完成
// 2. 添加版本检查，不匹配时跳过 hooks
// 3. 添加错误回退，单个 hook 失败不崩溃
// 4. 添加重试机制，处理初始化时序问题

#include "pch.h"
#include "loader.h"
#include "developer_mode.h"
#include "IAT_hook.h"
#include "kill_crashpad.h"
#include "log_thread.h"
#include "cef_url_hook.h"
#include "cef_zip_reader_hook.h"
#include "libcef_hook.h"
#include "css_cosmetic.h"
#pragma	comment(lib, "version.lib")

// 改进1: 添加重试计数和延迟
static constexpr int MAX_INIT_RETRIES = 3;
static constexpr DWORD INIT_RETRY_DELAY_MS = 500;

// 改进2: 版本信息结构
struct SpotifyVersion {
	WORD major;
	WORD minor;
	WORD build;
	WORD revision;
};

bool remove_debug_log() noexcept
{
	wchar_t old_dpapi[MAX_PATH];
	DWORD len = GetCurrentDirectoryW(MAX_PATH, old_dpapi);
	if (len > 0 && len < MAX_PATH) {
		wcscat_s(old_dpapi, L"\\debug.log");
		return DeleteFileW(old_dpapi);
	}
	return false;
}

static inline bool remove_unused_dll() noexcept
{
	wchar_t old_dpapi[MAX_PATH];
	DWORD len = GetCurrentDirectoryW(MAX_PATH, old_dpapi);
	if (len > 0 && len < MAX_PATH) {
		wcscat_s(old_dpapi, L"\\dpapi.dll");
		return DeleteFileW(old_dpapi);
	}
	return false;
}

static inline void get_ImageDirectoryEntryToDataEx() noexcept
{
	auto dbghelp_dll_handle = GetModuleHandleW(L"dbghelp.dll");
	if (!dbghelp_dll_handle) {
		// 改进: 延迟加载 dbghelp
		dbghelp_dll_handle = LoadLibraryW(L"dbghelp.dll");
		if (!dbghelp_dll_handle) {
			// 如果加载失败，等待一下再试
			Sleep(100);
			dbghelp_dll_handle = LoadLibraryW(L"dbghelp.dll");
		}
	}
	if (!dbghelp_dll_handle) {
		OutputDebugStringW(L"Failed to load dbghelp.dll\n");
		return;
	}

	ImageDirectoryEntryToDataEx =
		reinterpret_cast<ImageDirectoryEntryToDataEx_t>(
			GetProcAddress(dbghelp_dll_handle, "ImageDirectoryEntryToDataEx")
			);

	if (nullptr == ImageDirectoryEntryToDataEx) {
		OutputDebugStringW(L"Failed to get ImageDirectoryEntryToDataEx address\n");
	}
}

static inline bool is_chrome_elf_required_exist() noexcept
{
	const auto required = CreateFileW(
		ORIGINAL_CHROME_ELF_DLL,
		GENERIC_READ,
		FILE_SHARE_READ,
		nullptr,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		nullptr
	);
	if (INVALID_HANDLE_VALUE == required) {
		return false;
	}
	CloseHandle(required);
	return true;
}

// 改进3: 获取 Spotify 版本
static inline bool get_spotify_version(SpotifyVersion& ver) noexcept {
	wchar_t spotify_path[MAX_PATH];
	DWORD len = GetModuleFileNameW(GetModuleHandleW(L"spotify.exe"), spotify_path, MAX_PATH);
	if (len == 0 || len >= MAX_PATH) {
		return false;
	}

	DWORD dummy;
	DWORD size = GetFileVersionInfoSizeW(spotify_path, &dummy);
	if (size == 0) {
		return false;
	}

	std::vector<BYTE> data(size);
	if (!GetFileVersionInfoW(spotify_path, 0, size, data.data())) {
		return false;
	}

	VS_FIXEDFILEINFO* fileInfo = nullptr;
	UINT fileInfoLen = 0;
	if (!VerQueryValueW(data.data(), L"\\", reinterpret_cast<LPVOID*>(&fileInfo), &fileInfoLen)) {
		return false;
	}

	if (fileInfo && fileInfoLen >= sizeof(VS_FIXEDFILEINFO)) {
		ver.major = HIWORD(fileInfo->dwFileVersionMS);
		ver.minor = LOWORD(fileInfo->dwFileVersionMS);
		ver.build = HIWORD(fileInfo->dwFileVersionLS);
		ver.revision = LOWORD(fileInfo->dwFileVersionLS);
		return true;
	}
	return false;
}

// 改进4: 版本兼容性检查
static inline bool is_version_supported(const SpotifyVersion& ver) noexcept {
	// 支持的版本范围: 1.2.x - 1.2.99
	// 可以根据需要调整
	return (ver.major == 1 && ver.minor == 2);
}

// 改进5: 带重试的模块加载
static inline HMODULE load_module_with_retry(const wchar_t* module_name, int retries = MAX_INIT_RETRIES) noexcept {
	for (int i = 0; i < retries; ++i) {
		HMODULE handle = GetModuleHandleW(module_name);
		if (handle) return handle;
		
		handle = LoadLibraryW(module_name);
		if (handle) return handle;
		
		if (i < retries - 1) {
			log_debug("Retry %d: Failed to load %ls, waiting...", i + 1, module_name);
			Sleep(INIT_RETRY_DELAY_MS);
		}
	}
	return nullptr;
}

// 改进6: 安全的 hook 包装函数
static inline bool safe_hook_developer_mode(HMODULE spotify_dll) noexcept {
	__try {
		hook_developer_mode(spotify_dll);
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		log_debug("hook_developer_mode crashed, continuing without it.");
		return false;
	}
}

static inline bool safe_hook_libcef(HMODULE spotify_dll) noexcept {
	__try {
		return libcef_IAT_hook_GetProcAddress(spotify_dll);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		log_debug("libcef_IAT_hook_GetProcAddress crashed, continuing without it.");
		return false;
	}
}

static inline bool safe_hook_cef_url(HMODULE libcef_dll) noexcept {
	__try {
		return hook_cef_url(libcef_dll);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		log_debug("hook_cef_url crashed, continuing without it.");
		return false;
	}
}

static inline bool safe_modify_css() noexcept {
	__try {
		modify_css_init();
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		log_debug("modify_css_init crashed, CSS modifications disabled.");
		return false;
	}
}

VOID CALLBACK bts_main(ULONG_PTR param)
{
	// 改进7: 延迟初始化，等待 Spotify 准备好
	Sleep(300);  // 初始延迟，让 Spotify 完成基本初始化
	
	get_ImageDirectoryEntryToDataEx();
	process_IAT_hook_GetProcAddress(GetModuleHandleW(NULL));
	
	const wchar_t* cmd = reinterpret_cast<const wchar_t*>(param);
	
	// Spotify's main process
	if (NULL == wcsstr(cmd, L"--type=") && NULL == wcsstr(cmd, L"--url=")) {
		init_log_thread();
		
		if (false == is_chrome_elf_required_exist()) {
			log_info("chrome_elf_required.dll file not found, Did you skip something?");
			return;
		}
		
		// 改进8: 版本检查
		SpotifyVersion ver{};
		if (get_spotify_version(ver)) {
			_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, 
				"Spotify version: %d.%d.%d.%d", ver.major, ver.minor, ver.build, ver.revision);
			log_info(shared_buffer);
			
			if (!is_version_supported(ver)) {
				_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE,
					"Version %d.%d not supported, running in safe mode (ads only).", ver.major, ver.minor);
				log_info(shared_buffer);
				// 版本不支持时，只启用最基本的 URL 拦截
				goto minimal_mode;
			}
		} else {
			log_debug("Failed to get Spotify version, proceeding with caution.");
		}
		
		if (true == remove_unused_dll()) {
			log_debug("Remove unused dpapi.dll.");
		}
		
		// 改进9: 带重试的模块加载
		HMODULE spotify_dll_handle = load_module_with_retry(L"spotify.dll");
		HMODULE libcef_dll_handle = load_module_with_retry(L"libcef.dll");

		if (!spotify_dll_handle) {
			log_debug("Failed to load spotify.dll after retries.");
			return;
		}
		if (!libcef_dll_handle) {
			log_debug("Failed to load libcef.dll after retries.");
			return;
		}
		
		// 额外延迟，确保 CEF 完全初始化
		Sleep(200);

		// 改进10: 逐个安全地执行 hooks
		bool dev_ok = safe_hook_developer_mode(spotify_dll_handle);
		bool libcef_ok = safe_hook_libcef(spotify_dll_handle);
		bool url_ok = safe_hook_cef_url(libcef_dll_handle);
		bool reader_ok = false;
		
		__try {
			reader_ok = hook_cef_reader(libcef_dll_handle);
		}
		__except (EXCEPTION_EXECUTE_HANDLER) {
			log_debug("hook_cef_reader crashed, continuing without it.");
		}
		
		// CSS 修改最容易导致黑屏，放在最后且用安全包装
		bool css_ok = safe_modify_css();
		
		// 报告初始化状态
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE,
			"Hooks: dev=%d, libcef=%d, url=%d, reader=%d, css=%d",
			dev_ok, libcef_ok, url_ok, reader_ok, css_ok);
		log_info(shared_buffer);
		
		if (!dev_ok && !libcef_ok && !url_ok) {
			log_info("All hooks failed, BlockTheSpot may not work correctly.");
		} else if (!css_ok) {
			log_info("CSS modification failed, some cosmetic features disabled.");
		}
		
		log_info("Loader initialized.");
		return;
		
	minimal_mode:
		// 最小模式：只加载必要的 DLL，只执行 URL 拦截
		HMODULE libcef_min = load_module_with_retry(L"libcef.dll", 2);
		if (libcef_min) {
			safe_hook_cef_url(libcef_min);
			log_info("Minimal mode: URL blocking only.");
		}
	}
}
