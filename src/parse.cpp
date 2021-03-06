#include <functional>
#include <iostream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>
#include <queue>
#include <fstream>
#include <algorithm>
#include <chrono>

#define ONE_INDENT "  "

// using namespace std::string_literals; // enables s-suffix for std::string literals

const bool AllowTrailingCommas = true;
const bool AllowSuperfluousLeadingZeroes = false;

enum TokenKind
{
    LeftSQBracket,
    RightSQBracket,
    LeftCBracket,
    RightCBracket,
    Colon,
    Comma,

    DoubleQuote,
    Sign,
    Exp,
    DecimalPoint,

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

    PropertyToken,// Technically by the JSON def, these aren't tokens
    ArrayElementToken,

    PropertyNameToken,// Not a real token, only for state machine reasons
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
    virtual JTokenKind Kind() = 0;
    virtual void Print(std::string indent) = 0;
};

class JNumber : public JToken {
public:
    JTokenKind Kind() { return JTokenKind::NumberToken; }

    Token* LeadingSign;
    Token* Integer;
    Token* Period;
    Token* FractionalInteger;
    Token* Exponent;
    Token* ExponentSign;
    Token* ExponentInteger;

    JNumber(
        Token* leadingSign,
        Token* integer,
        Token* period,
        Token* fractionalInteger,
        Token* exponent,
        Token* exponentSign,
        Token* exponentInteger)
    {
        LeadingSign = leadingSign;
        Integer = integer;
        Period = period;
        FractionalInteger = fractionalInteger;
        Exponent = exponent;
        ExponentSign = exponentSign;
        ExponentInteger = exponentInteger;
    }
    
    ~JNumber() {
        if (LeadingSign != nullptr) delete LeadingSign;
        if (Integer != nullptr) delete Integer;
        if (Period != nullptr) delete Period;
        if (FractionalInteger != nullptr) delete FractionalInteger;
        if (Exponent != nullptr) delete Exponent;
        if (ExponentSign != nullptr) delete ExponentSign;
        if (ExponentInteger != nullptr) delete ExponentInteger;
    }
    
    void Print(std::string indent)
    {
        std::cout << indent;
        if (LeadingSign != nullptr)
            std::cout << LeadingSign->StringValue;
        std::cout << Integer->StringValue;
        if (Period != nullptr)
            std::cout << Period->StringValue
                      << FractionalInteger->StringValue;
        if (Exponent != nullptr) {
            std::cout << Exponent->StringValue;
            if (ExponentSign != nullptr)
                std::cout << ExponentSign->StringValue;
            std::cout << ExponentInteger->StringValue;
        }
        std::cout << std::endl;
    }
};

class JString : public JToken {
public:
    JTokenKind Kind() { return JTokenKind::StringToken; }

    Token *LeftQuote;
    Token *Value;
    Token *RightQuote;
    JString(Token *start, Token *value, Token *end)
    {
        LeftQuote = start;
        Value = value;
        RightQuote = end;
    }

    ~JString(){

        if (LeftQuote != nullptr) delete LeftQuote;
        if (Value != nullptr) delete Value;
        if (RightQuote != nullptr) delete RightQuote;
    }

    void Print(std::string indent) {
        std::cout
            << indent
            << LeftQuote->StringValue
            << Value->StringValue
            << RightQuote->StringValue
            << std::endl;
    }
};

class JLiteral : public JToken {
public:
    JTokenKind Kind() { return JTokenKind::LiteralToken; }

    Token *Value;
    JLiteral(Token *value)
    {
        Value = value;
    }

    ~JLiteral() {
        if (Value != nullptr) delete Value;
    }

    void Print(std::string indent) {
        std::cout << indent << Value->StringValue << std::endl;
    }
};

class JProperty : public JToken {
public:
    JTokenKind Kind() { return JTokenKind::PropertyToken; }

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

    ~JProperty(){
        if (NameString != nullptr) delete NameString;
        if (ColonToken != nullptr) delete ColonToken;
        if (Value != nullptr) delete Value;
        if (TrailingComma != nullptr) delete TrailingComma;
    }

    void Print(std::string indent) {
        std::cout << indent << "Property '" << NameString->Value->StringValue << "':" << std::endl;
        Value->Print(indent + ONE_INDENT);
    }
};

class JObject : public JToken {
public:
    JTokenKind Kind() { return JTokenKind::ObjectToken; }

    Token *BeginToken;
    std::vector<JProperty *> *Properties;
    Token *EndToken;

    JObject(Token *begin, std::vector<JProperty*>* properties, Token* end){
        BeginToken = begin;
        Properties = properties;
        EndToken = end;
    }

    ~JObject(){
        if (BeginToken != nullptr) delete BeginToken;
        if (EndToken != nullptr) delete EndToken;
        if (Properties != nullptr) {
            for (auto iter = Properties->begin(); iter != Properties->end(); ++iter) {
                if (*iter != nullptr) delete *iter;
            }
            delete Properties;
        }
    }

    void Print(std::string indent) {
        std::cout << indent << "Object:" << std::endl;
        for (auto it = Properties->begin(); it != Properties->end(); ++it){
            (*it)->Print(indent + ONE_INDENT);
        }
    }
};

class JArrayElement : public JToken {
public:
    JTokenKind Kind() { return JTokenKind::ArrayElementToken; }

    JToken *Value;
    Token *TrailingComma;
    JArrayElement(JToken *value, Token *trailingComma){
        Value = value;
        TrailingComma = trailingComma;
    }

    ~JArrayElement() {
        if (Value != nullptr) delete Value;
        if (TrailingComma != nullptr) delete TrailingComma;
    }

    void Print(std::string indent){
        Value->Print(indent);
    }
};

class JArray : public JToken
{
public:
    JTokenKind Kind() { return JTokenKind::ArrayToken; }

    Token *StartToken;
    std::vector<JArrayElement *> *Values;
    Token *EndToken;
    JArray(Token *start, std::vector<JArrayElement *> *values, Token *end)
    {
        StartToken = start;
        Values = values;
        EndToken = end;
    }

    ~JArray(){
        if (StartToken != nullptr) delete StartToken;
        if (EndToken != nullptr) delete EndToken;
        if (Values != nullptr) {
            for (auto iter = Values->begin(); iter != Values->end(); ++iter) {
                if (*iter != nullptr) delete *iter;
            }
            delete Values;
        }
    }

    void Print(std::string indent) {
        std::cout << indent << "Array:" << std::endl;
        for (auto it = Values->begin(); it != Values->end(); ++it) {
            (*it)->Print(indent + ONE_INDENT);
        }
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
ParseState *parseIntegerStart();
ParseState *parseInteger();
ParseState *parseOptionalDecimal();
ParseState *parseFractionalIntegerStart();
ParseState *parseFractionalInteger();
ParseState *parseOptionalExp();
ParseState *parseOptionalExpSign();
ParseState *parseExpIntegerStart();
ParseState *parseExpInteger();
ParseState *parseTokenOrArrayEnd();
ParseState *parseArrayEnd();

ParseState *pushNode();

// Terminal states
ParseState *eof();
ParseState *error(std::string);
ParseState *unexpectedInput(char c);
ParseState *expectedInput(std::string expectedMessage, char c);
ParseState *ignoreInput();

// Helpers
ParseState *ignoreWhitespace(ParseState *);
ParseState *unpeek(ParseState *, char);
ParseState *readLiteral(std::string sequence, TokenKind literalKind);

int main(int argc, char *argv[])
{
    bool noprint = false;
    bool bench = false;
    for (int i = 0; i < argc - 1; i++)
    {
        auto arg = std::string(argv[i]);
        if (arg == "-noprint") {
            noprint = true;
        }
        else if (arg == "-bench") {
            bench = true;
        }
    }

    if (argc < 1) {
        std::cerr << "Filename is required" << std::endl;
        return 1;
    }

    auto filename = std::string(argv[argc - 1]);
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Could not open the file - '"
             << filename << "'" << std::endl;
        return 1;
    }


    auto start = std::chrono::high_resolution_clock::now();
    auto state = ignoreWhitespace(beginToken());
    char c;
    while (file.get(c))
    {
        // std::cout << (*iter) << std::endl;
        auto next = ((*state)(c));
        delete state;
        state = next;
    }
    auto end = std::chrono::high_resolution_clock::now();
    file.close();

    if (nodes.size() > 1 || tokens.size() > 0) {
        std::cerr << "Unexpected EOF" << std::endl;
    }
    else if (!noprint && !nodes.empty()) {
        nodes.top()->Print("");
    }

    if (bench) {
        std::cout
            << "Parsing '" << filename << "' Completed in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << "ms."
            << std::endl;
    }

    while (!nodes.empty()){
        if (nodes.top() != nullptr) {
            // nodes.top()->Print("");
            delete nodes.top();
        }
        nodes.pop();
    };

    while (!tokens.empty()){
        if (tokens.top() != nullptr) {
            // std::cout << "Token: " << tokens.top()->Kind << " Value: " << tokens.top()->StringValue << std::endl;
            delete tokens.top();
        }
        tokens.pop();
    };

    return 0;
}

ParseState *beginToken(){
    return new ParseState([](char c) {
        if (std::isdigit(c)) {
            nodeKinds.push(JTokenKind::NumberToken);
            tokens.push(nullptr);// leading sign
            token << c;
            if (!AllowSuperfluousLeadingZeroes && c == '0') {
                emit(TokenKind::Integer);
                return parseOptionalDecimal();
            }
            return parseInteger();
        }
        switch (c) {
            case '{':
                nodeKinds.push(JTokenKind::ObjectToken);
                token << c;
                emit(TokenKind::LeftCBracket);
                return ignoreWhitespace(parseObjectPropertyOrEnd());
            case '[':
                nodeKinds.push(JTokenKind::ArrayToken);
                token << c;
                emit(TokenKind::LeftSQBracket);
                return ignoreWhitespace(parseTokenOrArrayEnd());
            case '"':
                nodeKinds.push(JTokenKind::StringToken);
                token << c;
                emit(TokenKind::DoubleQuote);
                return parseString();
            case '-':
                nodeKinds.push(JTokenKind::NumberToken);
                token << c;
                emit(TokenKind::Sign);
                return parseIntegerStart();
            case 't':
                nodeKinds.push(JTokenKind::LiteralToken);
                return unpeek(readLiteral("true", TokenKind::TrueLiteral), c);
            case 'f':
                nodeKinds.push(JTokenKind::LiteralToken);
                return unpeek(readLiteral("false", TokenKind::FalseLiteral), c);
            case 'n':
                nodeKinds.push(JTokenKind::LiteralToken);
                return unpeek(readLiteral("null", TokenKind::NullLiteral), c);
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
        return expectedInput("'\"'", c);
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
        return expectedInput("'}' or ','", c);
    });
}

ParseState *parseString(){// TODO JOSH, parse escapes
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

ParseState *parseIntegerStart() {
    if (AllowSuperfluousLeadingZeroes){
        return parseInteger();
    }
    return new ParseState([](char c) { 
        if (c == '0')
        {
            token << c;
            emit(TokenKind::Integer);
            return parseOptionalDecimal();
        }
        if (std::isdigit(c))
        {
            token << c;
            return parseInteger();
        }
        return expectedInput("digit", c);
    });
}

ParseState *parseInteger() {
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseInteger();
        }
        emit(TokenKind::Integer);
        return unpeek(parseOptionalDecimal(), c);
    });
}

ParseState *parseOptionalDecimal() {
    return new ParseState([](char c) { 
        if (c == '.')
        {
            token << c;
            emit(TokenKind::DecimalPoint);
            return parseFractionalIntegerStart();
        }
        tokens.push(nullptr);// .
        tokens.push(nullptr);// XXX
        return unpeek(parseOptionalExp(), c);
    });
}

ParseState *parseFractionalIntegerStart() {
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseFractionalInteger();
        }
        return expectedInput("digit", c);
    });
}

ParseState *parseFractionalInteger() {
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseFractionalInteger();
        }
        emit(TokenKind::Integer);
        return unpeek(parseOptionalExp(), c);
    });
}

ParseState *parseOptionalExp() {
    return new ParseState([](char c) { 
        if (c == 'e' || c == 'E')
        {
            token << c;
            emit(TokenKind::Exp);
            return parseOptionalExpSign();
        }
        tokens.push(nullptr);// e
        tokens.push(nullptr);// +/-
        tokens.push(nullptr);// XXX
        return unpeek(pushNode(), c);
    });
}

ParseState *parseOptionalExpSign() {
    return new ParseState([](char c) { 
        if (c == '-' || c == '+')
        {
            token << c;
            emit(TokenKind::Sign);
            return parseExpIntegerStart();
        }
        tokens.push(nullptr);// +/-
        return unpeek(parseExpIntegerStart(), c);
    });
}

ParseState *parseExpIntegerStart() {
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseExpInteger();
        }
        return expectedInput("digit", c);
    });
}

ParseState *parseExpInteger() {
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseExpInteger();
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
    case JTokenKind::LiteralToken:
    {
        auto value = tokens.top(); tokens.pop();
        nodes.push(new JLiteral(value));
        break;
    }
    case JTokenKind::NumberToken:
    {
        auto expPart = tokens.top(); tokens.pop();
        auto expSign = tokens.top(); tokens.pop();
        auto exp = tokens.top(); tokens.pop();
        auto fractionalPart = tokens.top(); tokens.pop();
        auto decimal = tokens.top(); tokens.pop();
        auto wholePart = tokens.top(); tokens.pop();
        auto leadingMinus =   tokens.top(); tokens.pop();

        nodes.push(new JNumber(leadingMinus, wholePart, decimal, fractionalPart, exp, expSign, expPart));
        break;
    }
    case JTokenKind::ObjectToken:
    {
        auto end = tokens.top(); tokens.pop();
        auto props = new std::vector<JProperty *>();
        while (!nodes.empty())
        {
            if (nodes.top()->Kind() == JTokenKind::PropertyToken){
                props->push_back(dynamic_cast<JProperty *>(nodes.top()));
                nodes.pop();
            }
            else {
                break;
            }
        }
        std::reverse(props->begin(), props->end());
        auto begin = tokens.top(); tokens.pop();
        nodes.push(new JObject(begin, props, end));
        break;
    }
    case JTokenKind::ArrayToken:
    {
        auto end = tokens.top(); tokens.pop();
        auto elems = new std::vector<JArrayElement *>();
        while (!nodes.empty())
        {
            if (nodes.top()->Kind() == JTokenKind::ArrayElementToken){
                elems->push_back(dynamic_cast<JArrayElement *>(nodes.top()));
                nodes.pop();
            }
            else {
                break;
            }
        }
        std::reverse(elems->begin(), elems->end());
        auto begin = tokens.top(); tokens.pop();
        nodes.push(new JArray(begin, elems, end));
        break;
    }
    case JTokenKind::ArrayElementToken:
    {
        auto trailing = tokens.top(); tokens.pop();
        auto value = nodes.top(); nodes.pop();
        nodes.push(new JArrayElement(value, trailing));
        hadTrailingComma = trailing != nullptr;
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
        auto trailingComma = tokens.top(); tokens.pop();
        auto value = nodes.top(); nodes.pop();
        auto colon = tokens.top(); tokens.pop();
        auto name = dynamic_cast<JString*>(nodes.top()); nodes.pop();
        nodes.push(new JProperty(name, colon, value, trailingComma));
        hadTrailingComma = trailingComma != nullptr;
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
    case JTokenKind::ArrayElementToken:
        return ignoreWhitespace(parseOptionalComma());
    case JTokenKind::ArrayToken:
        if (!AllowTrailingCommas && hadTrailingComma){
            nodeKinds.push(JTokenKind::ArrayElementToken);
        }
        return ignoreWhitespace(hadTrailingComma 
            ? (AllowTrailingCommas ? parseTokenOrArrayEnd() : beginToken())
            : parseArrayEnd());
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
        tokens.push(nullptr);// ,
        return unpeek(pushNode(), c);
    });
}

ParseState *parseTokenOrArrayEnd(){
    return new ParseState([](char c) { 
        if (c == ']')
        {
            token << c;
            emit(TokenKind::RightSQBracket);
            return pushNode();
        }
        nodeKinds.push(JTokenKind::ArrayElementToken);
        return unpeek(beginToken(), c);
    });
}

ParseState *parseArrayEnd(){
    return new ParseState([](char c) { 
        if (c == ']')
        {
            token << c;
            emit(TokenKind::RightSQBracket);
            return pushNode();
        }
        return expectedInput("']' or ','", c);
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

ParseState *expectedInput(std::string expectedMessage, char c){
    std::cerr << "Expected input " << expectedMessage << " got '" << c << "'" << std::endl;
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

ParseState *readLiteral(std::string sequence, TokenKind literalKind)
{
    return new ParseState([=](char c) {
        if (c == sequence[0])
        {
            token << c;
            if (sequence.length() == 1)
            {
                emit(literalKind);
                return pushNode();
            }
            return readLiteral(sequence.substr(1), literalKind);
        }
        return unexpectedInput(c);
    });
}