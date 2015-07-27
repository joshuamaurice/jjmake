// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include <algorithm>
#include <limits>
#include <climits>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;

void trim(string& x)
{
    for (size_t i=0; i<x.size(); ++i)
    {
        if (x[i] != ' ')
        {
            x = x.substr(i);
            break;
        }
    }
    for (size_t i=x.size(); i>0; --i)
    {
        if (x[i-1] != ' ')
        {
            x = x.substr(0, i);
            break;
        }
    }
}

unsigned int toUInt(string const& x)
{
    stringstream ss;
    ss << x;
    unsigned int retval = 0;
    ss >> std::hex >> retval;
    if (!ss)
    {   cerr << __FILE__ << " " << __LINE__ << " Bad string to unsigned int \"" << x << "\"" << endl;
        exit(1);
    }
    ss.get();
    if (!!ss)
    {   cerr << __FILE__ << " " << __LINE__ << " Bad string to unsigned int \"" << x << "\"" << endl;
        exit(1);
    }
    return retval;
}

class UIntRange
{
public:
    UIntRange() : first(static_cast<unsigned int>(-1)), last(static_cast<unsigned int>(-1)) {}
    UIntRange(unsigned int first_, unsigned int last_) : first(first_), last(last_) {}
    unsigned int first, last; //last not inclusive
};
bool operator< (UIntRange a, UIntRange b) //works only on non-overlapping ranges
{
    if (a.first <= b.first && b.last <= a.last)
    {   cerr << __FILE__ << " " << __LINE__ << endl;
        exit(1); //overlapping ranges
    }
    if (a.first <= b.first && b.first < a.last && a.last <= b.last)
    {   cerr << __FILE__ << " " << __LINE__ << endl;
        exit(1); //overlapping ranges
    }
    if (b.first <= a.first && a.first < b.last && b.last <= a.last)
    {   cerr << __FILE__ << " " << __LINE__ << endl;
        exit(1); //overlapping ranges
    }
    return a.first < b.first;
}

enum OptionalBool { NotSet = 0, SetTrue, SetFalse };

void setUnset(OptionalBool & x, OptionalBool v)
{   
    if (x == NotSet)
        x = v;
}

void initGraphemeBreakTable(
        vector<vector<OptionalBool> > & graphemeBreakTable,
        map<string, unsigned int> & mapCategoryNameToFlag)
{   
    graphemeBreakTable.resize(mapCategoryNameToFlag.size());
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        graphemeBreakTable[i].resize(mapCategoryNameToFlag.size());

    //GB3
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["CR"]][mapCategoryNameToFlag["LF"]], SetFalse);

    //GB4
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[mapCategoryNameToFlag["Control"]][i], SetTrue);
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[mapCategoryNameToFlag["CR"]][i], SetTrue);
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[mapCategoryNameToFlag["LF"]][i], SetTrue);

    //GB5
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[i][mapCategoryNameToFlag["Control"]], SetTrue);
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[i][mapCategoryNameToFlag["CR"]], SetTrue);
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[i][mapCategoryNameToFlag["LF"]], SetTrue);

    //GB6
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["L"]][mapCategoryNameToFlag["L"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["L"]][mapCategoryNameToFlag["V"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["L"]][mapCategoryNameToFlag["LV"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["L"]][mapCategoryNameToFlag["LVT"]], SetFalse);

    //GB7
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["LV"]][mapCategoryNameToFlag["V"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["LV"]][mapCategoryNameToFlag["T"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["V"]][mapCategoryNameToFlag["V"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["V"]][mapCategoryNameToFlag["T"]], SetFalse);

    //GB8
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["LVT"]][mapCategoryNameToFlag["T"]], SetFalse);
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["T"]][mapCategoryNameToFlag["T"]], SetFalse);

    //GB8a
    setUnset(graphemeBreakTable[mapCategoryNameToFlag["Regional_Indicator"]][mapCategoryNameToFlag["Regional_Indicator"]], SetFalse);

    //GB9
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[i][mapCategoryNameToFlag["Extend"]], SetFalse);

    //GB9a
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        setUnset(graphemeBreakTable[i][mapCategoryNameToFlag["SpacingMark"]], SetFalse);

    //GB9b -- currently there are no characters with this value.
    //for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
    //    setUnset(graphemeBreakTable[mapCategoryNameToFlag["Prepend"]][i], SetFalse);

    //GB10
    for (size_t i=0; i<mapCategoryNameToFlag.size(); ++i)
        for (size_t j=0; j<mapCategoryNameToFlag.size(); ++j)
            setUnset(graphemeBreakTable[i][j], SetTrue);
}
    


#ifdef _WIN32
    int wmain()
#else
    int main()
#endif
{
    static_assert(CHAR_BIT == 8, "ERROR"); 
    static_assert(sizeof(int) >= 4, "ERROR"); 

    map<string, set<UIntRange> > graphemeBreakProperties;

    ifstream in("GraphemeBreakProperty.txt");
    if (!in)
    {   cerr << __FILE__ << " " << __LINE__ << endl;
        exit(1); 
    }
    for (string originalLine; getline(in, originalLine); )
    {
        stringstream ss;
        ss << originalLine;
        ss >> ws;
        string line = ss.str();
        line = line.substr(0, line.find('#'));
        if (line.size() == 0)
            continue;
        size_t semicolon = line.find(';');
        if (semicolon == string::npos)
        {
            cerr << "Bad line: \"" << originalLine << "\"" << endl;
            return 1;
        }
        string category = line.substr(semicolon + 1);
        trim(category);
        line = line.substr(0, semicolon);
        trim(line);

        unsigned int firstRange, lastInclusiveRange;
        size_t dotdot = line.find("..");
        if (dotdot == string::npos)
        {
            firstRange = lastInclusiveRange = toUInt(line);
        }
        else
        {
            string firstRangeStr = line.substr(0, dotdot);
            string lastInclusiveRangeStr = line.substr(dotdot+2);
            firstRange = toUInt(firstRangeStr);
            lastInclusiveRange = toUInt(lastInclusiveRangeStr);
        }

        graphemeBreakProperties[category].insert(UIntRange(firstRange, lastInclusiveRange+1));
    }

    map<string, unsigned int> mapCategoryNameToFlag;
    for (map<string, set<UIntRange> >::const_iterator 
            x = graphemeBreakProperties.begin(); x != graphemeBreakProperties.end(); ++x)
    {   mapCategoryNameToFlag[x->first];
        mapCategoryNameToFlag[x->first] = static_cast<unsigned int>(mapCategoryNameToFlag.size() - 1); 
    }
    mapCategoryNameToFlag["other"];
    mapCategoryNameToFlag["other"] = static_cast<unsigned int>(mapCategoryNameToFlag.size() - 1); 

    map<UIntRange, string> graphemeBreakProperties2;
    for (map<string, set<UIntRange> >::const_iterator 
            x = graphemeBreakProperties.begin(); x != graphemeBreakProperties.end(); ++x)
    {   for (set<UIntRange>::const_iterator y = x->second.begin(); y != x->second.end(); ++y)
        {   graphemeBreakProperties2[*y] = x->first;
        }
    }


    cout << hex;
    cout << "/* This file is machine generated using data from: " << '\n';
    cout << "  http://www.unicode.org/Public/UNIDATA/auxiliary/GraphemeBreakProperty.txt  */" << '\n';
    cout << '\n';

    cout << "#ifdef __cplusplus" << '\n';
    cout << "#error" << '\n';
    cout << "#endif" << '\n';
    cout << '\n';

    for (map<string, unsigned int>::const_iterator 
            x = mapCategoryNameToFlag.begin(); x != mapCategoryNameToFlag.end(); ++x)
    {   cout << "/* " << x->first << " = 0x" << (x->second) << " */" << '\n';
    }
    cout << '\n';
    
    vector<vector<OptionalBool> > graphemeBreakTable;
    initGraphemeBreakTable(graphemeBreakTable, mapCategoryNameToFlag);

    vector<unsigned int> graphemeBreakTable2(graphemeBreakTable.size(), 0);
    for (size_t i=0; i<graphemeBreakTable.size(); ++i)
    {   unsigned int bitField = 0;
        for (size_t j=0; j<graphemeBreakTable.size(); ++j)
            if (graphemeBreakTable[i][j] == SetTrue)
                bitField |= (1 << j);
        graphemeBreakTable2[i] = bitField;
    }

//#ifdef _WIN32
//    cout << "#pragma data_seg(\"shared\")" << '\n';
//#endif

    cout << "/* This is the table for the default grapheme cluster boundary." << '\n';
    cout << "For two adjacent unicode code points, AB, get the categories C_A and C_B." << '\n';
    cout << "There is a grapheme cluster break iff (value of index C_A) & (1 << C_B) is non-zero. */ " << '\n';

//#ifdef _WIN32
//    cout << "__declspec(dllexport) ";
//#endif
    cout << "unsigned int const jjUnicodeGraphemeBreakTable[] = { " << '\n';
    for (size_t i=0; i<graphemeBreakTable2.size(); ++i)
        cout << "    0x" << graphemeBreakTable2[i] << "," << '\n';
    cout << "    0 };" << '\n';
    cout << '\n';

    cout << "/* For a unicode code point P, find the entry E[?] in the sorted table so that:" << '\n';
    cout << "P <= E[?][0] and E[?][0] the least such entry." << '\n';
    cout << "(The C++ std lib function std::lower_bound is useful to find this.)" << '\n';
    cout << "Then E[?][1] is the grapheme break property for P. */" << '\n';

//#ifdef _WIN32
//    cout << "__declspec(dllexport) ";
//#endif
    cout << "unsigned int const jjUnicodeGraphemeBreakPropertiesStruct[][2] = {" << '\n';

    unsigned int prevLastNotInclusive = 0;
    for (map<UIntRange, string>::const_iterator 
            x = graphemeBreakProperties2.begin(); x != graphemeBreakProperties2.end(); ++x)
    {
        unsigned int const first = x->first.first;
        unsigned int const lastNotInclusive = x->first.last;
        if (prevLastNotInclusive != first && first != 0)
        {   cout << "    { 0x" << first - 1 
                    << " , 0x" << mapCategoryNameToFlag["other"] 
                    << " } , " << '\n';
        }
        cout << "    { 0x" << lastNotInclusive - 1 
                << " , 0x" << mapCategoryNameToFlag[x->second] 
                << " } , /* " << x->second << " */" << '\n';
        prevLastNotInclusive = lastNotInclusive;
    }
    cout << "    { 0xFFFFFFFF , 0x" << mapCategoryNameToFlag["other"] << " } " << '\n';
    cout << "    };" << '\n';

//#ifdef _WIN32
//    cout << "__declspec(dllexport) ";
//#endif
    cout << "unsigned int const jjUnicodeGraphemeBreakPropertiesStructLength = " << '\n';
    cout << "        sizeof(jjUnicodeGraphemeBreakPropertiesStruct) / sizeof(jjUnicodeGraphemeBreakPropertiesStruct[0]);" << '\n';

//#ifdef _WIN32
//    cout << "#pragma data_seg()" << '\n';
//#endif
    return 0; 
}
