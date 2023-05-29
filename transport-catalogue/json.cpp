#include "json.h"

#include <iterator>
#include <cctype>

namespace json {

    namespace {
        using namespace std::literals;

        Node LoadNode(std::istream& input);
        Node LoadString(std::istream& input);

        std::string LoadLiteral(std::istream& input) {
            std::string s;
            while (std::isalpha(input.peek())) {
                s.push_back(static_cast<char>(input.get()));
            }
            return s;
        }

        Node LoadArray(std::istream& input) {
            std::vector<Node> result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }
            if (!input) {
                throw ParsingError("Array parsing error"s);
            }
            return Node(std::move(result));
        }

        Node LoadDict(std::istream& input) {
            Dict dict;

            for (char c; input >> c && c != '}';) {
                if (c == '"') {
                    std::string key = LoadString(input).AsString();
                    if (input >> c && c == ':') {
                        if (dict.find(key) != dict.end()) {
                            throw ParsingError("Duplicate key '"s + key + "' have been found");
                        }
                        dict.emplace(std::move(key), LoadNode(input));
                    }
                    else {
                        throw ParsingError(": is expected but '"s + c + "' has been found"s);
                    }
                }
                else if (c != ',') {
                    throw ParsingError(R"(',' is expected but ')"s + c + "' has been found"s);
                }
            }
            if (!input) {
                throw ParsingError("Dictionary parsing error"s);
            }
            return Node(std::move(dict));
        }

        Node LoadString(std::istream& input) {
            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    s.push_back(ch);
                }
                ++it;
            }
            return Node(std::move(s));
        }

        Node LoadBool(std::istream& input) {
            const auto s = LoadLiteral(input);
            if (s == "true"sv) {
                return Node{ true };
            }
            else if (s == "false"sv) {
                return Node{ false };
            }
            else {
                throw ParsingError("Failed to parse '"s + s + "' as bool"s);
            }
        }

        Node LoadNull(std::istream& input) {
            if (auto literal = LoadLiteral(input); literal == "null"sv) {
                return Node{ nullptr };
            }
            else {
                throw ParsingError("Failed to parse '"s + literal + "' as null"s);
            }
        }

        Node LoadNumber(std::istream& input) {
            std::string parsed_num;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
            };

            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
            };

            if (input.peek() == '-') {
                read_char();
            }
            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_int = true;
            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {

                    }
                }
                return std::stod(parsed_num);
            }
            catch (...){
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        Node LoadNode(std::istream& input) {
            char c;
            if (!(input >> c)) {
                throw ParsingError("Unexpected EOF"s);
            }
            switch (c) {
            case '[':
                return LoadArray(input);
            case '{':
                return LoadDict(input);
            case '"':
                return LoadString(input);
            case 't':
                [[fallthrough]];
            case 'f':
                input.putback(c);
                return LoadBool(input);
            case 'n':
                input.putback(c);
                return LoadNull(input);
            default:
                input.putback(c);
                return LoadNumber(input);
            }
        }

        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const {
                for (int i = 0; i < indent; ++i) {
                    out.put(' ');
                }
            }

            PrintContext Indented() const {
                return { out, indent_step, indent_step + indent };
            }
        };

        void PrintNode(const Node& value, const PrintContext& ctx);

        template <typename Value>
        void PrintValue(const Value& value, const PrintContext& ctx)  {
            ctx.out << value;
        }

        void PrintString(const std::string& value, std::ostream& out) {
            out.put('"');
            for (const char c : value) {
                switch (c) {
                case '\r':
                    out << "\\r"sv;
                    break;
                case '\n':
                    out << "\\n"sv;
                    break;
                case '"':
                    [[fallthrough]];
                case '\\':
                    out.put('\\');
                    [[fallthrough]];
                default:
                    out.put(c);
                    break;
                }
            }
            out.put('"');
        }

        template <>
        void PrintValue<std::string>(const std::string& value, const PrintContext& ctx) {
            PrintString(value, ctx.out);
        }

        template <>
        void PrintValue<std::nullptr_t>(const std::nullptr_t&, const PrintContext& ctx) {
            ctx.out << "null"sv;
        }

        template <>
        void PrintValue<bool>(const bool& value, const PrintContext& ctx) {
            ctx.out << (value ? "true"sv : "false"sv);
        }

        template <>
        void PrintValue<Array>(const Array& nodes, const PrintContext& ctx)
        {
            std::ostream& out = ctx.out;
            if (nodes.size() == 0) {
                out << "[]"sv;
            }
            else {
                out << "[\n"sv;
                bool first = true;
                auto inner_ctx = ctx.Indented();
                for (const Node& node : nodes) {
                    if (first) {
                        first = false;
                    }
                    else {
                        out << ",\n"sv;
                    }
                    inner_ctx.PrintIndent();
                    PrintNode(node, inner_ctx);
                }
                out.put('\n');
                ctx.PrintIndent();
                out.put(']');
            }
        }

        template <>
        void PrintValue<Dict>(const Dict& nodes, const PrintContext& ctx) {
            std::ostream& out = ctx.out;
            out << "{\n"sv;
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const auto& [key, node] : nodes) {
                if (first) {
                    first = false;
                }
                else {
                    out << ",\n"sv;
                }
                inner_ctx.PrintIndent();
                PrintString(key, ctx.out);
                out << ": "sv;
                PrintNode(node, inner_ctx);
            }
            out.put('\n');
            ctx.PrintIndent();
            out.put('}');
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit([&ctx](const auto& value) {
                    PrintValue(value, ctx);
                }, node.GetValue());
        }

    } // namespace

    //------------Node---------------

    bool Node::IsInt() const {
        return std::holds_alternative<int>(*this);
    }

    int Node::AsInt()  const {
        return (!IsInt()) ? throw std::logic_error("Not a intager") : std::get<int>(*this);
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(*this);
    }

    bool Node::IsDouble() const {
        bool pd = IsPureDouble();
        if (pd == true)
        {
            return pd;
        }
        pd = IsInt();
        return pd;
    }

    double Node::AsDouble() const {
        return (!IsDouble()) ? throw std::logic_error("Not a double") : IsPureDouble() ? std::get<double>(*this) : AsInt();
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(*this);
    }

    bool Node::AsBool() const {
        return (!IsBool()) ? throw std::logic_error("Not a bool") : std::get<bool>(*this);
    }

    bool Node::IsNull() const {
        return std::holds_alternative<nullptr_t>(*this);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(*this);
    }

    const Array& Node::AsArray() const {
        return (!IsArray()) ? throw std::logic_error("Not a Array") : std::get<Array>(*this);
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(*this);
    }

    const std::string& Node::AsString() const {
        return (!IsString()) ? throw std::logic_error("Not a string") : std::get<std::string>(*this);
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(*this);
    }

    const Dict& Node::AsMap() const {
        return (!IsMap()) ? throw std::logic_error("Not a dict") : std::get<Dict>(*this);
    }

    const Node::Value& Node::GetValue() const {
        return *this;
    }

    bool Node::operator==(const Node& rhs) const {
        return GetValue() == rhs.GetValue();
    }

    bool Node::operator!=(const Node& rhs) const {
        return GetValue() != rhs.GetValue();
    }

    //------------Document----------------
    const Node& Document::GetRoot() const {
        return root_;
    }

    bool Document::operator==(const Document& rhs) const {
        return GetRoot() == rhs.GetRoot();
    }

    bool Document::operator!=(const Document& rhs) const {
        return GetRoot() != rhs.GetRoot();
    }

    //------------Functions---------------

    Document Load(std::istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintNode(doc.GetRoot(), PrintContext{ output });
    }

    //------------------Builder----------------------------

    Builder::Builder() {
        nodes_stack_.push_back(&root_);
    }

    DictItemContext Builder::StartDict() {
        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(Dict());
            Node* node = &const_cast<Array&>(nodes_stack_.back()->AsArray()).back();
            nodes_stack_.push_back(node);
        }
        else {
            *nodes_stack_.back() = Dict();
        }
        return std::move(*this);
    }

    ArrayItemContext Builder::StartArray() {
        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(Array());
            Node* node = &const_cast<Array&>(nodes_stack_.back()->AsArray()).back();
            nodes_stack_.push_back(node);
        }
        else {
            *nodes_stack_.back() = Array();
        }
        return std::move(*this);
    }

    Builder& Builder::EndDict() {
        nodes_stack_.erase(nodes_stack_.end() - 1);
        return *this;
    }

    Builder& Builder::EndArray() {
        nodes_stack_.erase(nodes_stack_.end() - 1);
        return *this;
    }

    KeyContext Builder::Key(std::string key)
    {
        nodes_stack_.emplace_back(&const_cast<Dict&>(nodes_stack_.back()->AsMap())[key]);
        return std::move(*this);
    }

    Builder& Builder::Value(Node value) {
        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(value);
        }
        else {
            *nodes_stack_.back() = value;
            nodes_stack_.erase(nodes_stack_.end() - 1);
        }
        return *this;
    }

    Node Builder::Build() {
        return std::move(root_);
    }

    //----------------KeyContext----------------

    KeyContext::KeyContext(Builder&& builder)
        : builder_(builder) {}

    DictItemContext KeyContext::Value(Node value) {
        return std::move(builder_.Value(std::move(value)));
    }

    ArrayItemContext KeyContext::StartArray() {
        return std::move(builder_.StartArray());
    }

    DictItemContext KeyContext::StartDict() {
        return std::move(builder_.StartDict());
    }

    //------------------DictItemContext------------------

    DictItemContext::DictItemContext(Builder&& builder)
        : builder_(builder) {}

    KeyContext DictItemContext::Key(std::string key) {
        return std::move(builder_.Key(std::move(key)));
    }

    Builder& DictItemContext::EndDict() {
        return builder_.EndDict();
    }

    //-----------------ArrayItemContext------------------------

    ArrayItemContext::ArrayItemContext(Builder&& builder)
        : builder_(builder) {}

    ArrayItemContext& ArrayItemContext::Value(Node value) {
        builder_.Value(std::move(value));
        return *this;
    }

    DictItemContext ArrayItemContext::StartDict() {
        return std::move(builder_.StartDict());
    }

    ArrayItemContext ArrayItemContext::StartArray() {
        return std::move(builder_.StartArray());
    }

    Builder& ArrayItemContext::EndArray() {
        return builder_.EndArray();
    }

} // namespace json
