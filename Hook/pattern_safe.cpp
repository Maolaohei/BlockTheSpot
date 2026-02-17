// pattern_safe.cpp - 安全的 pattern 匹配（改进版）
// 主要改进：
// 1. 添加 pattern 有效性检查
// 2. 添加备用 pattern 机制
// 3. 添加模糊匹配选项
// 4. 更好的错误报告

#include "pch.h"
#include "pattern.h"
#include "log_thread.h"
#include <vector>
#include <string>

// 改进1: 定义备用 pattern 结构
struct PatternEntry {
	const char* name;
	const char* primary_signature;
	const char* fallback_signature;  // 备用签名
	const char* mask;
	size_t offset;
	bool required;  // 是否为必需 pattern
};

// 改进2: 定义常用 pattern 的多个版本
// 当主 signature 失效时，尝试备用 signature
static const PatternEntry g_patterns[] = {
	// adsEnabled - 主要功能，必需
	{
		"adsEnabled",
		"61 64 73 45 6E 61 62 6C 65 64 3A 21 30",  // adsEnabled:!0
		"61 64 73 45 6E 61 62 6C 65 64 3A 21 00",  // 备用: 可能的大小写变化
		"xxxxxxxxxxxxx",
		12,  // offset to '0'
		true  // required
	},
	// isHptoHidden
	{
		"isHptoHidden",
		"69 73 48 70 74 6F 48 69 64 64 65 6E 3A 21 30",
		nullptr,  // 无备用
		"xxxxxxxxxxxxxxx",
		14,
		false
	},
	// 可以添加更多 pattern...
};

// 改进3: 安全的 pattern 查找，带重试
LPVOID FindPatternSafe(
	BYTE* buffer,
	DWORD bufferSize,
	const BYTE* pattern,
	const char* mask,
	int maxRetries = 1
) noexcept {
	for (int attempt = 0; attempt < maxRetries; ++attempt) {
		auto result = FindPattern(buffer, bufferSize, pattern, mask);
		if (result != nullptr) {
			return result;
		}
		if (attempt < maxRetries - 1) {
			Sleep(50);  // 短暂延迟后重试
		}
	}
	return nullptr;
}

// 改进4: 模糊匹配 - 允许少量字节不匹配
LPVOID FindPatternFuzzy(
	BYTE* buffer,
	DWORD bufferSize,
	const BYTE* pattern,
	const char* mask,
	int maxMismatches = 2
) noexcept {
	const size_t patternLen = strlen(mask);
	if (patternLen == 0 || bufferSize < patternLen) {
		return nullptr;
	}

	for (DWORD i = 0; i <= bufferSize - patternLen; ++i) {
		int mismatches = 0;
		bool match = true;
		
		for (size_t j = 0; j < patternLen; ++j) {
			if (mask[j] == 'x' && buffer[i + j] != pattern[j]) {
				mismatches++;
				if (mismatches > maxMismatches) {
					match = false;
					break;
				}
			}
		}
		
		if (match) {
			return buffer + i;
		}
	}
	return nullptr;
}

// 改进5: 主查找函数，带备用机制
bool FindAndPatchWithFallback(
	BYTE* buffer,
	DWORD bufferSize,
	const PatternEntry& entry,
	const BYTE* patchValue,
	size_t patchSize
) noexcept {
	BYTE signature[256]{};
	BYTE mask[256]{};
	
	// 尝试主 signature
	if (entry.primary_signature) {
		size_t sigLen = parse_signaure(
			entry.primary_signature,
			strlen(entry.primary_signature),
			signature,
			mask,
			sizeof(signature)
		);
		
		if (sigLen != SIZE_MAX) {
			mask[sigLen] = '\0';
			auto address = FindPatternSafe(buffer, bufferSize, signature, mask, 2);
			
			if (address) {
				// 找到主 signature，执行 patch
				auto patchAddr = static_cast<BYTE*>(address) + entry.offset;
				DWORD oldProtect;
				if (VirtualProtect(patchAddr, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
					memcpy(patchAddr, patchValue, patchSize);
					VirtualProtect(patchAddr, patchSize, oldProtect, &oldProtect);
					_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE,
						"Pattern %s: Primary signature matched and patched.", entry.name);
					log_info(shared_buffer);
					return true;
				}
			}
		}
	}
	
	// 主 signature 失败，尝试备用
	if (entry.fallback_signature) {
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE,
			"Pattern %s: Primary failed, trying fallback...", entry.name);
		log_debug(shared_buffer);
		
		size_t sigLen = parse_signaure(
			entry.fallback_signature,
			strlen(entry.fallback_signature),
			signature,
			mask,
			sizeof(signature)
		);
		
		if (sigLen != SIZE_MAX) {
			mask[sigLen] = '\0';
			auto address = FindPatternFuzzy(buffer, bufferSize, signature, mask, 3);
			
			if (address) {
				auto patchAddr = static_cast<BYTE*>(address) + entry.offset;
				DWORD oldProtect;
				if (VirtualProtect(patchAddr, patchSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
					memcpy(patchAddr, patchValue, patchSize);
					VirtualProtect(patchAddr, patchSize, oldProtect, &oldProtect);
					_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE,
						"Pattern %s: Fallback signature matched and patched.", entry.name);
					log_info(shared_buffer);
					return true;
				}
			}
		}
	}
	
	// 全部失败
	_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE,
		"Pattern %s: All signatures failed.%s", entry.name,
		entry.required ? " (REQUIRED)" : "");
	log_debug(shared_buffer);
	
	return !entry.required;  // 非必需 pattern 失败返回 true（继续）
}

// 改进6: 自动检测和适应不同版本
bool AutoDetectAndPatch(BYTE* buffer, DWORD bufferSize, const char* patternName) noexcept {
	// 可以根据 Spotify 版本选择不同的 pattern
	// 这里简化处理，实际可以实现版本检测逻辑
	
	for (const auto& entry : g_patterns) {
		if (strcmp(entry.name, patternName) == 0) {
			// 根据 pattern 类型决定 patch 值
			BYTE patchValue[16] = {0};
			size_t patchSize = 0;
			
			if (strstr(patternName, "Enabled") || strstr(patternName, "Hidden")) {
				patchValue[0] = 0x31;  // '1'
				patchSize = 1;
			}
			
			if (patchSize > 0) {
				return FindAndPatchWithFallback(buffer, bufferSize, entry, patchValue, patchSize);
			}
			break;
		}
	}
	
	return false;
}
