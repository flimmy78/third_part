#pragma once
#include <string>
using namespace std;

class Util
{
public:
	Util();
	~Util();

	static string base64_encode(unsigned char const*, unsigned int len);
	static string base64_decode(string const& s);

	static BOOL StringToWString(const std::string &str, std::wstring &wstr);
	static BOOL WStringToString(const std::wstring &wstr, std::string &str);
};

