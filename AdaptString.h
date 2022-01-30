#pragma once

#include <cstdlib>
#include <string>
#include <cassert>

class AdaptString
{
	using STR = char*;
	using WSTR = wchar_t*;
	using CSTR = const char*;
	using CWSTR = const wchar_t*;

public:
	static STR toSTR(CWSTR cwstr,const size_t& size);
	static std::string toString(CWSTR cwstr, const size_t& size);
	static std::string toString(const std::wstring& wstr);
};

inline AdaptString::STR AdaptString::toSTR(CWSTR cwstr, const size_t& size)
{
	size_t requireSize = sizeof(wchar_t) * size;
	STR str = static_cast<STR>(operator new(requireSize + sizeof(char)));

	size_t converted = 0;

	size_t error = wcstombs_s(&converted, str, requireSize + 1, cwstr, requireSize);//std::wcstombs(str, cwstr, requireSize);
	assert(error != -1 && "string convert error.");
	return str;
}

inline std::string AdaptString::toString(CWSTR cwstr, const size_t& size)
{
	STR ptr = AdaptString::toSTR(cwstr, size);
	std::string result = ptr;
	operator delete(ptr);
	return result;
}

inline std::string AdaptString::toString(const std::wstring& wstr)
{
	return AdaptString::toString(wstr.c_str(), wstr.size());
}
