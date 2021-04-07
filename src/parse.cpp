#include <functional>
#include <iostream>
#include <string>
#include <stack>
#include <sstream>
#include <vector>
#include <algorithm>

#define ONE_INDENT "  "

using namespace std::string_literals; // enables s-suffix for std::string literals

enum TokenKind
{
    ObjectBegin,
    ObjectEnd,
    PropertyNameQuoted,
    PropertyColon,
    Literal,
    Comma,
};

enum NodeKind
{
    TokenNode,
    ArrayNode,
    ObjectNode,
    PropertyNode,
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

class JNode
{
public:
    virtual void Print(std::string indent) = 0;
};
class JToken : public JNode {
public:
    Token* Value;
    JToken(Token* value){
        Value = value;
    }
    void Print(std::string indent)
    {
        std::cout << indent << "Value: " << Value->StringValue << std::endl;
    }
};
class JProperty : public JNode {
public:
    Token *Name;
    Token* ColonToken;
    JNode *Value;
    Token* TrailingComma;
    JProperty(Token* name, Token* colon, JNode* value, Token* trailingComma){
        Name = name;
        ColonToken = colon;
        Value = value;
        TrailingComma = trailingComma;
    }
    void Print(std::string indent) {
        std::cout << indent << "Property '" << Name->StringValue << "':" << std::endl;
        Value->Print(indent + ONE_INDENT);
    }
};
class JObject : public JNode {
public: 
    Token* BeginToken;
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
class JArray : public JNode {
public: 
    void Print(std::string indent) {
        std::cout << indent << "Array:" << std::endl;
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
std::stack<NodeKind> nodeKinds;
std::stack<JNode *> nodes;

void emit(TokenKind kind){
    tokens.push(new Token(kind, token.str()));
    token.str("");
    token.clear();
}

ParseState *beginToken();
ParseState *error(std::string);
ParseState *unexpectedInput(char c);
ParseState *ignoreInput();
ParseState *parseObject();
ParseState *parseObjectEnd();
ParseState *parsePropertyNameQuoted();
ParseState *parsePropertyValue();
ParseState *parseOptionalComma();
ParseState *parseInteger();
ParseState *pushNode();
ParseState *eof();

ParseState *ignoreWhitespace(ParseState *);
ParseState *unpeek(ParseState *, char);

int main()
{
    auto jsonText = R"(
        {
            "test": 12345,
            "foobar": 3
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
            nodeKinds.push(NodeKind::TokenNode);
            token << c;
            return parseInteger();
        }
        switch (c) {
            case '{':
                nodeKinds.push(NodeKind::ObjectNode);
                token << c;
                emit(TokenKind::ObjectBegin);
                return ignoreWhitespace(parseObject());
            }
        return error("Expected beginning of token.");
    });
}

ParseState *parseObject(){
    return new ParseState([](char c) { 
        switch (c)
        {
        case '"':
            nodeKinds.push(NodeKind::PropertyNode);
            token << c;
            return parsePropertyNameQuoted();
        }
        return unpeek(parseObjectEnd(), c);
    });
}

ParseState *parseObjectEnd(){
    return new ParseState([](char c) { 
        switch (c)
        {
        case '}':
            token << c;
            emit(TokenKind::ObjectEnd);
            return pushNode();
        }
        return unexpectedInput(c);
    });
}

ParseState *parsePropertyNameQuoted(){
    return new ParseState([](char c) {
        token << c;
        if (c == '"')
        {
            emit(TokenKind::PropertyNameQuoted);
            return ignoreWhitespace(parsePropertyValue());
        }
        return parsePropertyNameQuoted();
    });
}
ParseState *parsePropertyValue() {
    return new ParseState([](char c) {
        if (c == ':')
        {
            token << c;
            emit(TokenKind::PropertyColon);
            return ignoreWhitespace(beginToken());
        }

        return error("Expected :");
    });
}
ParseState *parseInteger(){
    return new ParseState([](char c) { 
        if (std::isdigit(c))
        {
            token << c;
            return parseInteger();
        }
        emit(TokenKind::Literal);
        return unpeek(pushNode(), c);
    });
}
ParseState *pushNode(){
    auto kind = nodeKinds.top();
    nodeKinds.pop();
    bool hadTrailingComma = false;
    switch (kind)
    {
    case NodeKind::TokenNode:
    {
        nodes.push(new JToken(tokens.top()));
        tokens.pop();
        break;
    }
    case NodeKind::ObjectNode:
    {
        auto end = tokens.top();
        tokens.pop();
        auto begin = tokens.top();
        tokens.pop();
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
    case NodeKind::PropertyNode:
    {
        Token *trailingComma = nullptr;
        if (tokens.top()->Kind == TokenKind::Comma){
            hadTrailingComma = true;
            trailingComma = tokens.top();
            tokens.pop();
        }
        auto colon = tokens.top();
        tokens.pop();
        auto prop = new JProperty(tokens.top(), colon, nodes.top(), trailingComma);
        tokens.pop();
        nodes.pop();
        nodes.push(prop);
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
    case NodeKind::ObjectNode:
        return ignoreWhitespace(hadTrailingComma ? parseObject() : parseObjectEnd());
    case NodeKind::PropertyNode:
        return ignoreWhitespace(parseOptionalComma());
    default:
        std::cerr << "Could not continue after node " << nodeKinds.top() << std::endl;
        return ignoreInput();
    }
}

ParseState *parseOptionalComma(){
    return new ParseState([](char c) { 
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