// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#include "parsercontext.hpp"

#include "node.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jinttostring.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/juniqueptr.hpp"
#include "josutils/jfilehandle.hpp"
#include "josutils/jfilestreams.hpp"
#include "josutils/jopen.hpp"
#include "josutils/jpath.hpp"
#include "josutils/jstat.hpp"
#include "josutils/jstdstreams.hpp"
#include <errno.h>
#include <fstream>
#include <vector>

#ifdef _WIN32
    #include <Windows.h>
#endif

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
                throw std::runtime_error("Function '" + arguments[0] + "' takes 1 or more additional arguments."); 
            std::int64_t result = 0; 
            for (size_t i = 1; i < arguments.size(); ++i)
            {   std::int64_t x; 
                if (false == jjm::decStrToInteger(x, arguments[i]))
                    throw std::runtime_error("Function '" + arguments[0] + "' was given non-numeric argument \"" + arguments[i] + "\"."); 
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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 2 additional arguments."); 
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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 2 additional arguments."); 
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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 1 additional argument."); 
            string const& name = arguments[1]; 
            if (name.size() == 0)
                throw std::runtime_error("Function '" + arguments[0] + "' does not accept an empty variable name."); 

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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 1 additional argument."); 
            string const& name = arguments[1]; 
            if (name.size() == 0)
                throw std::runtime_error("Function '" + arguments[0] + "' does not accept an empty variable name."); 

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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 1 additional argument."); 
            string const& name = arguments[1]; 
            if (name.size() == 0)
                throw std::runtime_error("Function '" + arguments[0] + "' does not accept an empty variable name."); 

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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 2 or 3 additional arguments."); 
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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 1 additional argument.");
            
            string prevDotPwd;
            jjm::ParserContext::Value const* prevDotPwdClass = c->getValue(".PWD");
            if (prevDotPwdClass && prevDotPwdClass->value.size())
                prevDotPwd = prevDotPwdClass->value[0]; 

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

            Path const path = Path::join(Path(prevDotPwd), Path(arguments[1]).getAbsolutePath()); 

            //TODO need to use current locale for POSIX multi-byte string
            FileHandleOwner file;
            try
            {   file.reset(FileOpener().readOnly().openExistingOnly().open(path));
            }catch (std::exception & e)
            {   string message;
                message += string() + "Function 'include' unable to open file \"" + path.getStringRep() + "\". Cause:\n"; 
                message += e.what(); 
                throw std::runtime_error(message); 
            }
            FileStream inputStream(file.release()); 
            BufferedInputStream in( & inputStream); 

            string contents; 
            for (;;)
            {   size_t oldSize = contents.size();
                size_t fetchSize = 16 * 1024; 
                contents.resize(oldSize + fetchSize); 

                //Assumes std::string uses contiguous storage. 
                //Not gauranteed by C++03. It is guaranteed by C++11. 
                //True of all commercial implementations. 
                in.read(&contents[0] + oldSize, fetchSize); 
#ifdef _WIN32
                DWORD const lastError = GetLastError(); 
#else
                int const lastErrno = errno; 
#endif

                ssize_t gcount = in.gcount(); 
                contents.resize(oldSize + gcount); 
                if (in)
                    continue;
                if (in.isEof())
                    break;
#ifdef _WIN32
                throw std::runtime_error(string() 
                        + "Function 'include': Failure when reading from file \"" + path.getStringRep() + "\". "
                        + "GetLastError() " + toDecStr(lastError) + "."); 
#else
                throw std::runtime_error(string() 
                        + "Function 'include': Failure when reading from file \"" + path.getStringRep() + "\". "
                        + "errno " + toDecStr(lastErrno) + "."); 
#endif
            }

            c->setValue(".PWD", path.getParent().getStringRep()); 
            c->setValue(".FILE", path.getStringRep()); 
            c->setValue(".LINE", "1"); 
            c->setValue(".COL", "1");  

            c->eval(contents); 

            c->setValue(".PWD", prevDotPwd); 
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
            c->toStdOut(out); 

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
                throw std::runtime_error("Function '" + arguments[0] + "' takes exactly 2 additional arguments."); 
            string const& name = arguments[1]; 
            string const& valueString = arguments[2]; 

            if (name.size() == 0)
                throw std::runtime_error("Function '" + arguments[0] + "' does not accept an empty variable name."); 
            if (name[0] == '.')
                throw std::runtime_error("Function '" + arguments[0] + "' will not set a variable whose name begins with a dot >>.<<."); 

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
                throw std::runtime_error("Function '" + arguments[0] + "' takes 1 or more additional arguments."); 
            string const& name = arguments[1]; 
            vector<string> valueVec(arguments.begin() + 2, arguments.end()); 
            if (name.size() == 0)
                throw std::runtime_error("Function '" + arguments[0] + "' does not accept an empty variable name."); 

            c->setValue(name, valueVec); 

            vector<string> result; 
            return result; 
        }
    };

    class TouchNode : public jjm::Node
    {
    public:
        TouchNode(Path const& path_, vector<Path> const& inputPaths_, vector<Path> const& outputPaths_)
            : Node(path_.getStringRep(), inputPaths_, outputPaths_),
            targetPath(path_)
            {}
        Path targetPath; 
        Stat targetStat; 
        virtual void execute()
        {   
            targetStat = Stat::stat(targetPath); 

            if (targetStat.type == FileType::NoExist)
            {   touchFile(); 
                return; 
            }
            if (targetStat.type == FileType::RegularFile)
            {   if (hasNewerDependency())
                    touchFile(); 
                return; 
            }
            if (targetStat.type == FileType::Directory)
                throw std::runtime_error("Cannot create regular file \"" + targetPath.getStringRep() + "\" because a directory exists that has the same name."); 
            if (targetStat.type == FileType::Symlink)
                JFATAL(targetStat.type.toEnum(), targetPath.getStringRep()); 
            if (targetStat.type == FileType::Other)
                throw std::runtime_error("Cannot create regular file \"" + targetPath.getStringRep() + "\" because a file of an unusual type exists that has the same name."); 
            JFATAL(targetStat.type.toEnum(), targetPath.getStringRep()); 
        }
        void touchFile()
        {
            FileHandleOwner file(FileOpener().createOrOpen().readWrite().open(targetPath)); 
            //TODO set timestamp
        }
        bool hasNewerDependency()
        {
            return true; 
        }
    }; 

    class TouchNodeFunction : public jjm::ParserContext::NativeFunction
    {
    public: 
        TouchNodeFunction() {}
        virtual vector<string> eval(ParserContext * c, vector<std::string> const& arguments) 
        {
            if (arguments.size() < 2)
                throw std::runtime_error("Function '" + arguments[0] + "' takes 1 or more additional argument."); 

            //.PWD guaranteed to already be canonized via Path::getRealPath()
            ParserContext::Value const * pwdClass = c->getValue(".PWD"); 
            if (pwdClass == 0 || pwdClass->value.size() != 1)
                JFATAL(0, 0);
            Path pwdPath(pwdClass->value[0]); 
            if ( ! pwdPath.isAbsolute())
                JFATAL(0, 0); 
            
            vector<Path> inputPaths;
            for (size_t i = 2; i < arguments.size(); ++i)
                inputPaths.push_back(Path::join(pwdPath, Path(arguments[i]))); 

            vector<Path> outputPaths; 
            outputPaths.push_back(Path::join(pwdPath, Path(arguments[1]))); 

            UniquePtr<TouchNode*> node(new TouchNode(outputPaths[0], inputPaths, outputPaths)); 
            c->newNode(node.release()); 

            return vector<string>(); 
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
    r["touch-node"] = new TouchNodeFunction; 
}

bool jjm::ParserContext::registerBuiltInFunctions2 = (jjm::ParserContext::registerBuiltInFunctions(), false); 
