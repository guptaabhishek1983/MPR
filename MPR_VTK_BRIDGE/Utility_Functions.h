#include "Stdafx.h"


System::String^ convert_to_managed_string(string str)
{
	System::String^ mgdStr = gcnew String(str.c_str());
	return mgdStr;
}

System::String^ convert_to_managed_string(wstring str)
{
	System::String^ mgdStr = gcnew System::String(str.c_str());
	return mgdStr;
}


System::String^ convert_to_managed_string(const char* str)
{
	String^ mgdStr = gcnew String(str);
	return mgdStr;
}

System::String^ convert_to_managed_string(char* str)
{
	System::String^ mgdStr = gcnew String(str);
	return mgdStr;
}

const char* convert_to_const_charPtr(System::String^ str)
{
	const char* stlStr = (const char*)Marshal::StringToHGlobalAnsi(str).ToPointer();
	return stlStr;
}
