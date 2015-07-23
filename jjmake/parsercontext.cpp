#include "parsercontext.hpp"

#include "jjmakecontext.hpp"
#include "jbase/juniqueptr.hpp"
#include "jbase/jfatal.hpp"
#include "jbase/jstdint.hpp"
#include "jbase/jinttostring.hpp"

#include <deque>
#include <list>
#include <memory>
#include <map>
#include <vector>
#include <stdexcept>

using namespace jjm;
using namespace std;




namespace
{
    inline bool isLatinLetter(char c)
    {
        return (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z')); 
    }
}

class jjm::ParserContext::Evaluator
{
public:
    Evaluator(ParserContext * parserContext_) : parserContext(parserContext_) {}
    ParserContext * parserContext; 

    vector<string> eval(string const& text); 

    class NewlineConverter
    {
    public:
        NewlineConverter() : text(0), mline(0), mcol(0) {}
        NewlineConverter(string const& text_, size_t line_, size_t col_) 
            : text(&text_), iter(text->begin()), mline(line_), mcol(col_) {}
        bool hasNext() const { return iter != text->end(); }
        char next(); 
        size_t pos()  const { return iter - text->begin(); }
        size_t line() const { return mline; }
        size_t col()  const { return mcol; }
        void move(size_t newPos, size_t newLine, size_t newCol) 
            {   iter = text->begin() + newPos; 
                mline = newLine; 
                mcol = newCol; 
            }
    private:
        string const * text;
        string::const_iterator iter; 
        size_t mline;
        size_t mcol; 
    };
    NewlineConverter source; 

    class Frame
    {
    public:
        Frame() : 
                state(InvalidState), control(InvalidControl), 
                skipFunctionEvaluation(false), tryNextIfElifElse(false), 
                textFrame(0), hasPartialArgument(false) , 
                nativeFunction(0), 
                frameStartPos(static_cast<size_t>(-1)),
                frameStartLineNum(static_cast<size_t>(-1)),
                frameStartColNum(static_cast<size_t>(-1)),
                loopStartPos(static_cast<size_t>(-1)),
                loopStartLineNum(static_cast<size_t>(-1)),
                loopStartColNum(static_cast<size_t>(-1))
                {}

        enum State { FunctionState, ControlStatement, ControlBody, SingleQuoteState, DoubleQuoteState, CommentState, FinishState, InvalidState } state; 
        enum Control { If, Elif, Then, Else, While, Do, InvalidControl } control; 
        bool skipFunctionEvaluation; 
        bool tryNextIfElifElse; 

        Frame * textFrame; 
        bool hasPartialArgument; 
        string partialArgument; 
        vector<string> arguments; 

        NativeFunction * nativeFunction; 

        size_t frameStartPos; 
        size_t frameStartLineNum; 
        size_t frameStartColNum; 

        size_t loopStartPos; 
        size_t loopStartLineNum; 
        size_t loopStartColNum; 
    };
    list<Frame> frames; 

    void functionFrame(); 
    void controlStatementFrame(); 
    void controlBodyFrame(); 
    void singleQuoteFrame(); 
    void doubleQuoteFrame(); 
    void commentFrame(); 

    static map<string, void (Evaluator::*)()> getControlHandlers(); 
    static bool forceInitControlHandlers; 
    void ifControl();
    void elifControl();
    void thenControl(); 
    void elseControl();
    void fiControl();
    void whileControl();
    void doControl();
    void doneControl();

    void addFrame(Frame::State state); 
    void append(); //start or continue an argument, but don't add any char, 
    void append(char c); //start or continue an argument, and append the char
    void appendTake(string & str); //start or continue an argument, and append the str, may steal the contents of the arg
    void argumentDelimiter(); 
    void throwExceptionMissingExpected(string const& unexpectedSubmessage); 
};

inline char jjm::ParserContext::Evaluator::NewlineConverter::next()
{
    if (iter == text->end())
        JFATAL(0, 0); 

    char c = *iter; 
    ++iter; 
    ++mcol; 

    if (c == '\r')
    {   ++mline;
        mcol = 1; 
        
        if (iter == text->end())
            return '\n';

        c = *iter; 
        ++iter; 

        if (c == '\n')
            return '\n';

        --iter;
        return '\n'; 
    }

    if (c == '\n')
    {   ++mline;
        mcol = 1;
        return '\n'; 
    }

    return c;
}

void jjm::ParserContext::Evaluator::addFrame(Frame::State state)
{
    frames.push_back(Frame()); 
    Frame & f = frames.back(); 
    f.state = state; 
    f.textFrame = & f; 
    if (frames.size() > 1)
    {   Frame * prevFrame    = & * frames.end().operator--().operator--(); 
        f.skipFunctionEvaluation = prevFrame->skipFunctionEvaluation; 
    }
    f.frameStartPos = source.pos();
    f.frameStartLineNum = source.line();
    f.frameStartColNum = source.col(); 
}

inline void jjm::ParserContext::Evaluator::append()
{
    frames.back().textFrame->hasPartialArgument = true; 
}

inline void jjm::ParserContext::Evaluator::append(char c)
{
    frames.back().textFrame->hasPartialArgument = true; 
    frames.back().textFrame->partialArgument += c; 
}

inline void jjm::ParserContext::Evaluator::appendTake(string & str)
{
    if (frames.back().textFrame->hasPartialArgument && frames.back().textFrame->partialArgument.size() > 0)
    {   frames.back().textFrame->partialArgument += str; 
    }else
    {   frames.back().textFrame->hasPartialArgument = true; 
        frames.back().textFrame->partialArgument.swap(str); 
    }
    str.clear(); 
}

inline void jjm::ParserContext::Evaluator::argumentDelimiter()
{
    Frame & frame = frames.back(); 

    if (frame.textFrame->hasPartialArgument)
    {   frame.textFrame->arguments.push_back(string());
        frame.textFrame->arguments.back().swap(frame.textFrame->partialArgument);
        frame.textFrame->hasPartialArgument = false; 

        if (frames.size() > 1)
        {   if (frame.state == Frame::FunctionState && frame.arguments.size() == 1)
            {   map<string, jjm::ParserContext::NativeFunction*> const & r = jjm::ParserContext::getNativeFunctionRegistry();
                map<string, jjm::ParserContext::NativeFunction*>::const_iterator func = r.find(frame.arguments[0]); 
                if (func == r.end())
                    throw std::runtime_error("Unknown function >>(" + frame.arguments[0] + " ...)<<."); 
                frame.nativeFunction = func->second; 
            }

            if (frame.state == Frame::FunctionState)
            {   Frame & currentFrame = * frames.end().operator--(); 
                Frame & prevFrame    = * frames.end().operator--().operator--(); 
                currentFrame.skipFunctionEvaluation = prevFrame.skipFunctionEvaluation; 
                if (currentFrame.skipFunctionEvaluation == false
                        && currentFrame.nativeFunction->alwaysEvalArguments == false
                        && currentFrame.nativeFunction->evalNextArgument(parserContext, currentFrame.arguments) == false)
                {   currentFrame.skipFunctionEvaluation = true; 
                }
            }
        }
    }
}

map<string, void (jjm::ParserContext::Evaluator::*)()> jjm::ParserContext::Evaluator::getControlHandlers()
{
    static map<string, void (jjm::ParserContext::Evaluator::*)()> * x = 0;
    if (x == 0)
    {   x = new map<string, void (jjm::ParserContext::Evaluator::*)()>;
        (*x)["if"] = & ifControl;
        (*x)["elif"] = & elifControl;
        (*x)["then"] = & thenControl;
        (*x)["else"] = & elseControl;
        (*x)["fi"] = & fiControl;
        (*x)["while"] = & whileControl;
        (*x)["do"] = & doControl;
        (*x)["done"] = & doneControl;
    }
    return *x; 
}
bool jjm::ParserContext::Evaluator::forceInitControlHandlers = (jjm::ParserContext::Evaluator::getControlHandlers(), false); 

vector<string> jjm::ParserContext::Evaluator::eval(string const& text)
{
    vector<string> result;

    string file; 
    jjm::ParserContext::Value const * const fileValueClass = parserContext->getValue(".FILE");
    if (fileValueClass && fileValueClass->value.size())
        file = fileValueClass->value[0]; 

    size_t lineIntegerValue = 1; 
    jjm::ParserContext::Value const * const lineValueClass = parserContext->getValue(".LINE");
    if (lineValueClass && lineValueClass->value.size())
        decStrToInteger(lineIntegerValue, lineValueClass->value[0]); 

    size_t columnIntegerValue = 1; 
    jjm::ParserContext::Value const * const columnValueClass = parserContext->getValue(".COL");
    if (columnValueClass && columnValueClass->value.size())
        decStrToInteger(columnIntegerValue, columnValueClass->value[0]); 

    source = NewlineConverter(text, lineIntegerValue, columnIntegerValue); 

    try 
    {   addFrame(Frame::FunctionState);
        for (;;)
        {   if (frames.size() == 0)
                JFATAL(0, 0);
            switch (frames.back().state)
            {
            case Frame::FunctionState: functionFrame(); break;
            case Frame::ControlStatement: controlStatementFrame(); break; 
            case Frame::ControlBody: controlBodyFrame(); break; 
            case Frame::SingleQuoteState: singleQuoteFrame(); break; 
            case Frame::DoubleQuoteState: doubleQuoteFrame(); break; 
            case Frame::CommentState: commentFrame(); break; 
            case Frame::FinishState: result = frames.back().arguments; return result;
            default: JFATAL(0, 0); 
            }
        }
    }
    catch (std::exception & e)
    {   string message;
        message += "Evaluation failure at ";
        if (file.size() > 0)
            message += "file \"" + file + "\", "; 
        message += "line " + toDecStr(source.line()) + ", column " + toDecStr(source.col()) + ". ";
        message += "Cause:\n";
        message += e.what(); 
        throw std::runtime_error(message); 
    }
}

void jjm::ParserContext::Evaluator::functionFrame()
{
    for (;;)
    {   if ( ! source.hasNext()) 
        {   if (frames.size() == 1)
            {   argumentDelimiter(); 
                frames.back().state = Frame::FinishState; 
                return; 
            }
            throwExceptionMissingExpected("Unexpected end-of-text."); 
        }
        char const c = source.next(); 
        switch (c)
        {
        case '(': 
            if (frames.size() > 1 && frames.back().arguments.size() == 0)
                throw std::runtime_error("Nested Function calls are not allowed in the name of a function call.");
            append(); //starting a function counts as an argument, even if it's empty
            addFrame(Frame::FunctionState); 
            return; 
        case ')': 
            {
                argumentDelimiter(); 
                Frame const & f = frames.back(); 
                if (f.arguments.size() == 0)
                    throw std::runtime_error("Unexpected close-paren >>)<<. Missing function name."); 
                if (f.nativeFunction == 0)
                    JFATAL(0, 0); 
                
                //We actually don't care about the value of skipFunctionEvaluation
                //for this frame. 
                //Skip might have been specified by the function itself for its arguments. 
                //For determining if we want to evaluate the function itself, 
                //ask the previous frame. 
                if (frames.size() > 1)
                    frames.back().skipFunctionEvaluation = frames.end().operator--().operator--()->skipFunctionEvaluation; 
                if (f.skipFunctionEvaluation == false)
                {   size_t line = source.line();
                    size_t col = source.col(); 
                    parserContext->setValue(".LINE", toDecStr(line));
                    parserContext->setValue(".COL", toDecStr(col));
                    
                    vector<string> result = (*f.nativeFunction).eval(parserContext, f.arguments);
                    if (frames.size() < 2)
                        JFATAL(0, 0); 
                    frames.back().textFrame = frames.end().operator--().operator--()->textFrame; 
                    for (size_t i = 0; i < result.size(); ++i)
                    {   appendTake(result[i]); 
                        if (i + 1 < result.size())
                            argumentDelimiter(); 
                    }
                }
                frames.pop_back(); 
                return; 
            }
        case '[': 
            addFrame(Frame::ControlStatement); 
            return; 
        case ']': 
            if (frames.size() == 1)
                throw std::runtime_error("Unexpected close-bracket >>]<<."); 
            throwExceptionMissingExpected("Unexpected close-bracket >>]<<."); 
        case '#': 
            argumentDelimiter(); 
            addFrame(Frame::CommentState); 
            return; 
        case '\'': 
            addFrame(Frame::SingleQuoteState); 
            return; 
        case '\"': 
            addFrame(Frame::DoubleQuoteState); 
            return; 
        case ' ': case '\t': case '\n': 
            argumentDelimiter(); 
            break; 
        default: 
            append(c); 
            break; 
        }
    }
}

void jjm::ParserContext::Evaluator::controlStatementFrame()
{
    //pop the fake control statement frame, 
    //the [if] and [while] handlers will add new ControlBody frames, 
    //and the handlers for the other control statements will modify an existing ControlBody frame
    frames.pop_back(); 

    string controlName;
    for (;;)
    {   if ( ! source.hasNext())
            throwExceptionMissingExpected("Unexpected end-of-text."); 
        char c = source.next(); 
        if (c == ']')
            break; 
        if ( ! isLatinLetter(c))
            throw std::runtime_error("Invalid control statement >>[" + controlName + "...]<<."); 
        controlName += c; 
        if (controlName.size() >= 6)
            throw std::runtime_error("Invalid control statement >>[" + controlName + "...]<<."); 
    }
    map<string, void (jjm::ParserContext::Evaluator::*)()> const& m = getControlHandlers(); 
    map<string, void (jjm::ParserContext::Evaluator::*)()>::const_iterator c = m.find(controlName);
    if (c == m.end())
        throw std::runtime_error("Unknown control statement >>[" + controlName + "]<<."); 

    (this->*(c->second))(); 
    return; 
}

void jjm::ParserContext::Evaluator::controlBodyFrame()
{
    for (;;)
    {   if ( ! source.hasNext()) 
            throwExceptionMissingExpected("Unexpected end-of-text."); 
        char const c = source.next(); 
        switch (c)
        {
        case '(': 
            append(); //starting a function counts as an argument, even if it's empty
            addFrame(Frame::FunctionState); 
            return; 
        case ')': 
            throwExceptionMissingExpected("Unexpected close-paren >>)<<."); 
        case '[': 
            addFrame(Frame::ControlStatement); 
            return; 
        case ']': 
            throwExceptionMissingExpected("Unexpected close-bracket >>]<<."); 
        case '#': 
            switch (frames.back().textFrame->state)
            {
            case Frame::FunctionState: argumentDelimiter(); addFrame(Frame::CommentState); return; 
            case Frame::ControlBody:   argumentDelimiter(); addFrame(Frame::CommentState); return; 
            case Frame::SingleQuoteState: append(c); break; 
            case Frame::DoubleQuoteState: append(c); break; 
            default: JFATAL(0, 0); 
            }
            break;
        case '\'': 
            addFrame(Frame::SingleQuoteState); 
            return; 
        case '\"': 
            addFrame(Frame::DoubleQuoteState); 
            return; 
        case ' ': case '\t': case '\n': 
            switch (frames.back().textFrame->state)
            {
            case Frame::FunctionState:    argumentDelimiter(); break; 
            case Frame::ControlBody:      append(c); break; 
            case Frame::SingleQuoteState: append(c); break; 
            case Frame::DoubleQuoteState: append(c); break; 
            default: JFATAL(0, 0); 
            }
            break;
        default: 
            append(c); 
            break; 
        }
    }
}

void jjm::ParserContext::Evaluator::singleQuoteFrame()
{
    for (;;)
    {   if ( ! source.hasNext()) 
            throwExceptionMissingExpected("Unexpected end-of-text."); 
        char const c = source.next(); 
        if (c == '\n')
            throwExceptionMissingExpected("Newlines are not allowed in single-quote region."); 
        if (c == '\'')
        {   if (frames.back().arguments.size() > 0)
                JFATAL(0, 0); 
            if (frames.size() < 2)
                JFATAL(0, 0); 
            frames.back().textFrame = frames.end().operator--().operator--()->textFrame; 
            appendTake(frames.back().partialArgument); //unconditionally append to make >>''<< count as an argument, an empty argument
            frames.pop_back();
            return; 
        }
        append(c); 
    }
}

void jjm::ParserContext::Evaluator::doubleQuoteFrame()
{
    for (;;)
    {   if ( ! source.hasNext()) 
            throwExceptionMissingExpected("Unexpected end-of-text."); 
        char const c = source.next(); 
        switch (c)
        {
        case '(': 
            append(); //starting a function counts as an argument, even if it's empty
            addFrame(Frame::FunctionState); 
            return; 
        case ')': 
            throwExceptionMissingExpected("Unexpected close-paren >>)<<."); 
        case '[': 
            addFrame(Frame::ControlStatement); 
            return; 
        case ']': 
            throwExceptionMissingExpected("Unexpected close-bracket >>]<<."); 
        case '#': 
            append(c); 
            break; 
        case '\'': 
            addFrame(Frame::SingleQuoteState); 
            return; 
        case '\"': 
            if (frames.back().arguments.size() > 0)
                JFATAL(0, 0); 
            if (frames.size() < 2)
                JFATAL(0, 0); 
            frames.back().textFrame = frames.end().operator--().operator--()->textFrame; 
            appendTake(frames.back().partialArgument); //unconditionally append to make >>""<< count as an argument, an empty argument
            frames.pop_back();
            return; 
        case ' ': case '\t': 
            append(c); 
            break; 
        case '\n': 
            throwExceptionMissingExpected("Newlines are not allowed in double-quote region."); 
        default: 
            append(c); 
            break; 
        }
    }
}

void jjm::ParserContext::Evaluator::commentFrame()
{
    for (;;)
    {   if ( ! source.hasNext()) 
            return; 
        char c = source.next();
        if (c == '\n')
            return;
    }
}

void jjm::ParserContext::Evaluator::ifControl()
{
    addFrame(Frame::ControlBody);
    Frame & f = frames.back(); 
    f.control = Frame::If; 

    //if we're in normal-eval mode, then try to take one of the if-elif-else branches
    if (f.skipFunctionEvaluation == false)
        f.tryNextIfElifElse = true;         
}

void jjm::ParserContext::Evaluator::elifControl()
{
    Frame & f = frames.back(); 
    if (f.state != Frame::ControlBody || f.control != Frame::Then)
        throwExceptionMissingExpected("Unexpected >>[elif]<<."); 

    f.control = Frame::Elif; 
    f.frameStartPos = source.pos(); 
    f.frameStartLineNum = source.line();
    f.frameStartColNum = source.col(); 

    f.textFrame = & f; 

    f.hasPartialArgument = false;
    f.partialArgument.clear(); 
    f.arguments.clear(); 

    if (f.skipFunctionEvaluation == false)
    {   //This means we just finished taking one if-elif-else branch for this frame.
        //So, don't take another if-elif-else branch for this frame. 
        f.skipFunctionEvaluation = true;
        f.tryNextIfElifElse = false;
        return; 
    }

    if (f.tryNextIfElifElse)
    {   f.skipFunctionEvaluation = false;
        return; 
    }
}

void jjm::ParserContext::Evaluator::thenControl()
{
    Frame & f = frames.back(); 
    if (f.state != Frame::ControlBody)
        throwExceptionMissingExpected("Unexpected >>[then]<<."); 
    if (f.control != Frame::If && f.control != Frame::Elif)
        throwExceptionMissingExpected("Unexpected >>[then]<<."); 

    f.control = Frame::Then; 
    f.frameStartPos = source.pos(); 
    f.frameStartLineNum = source.line();
    f.frameStartColNum = source.col(); 

    f.textFrame = & f; 
    
    if (f.skipFunctionEvaluation)
    {   //if we're not in normal-eval mode, then we're skipping this branch
        return; 
    }

    argumentDelimiter(); 
    if (f.arguments.size() > 1)
        JFATAL(0, 0); 
    bool const ifElifThenCondition = f.arguments.size() == 1 && f.arguments[0].size() > 0; 

    if (ifElifThenCondition)
    {   f.skipFunctionEvaluation = false;
        f.tryNextIfElifElse = false; 
        if (frames.size() < 2)
            JFATAL(0, 0);
        f.textFrame = frames.end().operator--().operator--()->textFrame; 
    }else
    {   f.skipFunctionEvaluation = true;
    }
}

void jjm::ParserContext::Evaluator::elseControl()
{
    Frame & f = frames.back(); 
    if (f.state != Frame::ControlBody || f.control != Frame::Then)
        throwExceptionMissingExpected("Unexpected >>[else]<<."); 

    f.control = Frame::Else; 
    f.frameStartPos = source.pos(); 
    f.frameStartLineNum = source.line();
    f.frameStartColNum = source.col(); 

    f.textFrame = & f; 

    if (f.skipFunctionEvaluation == false && f.tryNextIfElifElse)
    {   //should never be in the state where: 
        //1- we've reached an [else], and 
        //2- we're in normal-eval mode, and 
        //3- we're supposed to take the else
        JFATAL(0, 0); 
    }

    if (f.skipFunctionEvaluation == false)
    {   //if we're in normal-eval mode when we hit an [else], 
        //that means we should skip this else-body
        f.skipFunctionEvaluation = true;
        f.tryNextIfElifElse = false; 
        f.textFrame = & f; 
    }

    if (f.tryNextIfElifElse)
    {   //if we hit an [else] when this flag is true, 
        //it means we should take the else-body
        f.skipFunctionEvaluation = false; 
        f.tryNextIfElifElse = false;
        if (frames.size() < 2)
            JFATAL(0, 0);
        f.textFrame = frames.end().operator--().operator--()->textFrame; 
        return; 
    }
}

void jjm::ParserContext::Evaluator::fiControl()
{
    Frame & f = frames.back(); 
    if (f.state != Frame::ControlBody)
        throwExceptionMissingExpected("Unexpected >>[fi]<<."); 
    if (f.control != Frame::Then && f.control != Frame::Else)
        throwExceptionMissingExpected("Unexpected >>[fi]<<."); 
    frames.pop_back(); 
}

void jjm::ParserContext::Evaluator::whileControl()
{
    addFrame(Frame::ControlBody);
    Frame & f = frames.back(); 
    f.control = Frame::While; 
    f.loopStartPos = source.pos(); 
    f.loopStartLineNum = source.line(); 
    f.loopStartColNum = source.col(); 
}

void jjm::ParserContext::Evaluator::doControl()
{
    Frame & f = frames.back(); 
    if (f.state != Frame::ControlBody || f.control != Frame::While)
        throwExceptionMissingExpected("Unexpected >>[do]<<."); 
    
    f.textFrame = & f; 

    f.control = Frame::Do; 
    f.frameStartPos = source.pos(); 
    f.frameStartLineNum = source.line();
    f.frameStartColNum = source.col(); 
    
    if (f.skipFunctionEvaluation)
    {   //if we're not in normal-eval mode, then we're skipping this loop
        return; 
    }

    argumentDelimiter(); 
    if (f.arguments.size() > 1)
        JFATAL(0, 0); 
    bool const whileDoCondition = f.arguments.size() == 1 && f.arguments[0].size() > 0;
    
    if (whileDoCondition)
    {   if (frames.size() < 2)
            JFATAL(0, 0);
        f.textFrame = frames.end().operator--().operator--()->textFrame; 
    }else
    {   f.skipFunctionEvaluation = true;
    }
}

void jjm::ParserContext::Evaluator::doneControl()
{
    Frame & f = frames.back(); 
    if (f.state != Frame::ControlBody || f.control != Frame::Do)
        throwExceptionMissingExpected("Unexpected >>[done]<<."); 
    
    f.textFrame = & f; 

    if (f.skipFunctionEvaluation)
    {   frames.pop_back(); 
        return;
    }
    
    //so, do the loop again
    source.move(f.loopStartPos, f.loopStartLineNum, f.loopStartColNum); 

    f.control = Frame::While; 
    f.frameStartPos = source.pos(); 
    f.frameStartLineNum = source.line();
    f.frameStartColNum = source.col(); 

    f.hasPartialArgument = false;
    f.partialArgument.clear();
    f.arguments.clear(); 
}


void jjm::ParserContext::Evaluator::throwExceptionMissingExpected(string const& unexpectedSubmessage)
{
    string file;
    jjm::ParserContext::Value const* fileValue = parserContext->getValue(".FILE"); 
    if (fileValue && fileValue->value.size())
        file = fileValue->value[0]; 

    string message; 
    message += unexpectedSubmessage; 
    if (message.size() && message[message.size() - 1] != ' ')
        message += ' ';
    switch (frames.back().state)
    {
    case Frame::FunctionState:    
        message += "Missing expected close-paren >>)<< to match open-paren >>(<< at ";
        break; 
    case Frame::ControlStatement: 
        JFATAL(0, 0); 
    case Frame::ControlBody:
        switch (frames.back().control)
        {
        case Frame::If:    message += "Missing expected >>[then]<< to match >>[if]<< at "; break; 
        case Frame::Elif:  message += "Missing expected >>[then]<< to match >>[elif]<< at "; break; 
        case Frame::Then:  message += "Missing expected >>[elif]<< or >>[else]<< or >>[fi]<< to match >>[then]<< at "; break; 
        case Frame::While: message += "Missing expected >>[do]<< to match >>[while]<< at "; break; 
        case Frame::Do:    message += "Missing expected >>[done]<< to match >>[do]<< at "; break; 
        default: JFATAL(0, 0); 
        }
        break; 
    case Frame::SingleQuoteState: 
        message += "Missing expected single-quote >>'<< to match single-quote >>'<< at ";
        break; 
    case Frame::DoubleQuoteState: 
        message += "Missing expected double-quote >>\"<< to match double-quote >>\"<< at ";
        break; 
    default: JFATAL(0, 0); 
    }

    if (file.size())
        message += "file \"" + file + "\", ";
    message += "line " + toDecStr(frames.back().frameStartLineNum) + ", ";
    message += "column " + toDecStr(frames.back().frameStartColNum) + ".";

    throw std::runtime_error(message); 
}


map<string, jjm::ParserContext::NativeFunction*> & jjm::ParserContext::getNativeFunctionRegistry()
{
    static map<string, jjm::ParserContext::NativeFunction*> * x = 0; 
    if (x == 0)
        x = new map<string, jjm::ParserContext::NativeFunction*>; 
    return *x; 
}
bool jjm::ParserContext::initNativeFunctionRegistry = (jjm::ParserContext::getNativeFunctionRegistry(), false); 


void jjm::ParserContext::registerNativeFunction(std::string const& name, NativeFunction* nativeFunction)
{
    map<string, jjm::ParserContext::NativeFunction*> & r = getNativeFunctionRegistry(); 
    NativeFunction* & f = r[name];
    if (f != 0)
        JFATAL(0, 0);
    f = nativeFunction; 
}

jjm::ParserContext::ParserContext()
{
    jjmakeContext = 0; 
    parent = 0; 
}

jjm::ParserContext::~ParserContext()
{
    for (size_t i=0; i < owned.size(); ++i)
        delete owned[i]; 
}

jjm::ParserContext* jjm::ParserContext::newRoot(JjmakeContext * jjmakeContext_)
{
    ParserContext * c = new ParserContext; 
    c->jjmakeContext = jjmakeContext_;
    return c;
}

void jjm::ParserContext::newNode(jjm::Node * node_)
{
    jjmakeContext->newNode(node_); 
}


jjm::ParserContext* jjm::ParserContext::split()
{
    //optimization, if we're empty (no variable definitions), then ewe can use
    //this's parent as the child's parent instead of using this as the child's
    //parent. This allows us to keep the depth of the context parent chain 
    //small in important use cases. 
    if (variables.size() == 0 && parent)
    {   owned.push_back(0); 
        ParserContext * newChild = owned.back() = new ParserContext; 
        newChild->parent = parent; 
        return newChild; 
    }

    owned.push_back(0); 
    ParserContext * newParent = owned.back() = new ParserContext; 
    owned.push_back(0); 
    ParserContext * newChild = owned.back() = new ParserContext; 

    newParent->parent = this->parent; 
    this->parent = newParent; 
    newChild->parent = newParent; 

    using std::swap;
    swap(this->variables, this->parent->variables); 

    return newChild; 
}

vector<string> jjm::ParserContext::eval(string const& text)
{
    Evaluator evaluator(this); 
    return evaluator.eval(text);
}

jjm::ParserContext::Value const * jjm::ParserContext::getValue(string const& name)
{
    for (ParserContext const * c = this; ; )
    {   if (c == 0)
            return 0; 
        map<string, Value>::const_iterator v = c->variables.find(name); 
        if (v != c->variables.end())
            return & v->second; 
        c = c->parent; 
    }
}

void jjm::ParserContext::setValue(string const& name, string const& valueString)
{
    Value const * file = getValue(".FILE");
    Value const * line = getValue(".LINE"); 
    Value & value = variables[name];
    value = Value();
    if (file && file->value.size() > 0)
        value.definitionFile = file->value[0];
    if (line && line->value.size() > 0)
        value.definitionFile = line->value[0];
    value.value.push_back(valueString); 
}

void jjm::ParserContext::setValue(string const& name, vector<string> const& valueVec)
{
    Value const * file = getValue(".FILE");
    Value const * line = getValue(".LINE"); 
    Value & value = variables[name];
    value = Value();
    if (file && file->value.size() > 0)
        value.definitionFile = file->value[0];
    if (line && line->value.size() > 0)
        value.definitionFile = line->value[0];
    value.value = valueVec; 
}
