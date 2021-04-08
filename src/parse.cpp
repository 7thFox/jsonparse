#include <functional>
#include <iostream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>
#include <algorithm>

#define ONE_INDENT "  "

using namespace std::string_literals; // enables s-suffix for std::string literals

const bool AllowTrailingCommas = false;

enum TokenKind
{
    LeftSQBracket,
    RightSQBracket,
    LeftCBracket,
    RightCBracket,
    Colon,
    Comma,

    DoubleQuote,
    Plus,
    Minus,
    Exp,
    Period,

    TrueLiteral,
    FalseLiteral,
    NullLiteral,

    String,
    Integer,
};

enum JTokenKind
{
    ObjectToken,
    ArrayToken,
    NumberToken,
    StringToken,
    LiteralToken,

    PropertyNameToken,// Not a real token, only for state machine reasons
    PropertyToken,// Technically by the JSON def, this isn't a token
};

class Token{
public:
    Token(TokenKind kind, std::string value){
        Kind = kind;
        StringValue = value;
    }
    std::string StringValue;
    TokenKind Kind;
};

class JToken
{
public:
    virtual void Print(std::string indent) = 0;
};

class JNumber : public JToken {
public:
    Token* LeadingSign;
    Token* Integer;
    Token* Period;
    Token* FractionalInteger;
    Token* Exponent;
    Token* ExponentSign;
    Token* ExponentInteger;

    JNumber(Token* value){
        // TODO
    }
    void Print(std::string indent)
    {
        // TODO
    }
};

class JString : public JToken {
public:
    Token *LeftQuote;
    Token *Value;
    Token *RightQuote;
    JString(Token *start, Token *value, Token *end)
    {
        LeftQuote = start;
        Value = value;
        RightQuote = end;
    }
    void Print(std::string indent) {
        std::cout
            << indent
            << LeftQuote->StringValue
            << Value->StringValue// TODO JOSH, parse escapes
            << RightQuote->StringValue
            << std::endl;
    }
};

class JLiteral : public JToken {
    Token *Value;
    JLiteral()
    {
        // TODO
    }
    void Print(std::string indent) {
        // TODO
    }
};

class JProperty : public JToken {
public:
    JString *NameString;
    Token* ColonToken;
    JToken *Value;
    Token* TrailingComma;
    JProperty(JString* name, Token* colon, JToken* value, Token* trailingComma){
        NameString = name;
        ColonToken = colon;
        Value = value;
        TrailingComma = trailingComma;
    }
    void Print(std::string indent) {
        std::cout << indent << "Property '" << NameString->Value->StringValue << "':" << std::endl;
        Value->Print(indent + ONE_INDENT);
    }
};

class JObject : public JToken {
public:
    Token *BeginToken;
    std::vector<JProperty *> *Properties;
    Token *EndToken;

    JObject(Token *begin, std::vector<JProperty*>* properties, Token* end){
        BeginToken = begin;
        Properties = properties;
        EndToken = end;
    }

    void Print(std::string indent) {
        std::cout << indent << "Object:" << std::endl;
        for (auto it = Properties->begin(); it != Properties->end(); ++it){
            (*it)->Print(indent + ONE_INDENT);
        }
    }
};

class JArray : public JToken {
public:
    Token *StartToken;
    std::vector<JToken *> *Values;
    Token *EndToken;
    JArray()
    {
        // TODO
    }
    void Print(std::string indent) {
        std::cout << indent << "Array:" << std::endl;
        // TODO
    }
};

struct ParseState
{
    ParseState(std::function<ParseState *(char)> nextstate)
    {
        this->nextstate = nextstate;
    }
    std::function<ParseState *(char)> nextstate;
    ParseState *operator()(char c)
    {
        auto next = nextstate(c);
        return next;
    }
};

std::stringstream token;
std::stack<Token *> tokens;
std::stack<JTokenKind> nodeKinds;
std::stack<JToken *> nodes;

void emit(TokenKind kind){
    tokens.push(new Token(kind, token.str()));
    token.str("");
    token.clear();
}

ParseState *beginToken();
ParseState *parseObjectPropertyOrEnd();
ParseState *parseObjectPropertyRequired();
ParseState *parseObjectEnd();
ParseState *parseString();
ParseState *parsePropertyValue();
ParseState *parseOptionalComma();
ParseState *parseInteger();

ParseState *pushNode();

// Terminal states
ParseState *eof();
ParseState *error(std::string);
ParseState *unexpectedInput(char c);
ParseState *ignoreInput();

// Helpers
ParseState *ignoreWhitespace(ParseState *);
ParseState *unpeek(ParseState *, char);

int main()
{
    auto jsonText = R"(
        {
            "test": "12345",
        }   )"s;

    auto state = ignoreWhitespace(beginToken());

    for (auto iter = jsonText.begin(); iter != jsonText.end(); ++iter)
    {
        // std::cout << (*iter) << std::endl;
        auto next = ((*state)(*iter));
        delete state;
        state = next;
    }
    while (!nodes.empty()){
        nodes.top()->Print("");
        delete nodes.top();
        nodes.pop();
    };

    while (!tokens.empty()){
        std::cout << "Token: " << tokens.top()->Kind << " Value: " << tokens.top()->StringValue << std::endl;
        delete tokens.top();
        tokens.pop();
    };

    return 0;
}

ParseState *beginToken(){
    return new ParseState([](char c) {
        if (std::isdigit(c)){
            nodeKinds.push(JTokenKind::NumberToken);
            token << c;
            return parseInteger();
        }
        switch (c) {
            case '{':
                nodeKinds.push(JTokenKind::ObjectToken);
                token << c;
                emit(TokenKind::LeftCBracket);
                return ignoreWhitespace(parseObjectPropertyOrEnd());
            case '[':
                break;
            case '"':
                nodeKinds.push(JTokenKind::StringToken);
                token << c;
                emit(TokenKind::DoubleQuote);
                return parseString();
            case 't':
                break;
            case 'f':
                break;
            case 'n':
                break;
            }
        return error("Expected beginning of token.");
    });
}

ParseState *parseObjectPropertyOrEnd(){
    return new ParseState([](char c) { 
        switch (c)
        {
        case '"':
            nodeKinds.push(JTokenKind::PropertyToken);
            nodeKinds.push(JTokenKind::PropertyNameToken);
            token << c;
            emit(TokenKind::DoubleQuote);
            return parseString();
        }
        return unpeek(parseObjectEnd(), c);// Or End
    });
}

ParseState *parseObjectPropertyRequired(){
    return new ParseState([](char c) { 
        switch (c)
        {
        case '"':
            nodeKinds.push(JTokenKind::PropertyToken);
            nodeKinds.push(JTokenKind::PropertyNameToken);
            token << c;
            emit(TokenKind::DoubleQuote);
            return parseString();
        }
        return unexpectedInput(c);
    });
}

ParseState *parseObjectEnd(){
    return new ParseState([](char c) { 
        switch (c)
        {
        case '}':
            token << c;
            emit(TokenKind::LeftCBracket);
            return pushNode();
        }
        return unexpectedInput(c);
    });
}

ParseState *parseString(){
    return new ParseState([](char c) {
        switch (c)
        {
        case '"':
            emit(TokenKind::String);
            token << c;
            emit(TokenKind::DoubleQuote);
            return pushNode();
        }
        token << c;
        return parseString();
    });
}

ParseState *parsePropertyValue() {
    return new ParseState([](char c) {
        if (c == ':')
        {
            token << c;
            emit(TokenKind::Colon);
            return ignoreWhitespace(beginToken());
        }

        return error("Expected :");
    });
}

ParseState *parseInteger() {// TODO: Parse Number
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseInteger();
        }
        emit(TokenKind::Integer);
        return unpeek(pushNode(), c);
    });
}

ParseState *pushNode(){
    auto kind = nodeKinds.top();
    nodeKinds.pop();
    bool hadTrailingComma = false;
    switch (kind)
    {
    // case JTokenKind::NumberToken:
    // {
    //     // nodes.push(new JToken(tokens.top()));
    //     // tokens.pop();
    //     break;
    // }
    case JTokenKind::ObjectToken:
    {
        auto end = tokens.top(); tokens.pop();
        auto begin = tokens.top(); tokens.pop();
        auto props = new std::vector<JProperty *>();
        while (!nodes.empty())
        {
            if (auto prop = dynamic_cast<JProperty *>(nodes.top())){
                props->push_back(prop);
                nodes.pop();
            }
            else {
                break;
            }
        }
        std::reverse(props->begin(), props->end());
        nodes.push(new JObject(begin, props, end));
        break;
    }
    case JTokenKind::StringToken:
    case JTokenKind::PropertyNameToken:
    {
        auto end = tokens.top(); tokens.pop();
        auto value = tokens.top(); tokens.pop();
        auto start = tokens.top(); tokens.pop();
        nodes.push(new JString(start, value, end));
        break;
    }
    case JTokenKind::PropertyToken:
    {
        Token *trailingComma = nullptr;
        if (tokens.top()->Kind == TokenKind::Comma){
            hadTrailingComma = true;
            trailingComma = tokens.top();
            tokens.pop();
        }
        auto value = nodes.top(); nodes.pop();
        auto colon = tokens.top(); tokens.pop();
        auto name = dynamic_cast<JString*>(nodes.top()); nodes.pop();
        nodes.push(new JProperty(name, colon, value, trailingComma));
        break;
    }
    default:
        std::cerr << "Could not push node " << kind << std::endl;
        return ignoreInput();
    }

    if (nodeKinds.empty()){
        return ignoreWhitespace(eof());
    }

    switch (nodeKinds.top()){
    case JTokenKind::ObjectToken:
        return ignoreWhitespace(hadTrailingComma 
            ? (AllowTrailingCommas ? parseObjectPropertyOrEnd() : parseObjectPropertyRequired())
            : parseObjectEnd());
    case JTokenKind::PropertyToken:
        return kind == JTokenKind::PropertyNameToken
            ? ignoreWhitespace(parsePropertyValue()) // Name parsed, needs value
            : ignoreWhitespace(parseOptionalComma());// Value parsed, but have not created the property
    }
    std::cerr << "Could not continue after node " << nodeKinds.top() << std::endl;
    return ignoreInput();
}

ParseState *parseOptionalComma(){
    return new ParseState([](char c) { 
        // either way, we're pushing the property, we're just getting the trailing comma first
        if (c == ',')
        {
            token << c;
            emit(TokenKind::Comma);
            return pushNode();
        }
        return unpeek(pushNode(), c);
    });
}

ParseState *eof(){
    return new ParseState([](char c) { return error("Expected end of file"); });
}

ParseState *error(std::string message){
    std::cerr << message << std::endl;
    return ignoreInput();
}

ParseState *unexpectedInput(char c){
    std::cerr << "Unexpected Character '" << c << "'" << std::endl;
    return ignoreInput();
}

ParseState *ignoreInput(){
    return new ParseState([](char c) { return ignoreInput(); });
}

ParseState *ignoreWhitespace(ParseState *state){
    return new ParseState([=](char c) {
        if (std::isspace(c))
        {
            return ignoreWhitespace(state);
        }
        return unpeek(state, c);
    });
}

ParseState *unpeek(ParseState * state, char c)
{
    auto next = (*state)(c);
    delete state;
    return next;
}