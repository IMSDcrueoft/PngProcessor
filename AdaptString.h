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
	size_t requireSize = 4 * size;
	/*
	字节数 : 1;编码：GB2312
	字节数 : 1;编码：GBK
	字节数 : 1;编码：GB18030
	字节数 : 1;编码：ISO-8859-1
	字节数 : 1;编码：UTF-8
	字节数 : 4;编码：UTF-16
	字节数 : 2;编码：UTF-16BE
	字节数 : 2;编码：UTF-16LE
	中文汉字：
	字节数 : 2;编码：GB2312
	字节数 : 2;编码：GBK
	字节数 : 2;编码：GB18030
	字节数 : 1;编码：ISO-8859-1
	字节数 : 3;编码：UTF-8
	字节数 : 4;编码：UTF-16
	字节数 : 2;编码：UTF-16BE
	字节数 : 2;编码：UTF-16LE
	*/
	STR str = static_cast<STR>(operator new(requireSize + sizeof(char)));

	size_t converted = 0;

	size_t error = wcstombs_s(&converted, str, requireSize + sizeof(char), cwstr, requireSize);
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
