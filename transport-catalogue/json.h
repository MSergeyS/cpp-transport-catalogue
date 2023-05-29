#pragma once

#include <iostream>
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace json
{
    class Node;
    using Dict = std::map<std::string, Node>;
    using Array = std::vector<Node>;

    class ParsingError
        : public std::runtime_error
    {
    public:

        using runtime_error::runtime_error;
    };

    class Node final : private std::variant<std::nullptr_t, Array, Dict, bool, int, uint64_t, double, std::string>
    {
    public:

        using variant::variant;
        using Value = variant;

        bool IsInt() const;
        int AsInt()  const;
        bool IsPureDouble() const;
        bool IsDouble() const;
        double AsDouble() const;
        bool IsBool() const;
        bool AsBool() const;
        bool IsNull() const;
        bool IsArray() const;
        const Array& AsArray() const;
        bool IsString() const;
        const std::string& AsString() const;
        bool IsMap() const;
        const Dict& AsMap() const;
        const Value& GetValue() const;
        bool operator==(const Node& rhs) const;
        bool operator!=(const Node& rhs) const;
    };

    class Document
    {
    public:

        explicit Document(Node root) : root_(std::move(root)) {
        }

        const Node& GetRoot() const;

        bool operator==(const Document& rhs) const;
        bool operator!=(const Document& rhs) const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

    class KeyContext;
    class DictItemContext;
    class ArrayItemContext;

    //---------Builder----------------

    class Builder
    {
    public:
        Builder();

        virtual DictItemContext StartDict();
        virtual ArrayItemContext StartArray();

        virtual Builder& EndDict();
        virtual Builder& EndArray();

        virtual KeyContext Key(std::string key);
        Builder& Value(Node value);

        virtual Node Build();

    private:
        Node root_;
        std::vector<Node*> nodes_stack_;
    };

    //------------KeyContext------------------

    class KeyContext final : Builder
    {
    public:
        KeyContext(Builder&& builder);
        DictItemContext Value(Node value);
        ArrayItemContext StartArray() override;
        DictItemContext StartDict() override;

    private:
        Builder& builder_;
    };

    //---------------DictItemContext-------------------

    class DictItemContext final : Builder
    {
    public:
        DictItemContext(Builder&& builder);
        KeyContext Key(std::string key) override;
        Builder& EndDict() override;

    private:
        Builder& builder_;
    };

    //-----------ArrayItemContext----------------------

    class ArrayItemContext final : Builder
    {
    public:
        ArrayItemContext(Builder&& builder);
        ArrayItemContext& Value(Node value);
        DictItemContext StartDict() override;
        ArrayItemContext StartArray() override;
        Builder& EndArray() override;

    private:
        Builder& builder_;
    };

} // namespace json
