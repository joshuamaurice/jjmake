// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "parsercontext.hpp"

#include "jbase/jfatal.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jinttostring.hpp"
#include "junicode/jstdouterr.hpp"
#include <fstream>

using namespace jjm;
using namespace std;

namespace
{
    string arrayValueToSingleString(vector<string> const& v)
    {
        string result; 
        bool needSpace = false; 
        for (size_t i = 0; i < v.size(); ++i)
        {   if (v[i].size() > 0)
            {   if (needSpace)
                    result += ' ';
                result += v[i]; 
                needSpace = true; 
            }
        }
        return result; 
    }

    class AddFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        AddFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() <= 1)
                throw std::runtime_error("Function 'add' takes 1 or more additional arguments."); 
            std::int64_t result = 0; 
            for (size_t i = 1; i < arguments.size(); ++i)
            {   std::int64_t x; 
                if (false == jjm::decStrToInteger(x, arguments[i]))
                    throw std::runtime_error("Function 'add' was given non-numeric argument \"" + arguments[i] + "\"."); 
                result += x; 
            }

            vector<string> result2;
            result2.push_back(toDecStr(result));
            return result2; 
        }
    };

    class EqualsFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        EqualsFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 3)
                throw std::runtime_error("Function 'eq' takes exactly 2 additional arguments."); 
            bool b = arguments[1] == arguments[2];
            vector<string> result;
            if (b)
                result.push_back("t");
            return result; 
        }
    };
    
    class NotEqualsFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        NotEqualsFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 3)
                throw std::runtime_error("Function 'neq' takes exactly 2 additional arguments."); 
            bool b = arguments[1] != arguments[2];
            vector<string> result;
            if (b)
                result.push_back("t");
            return result; 
        }
    };

    class GetFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        GetFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 2)
                throw std::runtime_error("Function 'get' takes exactly 1 additional argument."); 
            string const& name = arguments[1]; 
            if (name.size() == 0)
                throw std::runtime_error("Function 'get' does not accept an empty variable name."); 

            jjm::ParserContext::Value const* v = c->getValue(name); 

            vector<string> result; 
            if (v && v->value.size() > 0)
                result.push_back(v->value[0]); 
            else
                result.push_back(string()); 
            return result; 
        }
    };
    
    class GetAtFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        GetAtFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 2)
                throw std::runtime_error("Function 'get@' takes exactly 1 additional argument."); 
            string const& name = arguments[1]; 
            if (name.size() == 0)
                throw std::runtime_error("Function 'get@' does not accept an empty variable name."); 

            jjm::ParserContext::Value const* v = c->getValue(name); 

            vector<string> result; 
            if (v)
                result = v->value; 
            return result; 
        }
    };

    class GetStarFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        GetStarFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 2)
                throw std::runtime_error("Function 'get*' takes exactly 1 additional argument."); 
            string const& name = arguments[1]; 
            if (name.size() == 0)
                throw std::runtime_error("Function 'get*' does not accept an empty variable name."); 

            jjm::ParserContext::Value const* v = c->getValue(name); 

            vector<string> result; 
            if (v)
                result.push_back(arrayValueToSingleString(v->value)); 
            else
                result.push_back(string()); 
            return result; 
        }
    };

    class IfFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        IfFunction() { alwaysEvalArguments = false; }
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 3 && arguments.size() != 4)
                throw std::runtime_error("Function 'if' takes exactly 2 or 3 additional arguments."); 
            string const& cond = arguments[1]; 
            string const& trueBody = arguments[2]; 
            string const& falseBody = arguments.size() == 4 ? arguments[3] : string(); 

            vector<string> result; 
            if (cond.size() > 0)
                result.push_back(trueBody);
            else
                result.push_back(falseBody);
            return result; 
        }
        bool evalNextArgument(ParserContext * , vector<string> const& argumentsThusFar)
        {
            switch (argumentsThusFar.size())
            {
            case 1: return true; //we always want to eval the condition
            case 2: return argumentsThusFar[1].size() > 0; //eval the if-true-arg iff the cond is true, 
            case 3: return argumentsThusFar[1].size() == 0; //eval the if-false-arg iff the cond is false, 
            }
            return false; 
        }
    };

    class IncludeFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        IncludeFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 2)
                throw std::runtime_error("Function 'include' takes exactly 1 additional argument."); 

            //TODO change to not use ifstream
            string contents; 
            std::ifstream fin(arguments[1].c_str(), std::ios::binary); 
            if (!fin)
                throw std::runtime_error("Function 'include' unable to open file \"" + arguments[1] + "\"."); 
            for (string line; getline(fin, line); )
            {   contents += line;
                contents += '\n'; 
            }

            string prevFile;
            jjm::ParserContext::Value const* prevFileClass = c->getValue(".FILE");
            if (prevFileClass && prevFileClass->value.size())
                prevFile = prevFileClass->value[0]; 

            string prevLine;
            jjm::ParserContext::Value const* prevLineClass = c->getValue(".LINE");
            if (prevLineClass && prevLineClass->value.size())
                prevLine = prevLineClass->value[0]; 

            string prevCol;
            jjm::ParserContext::Value const* prevColClass = c->getValue(".COL");
            if (prevColClass && prevColClass->value.size())
                prevCol = prevColClass->value[0]; 

            c->setValue(".FILE", arguments[1]); 
            c->setValue(".LINE", "1"); 
            c->setValue(".COL", "1"); 
            c->eval(contents); 
            c->setValue(".FILE", prevFile); 
            c->setValue(".LINE", prevLine); 
            c->setValue(".COL", prevCol); 

            vector<string> result; 
            return result; 
        }
    };

    class PrintFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        PrintFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            string out;
            for (size_t i = 1; i < arguments.size(); ++i)
            {   out += arguments[i];
                if (i + 1 < arguments.size())
                    out += ' ';
            }
            out += '\n';

            if (0 != jjm::writeToStdOut(out))
                throw std::runtime_error("Writing to stdout failed."); 
            if (0 != jjm::flushStdOut())
                throw std::runtime_error("Writing to stdout failed."); 

            vector<string> result; 
            return result; 
        }
    };

    class SetFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        SetFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() != 3)
                throw std::runtime_error("Function 'set' takes exactly 2 additional arguments."); 
            string const& name = arguments[1]; 
            string const& valueString = arguments[2]; 

            if (name.size() == 0)
                throw std::runtime_error("Function 'set' does not accept an empty variable name."); 
            if (name[0] == '.')
                throw std::runtime_error("Function 'set' will not set a variable whose name begins with a dot >>.<<."); 

            c->setValue(name, valueString); 

            vector<string> result; 
            return result; 
        }
    };

    class SetaFunction : public jjm::ParserContext::NativeFunction
    {
    public:
        SetaFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() <= 1)
                throw std::runtime_error("Function 'seta' takes 1 or more additional arguments."); 
            string const& name = arguments[1]; 
            vector<string> valueVec(arguments.begin() + 2, arguments.end()); 
            if (name.size() == 0)
                throw std::runtime_error("Function 'seta' does not accept an empty variable name."); 

            c->setValue(name, valueVec); 

            vector<string> result; 
            return result; 
        }
    };
}

void jjm::ParserContext::registerBuiltInFunctions()
{
    map<string, NativeFunction*> & r = getNativeFunctionRegistry();
    r["add"]     = new AddFunction; 
    r["eq"]      = new EqualsFunction; 
    r["equ"]     = new EqualsFunction; 
    r["get"]     = new GetFunction; 
    r["get@"]    = new GetAtFunction; 
    r["get*"]    = new GetStarFunction; 
    r["if"]      = new IfFunction; 
    r["include"] = new IncludeFunction; 
    r["neq"]     = new NotEqualsFunction; 
    r["print"]   = new PrintFunction; 
    r["set"]     = new SetFunction; 
    r["seta"]    = new SetaFunction; 
}

bool jjm::ParserContext::registerBuiltInFunctions2 = (jjm::ParserContext::registerBuiltInFunctions(), false); 
