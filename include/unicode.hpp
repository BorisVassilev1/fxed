#pragma once

#include <string>

namespace nri {
std::wstring UNICODE_TO_WIDE(const std::string &str);
std::string	 WIDE_TO_UNICODE(const std::wstring &wstr);
}	  // namespace nri
