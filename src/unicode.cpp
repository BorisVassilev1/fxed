#include "unicode.hpp"

namespace nri {
std::wstring UNICODE_TO_WIDE(const std::string &str) {
	std::wstring wstr;
	wstr.resize(str.size());
	std::mbstowcs(&wstr[0], str.c_str(), str.size());
	return wstr;
}

std::string WIDE_TO_UNICODE(const std::wstring &wstr) {
	std::string str;
	str.resize(wstr.size());
	std::wcstombs(&str[0], wstr.c_str(), wstr.size());
	return str;
}
}	  // namespace nri
