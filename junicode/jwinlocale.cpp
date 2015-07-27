// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#if 0
#include "jwinlocale.hpp"

#include "jbase/juniqueptr.hpp"
#include "jbase/jfatal.hpp"
#include <map>

#ifdef _WIN32
    #include <windows.h>
#endif

using namespace jjm;
using namespace std;


#ifdef _WIN32
namespace
{
    BOOL CALLBACK enumSystemLocalesCallback(wchar_t * localeString, DWORD dwFlags, LPARAM x)
    {
        vector<WinLocale> * v = reinterpret_cast<vector<WinLocale> *>(x); 
        v->push_back(WinLocale());
        v->back().localeShortName = makeU8Str(Utf16String(localeString)); 
        return TRUE; 
    }

    std::string  getWin32LocaleStringProperty(std::string const& localeShortName, LCTYPE const prop)
    {
        Utf16String const& localeShortName2 = makeU16Str(localeShortName); 

        SetLastError(0); 
        DWORD const x = GetLocaleInfoEx(localeShortName2.c_str(), prop, 0, 0); 
        if (x == 0)
        {   DWORD const lastError = GetLastError(); 
            JFATAL(lastError, 0);
        }

        UniqueArray<wchar_t*> buf(new wchar_t[x]); 
        SetLastError(0); 
        DWORD const y = GetLocaleInfoEx(localeShortName2.c_str(), prop, buf.get(), x); 
        if (y == 0)
        {   DWORD const lastError = GetLastError(); 
            JFATAL(lastError, 0);
        }

        return makeU8Str(Utf16String(buf.get())); 
    }
}
#endif

vector<WinLocale> const& jjm::getWinLocales()
{
    static vector<WinLocale> * locales = 0;
    if (locales == 0)
    {
        locales = new vector<WinLocale>; 
#ifdef _WIN32
        SetLastError(0); 
        if (0 == EnumSystemLocalesEx( & enumSystemLocalesCallback, LOCALE_ALL, reinterpret_cast<LONG_PTR>(locales), 0))
        {   DWORD const lastError = GetLastError(); 
            JFATAL(lastError, 0);
        }

        for (vector<WinLocale>::iterator locale = locales->begin(); locale != locales->end(); ++locale)
        {   locale->countryLongName = getWin32LocaleStringProperty(locale->localeShortName, LOCALE_SCOUNTRY); 
            locale->countryShortName = getWin32LocaleStringProperty(locale->localeShortName, LOCALE_SISO3166CTRYNAME); 
            locale->languageLongName = getWin32LocaleStringProperty(locale->localeShortName, LOCALE_SLANGUAGE); 
            locale->languageShortName = getWin32LocaleStringProperty(locale->localeShortName, LOCALE_SISO639LANGNAME); 
        }

        //sanity check for conflicts
        map<Utf8String, Utf8String> mapCountry; 
        for (vector<WinLocale>::iterator locale = locales->begin(); locale != locales->end(); ++locale)
        {   Utf8String & x = mapCountry[locale->countryLongName]; 
            if (x.size() > 0 && x != locale->countryShortName)
                JFATAL(0, locale->countryLongName); 
            x = locale->countryShortName; 
        }

        //sanity check for conflicts
        map<Utf8String, Utf8String> mapLanguage; 
        for (vector<WinLocale>::iterator locale = locales->begin(); locale != locales->end(); ++locale)
        {   Utf8String & x = mapLanguage[locale->languageLongName]; 
            if (x.size() > 0 && x != locale->languageShortName)
                JFATAL(0, locale->languageLongName); 
            x = locale->languageShortName; 
        }
#endif
    }
    return *locales; 
}
namespace { bool b1 = (getWinLocales(), false); }


jjm::WinLocale const *  jjm::getWinLocale(std::string const& alias)
{
    std::string alias2 = alias; 
    if (alias2.find('.') != string::npos)
        alias2.erase(alias2.find('.')); 

    static map<string, WinLocale const*> * aliasMap = 0; 
    if (aliasMap == 0)
    {
        aliasMap = new map<string, WinLocale const*>;

        vector<WinLocale> const& winLocales = getWinLocales(); 
        for (vector<WinLocale>::const_iterator locale = winLocales.begin(); locale != winLocales.end(); ++locale)
        {   if (locale->languageLongName.size() > 0 && locale->countryLongName.size() > 0)
                (*aliasMap)[locale->languageLongName + "_" + locale->countryLongName] = &*locale; 
        }
    }

    map<string, WinLocale const*>::const_iterator x = aliasMap->find(alias2); 
    if (x == aliasMap->end())
        return 0; 
    return x->second; 
}
#endif
