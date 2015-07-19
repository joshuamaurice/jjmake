// Copyright (c) 2010-2015, Informatica Corporation, Joshua Maurice
//       Distributed under the 3-clause BSD License
//      (See accompanying file LICENSE.TXT or copy at
//  http://www.w3.org/Consortium/Legal/2008/03-bsd-license.html)

#ifndef JJMAKE_PARSERCONTEXT_HPP_HEADER_GUARD
#define JJMAKE_PARSERCONTEXT_HPP_HEADER_GUARD

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace jjm
{

class ParserContext
{
public:
    static ParserContext* newRoot(); //caller owns return, must deallocate with delete 
    ~ParserContext(); 

    std::vector<std::string> eval(std::string const& text); 

    //This call has the following semantics. 
    //In effect, it creates two new ParserContext objects. 
    //      parent     := old_this
    //      new_this   := new
    //      return_val := new
    //This allows safe concurrent modification to the new_this and return_val. 
    //
    //The caller does not have any responsibility to deallocate any of these
    //new ParserContexts directly. Calling delete on the root will deallocate
    //all of these created ParserContexts. 
    ParserContext* split(); 

public:
    class Value
    {
    public:
        std::string definitionFile; 
        std::string definitionLine; 
        std::vector<std::string> value; 
    };
    Value const * getValue(std::string const& name); //returns null for no match
    void setValue(std::string const& name, std::string const& value);
    void setValue(std::string const& name, std::vector<std::string> const& value);

public:
    class NativeFunction
    {
    public:
        virtual ~NativeFunction() {}

        //first argument is function name
        virtual std::vector<std::string> eval(ParserContext * , std::vector<std::string> const& arguments) = 0; 

        bool alwaysEvalArguments; 

        //If alwaysEvalArguments is true, this function is never called. 
        //First argument is function name. 
        virtual bool evalNextArgument(ParserContext * , std::vector<std::string> const& argumentsThusFar) { return true; }

    protected: 
        NativeFunction() : alwaysEvalArguments(true) {}

    private:
        NativeFunction(NativeFunction const& ); //not defined, not copyable
        NativeFunction& operator= (NativeFunction const& ); //not defined, not copyable
    };

    //Does not take ownership
    static void registerNativeFunction(std::string const& name, NativeFunction* nativeFunction); 

private:
    ParserContext(ParserContext const& ); //not defined, not copyable
    ParserContext& operator= (ParserContext const& ); //not defined, not copyable

    ParserContext(); 

    ParserContext* parent; 
    std::vector<ParserContext*> owned; 
    std::map<std::string, Value> variables; 

    class Evaluator; 

    static std::map<std::string, NativeFunction*> & getNativeFunctionRegistry(); 
    static bool initNativeFunctionRegistry; 

    static void registerBuiltInFunctions(); 
    static bool registerBuiltInFunctions2; 
};

}//namespace jjm

#endif
