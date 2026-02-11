#include "pch.h"
#include "css_cosmetic.h"
#include "loader.h"
#include "log_thread.h"
#include "pattern.h"

void vbar_noop(const char* file_name, void* buffer, size_t bufferSize) noexcept {}

static inline bool is_homepage_vbar_hide() noexcept
{
	auto result = GetPrivateProfileIntA(
		"Homepage_vbar",
		"Enable",
		0,
		CONFIG_FILEA
	);
	return 0 != result;
}

static inline void do_hide_vbar(const char* file_name, void* buffer, size_t bufferSize) noexcept
{
	static char vbar_buffer[2048]{};
	size_t len = strnlen_s(file_name, 128);
	if (len < 4 || 0 != _stricmp(file_name + len - 4, ".css")) {
		return;
	}
	
	Modify modify{};

	const auto signature_raw_length = GetPrivateProfileStringA(
		"Homepage_vbar",
		"Signature",
		"",
		vbar_buffer,
		sizeof(vbar_buffer),
		CONFIG_FILEA
	);

	const auto signature_hex_size = parse_signaure(vbar_buffer,
		signature_raw_length,
		modify.signature,
		modify.mask,
		sizeof(vbar_buffer));

	if (SIZE_MAX == signature_hex_size) {
		log_debug("do_hide_vbar: parse_signaure limit exceed.");
		return;
	}

	modify.mask[signature_hex_size] = '\0';

	modify.offset = GetPrivateProfileIntA(
		"Homepage_vbar",
		"Offset",
		0,
		CONFIG_FILEA
	);

	const auto value_raw_length = GetPrivateProfileStringA(
		"Homepage_vbar",
		"Value",
		"",
		vbar_buffer,
		sizeof(vbar_buffer),
		CONFIG_FILEA
	);

	modify.patch_size = parse_hex(
		vbar_buffer,
		value_raw_length,
		modify.value,
		sizeof(vbar_buffer)
	);

	if (SIZE_MAX == modify.patch_size) {
		log_debug("do_hide_vbar: parse_hex limit exceed.");
		return;
	}

	if (modify.patch_size > signature_hex_size) {
		log_debug("do_hide_vbar: patch_size > signature_hex_size.");
		return;
	}

	const auto address = FindPattern(
		reinterpret_cast<BYTE*>(buffer),
		static_cast<DWORD>(bufferSize),
		modify.signature,
		reinterpret_cast<char*>(&modify.mask)
	);

	if (nullptr == address) {
		_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "do_hide_vbar: %s FindPattern failed.", file_name);
		log_debug(shared_buffer);
		return;
	}

	if (buffer != address) {
		// it the first in the css file...
		return;
	}
	if (address) {
		memcpy(address + modify.offset, modify.value, modify.patch_size);
	}
	_snprintf_s(shared_buffer, SHARED_BUFFER_SIZE, _TRUNCATE, "do_hide_vbar: %s patched.", file_name);
	log_info(shared_buffer);
}

void modify_css_init() noexcept
{
	if (true == is_homepage_vbar_hide()) {
		css_hide_vbar = do_hide_vbar;
	}
}
