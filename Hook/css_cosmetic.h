#pragma once

void vbar_noop(const char* file_name, void* buffer, size_t bufferSize) noexcept;
void modify_css_init() noexcept;

using css_hide_vbar_t = void (*)(const char* file_name, void* buffer, size_t bufferSize) noexcept;
inline css_hide_vbar_t css_hide_vbar = vbar_noop;