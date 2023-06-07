#pragma once

#include "json.h"

namespace json {

    class KeyContext;
    class DictItemContext;
    class ArrayItemContext;

    // Builder ----------------------------------------------------------------------

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
        Node root_{ nullptr };
        std::vector<Node*> nodes_stack_;

        enum class POSSIBLE_METHOD {
            CONSTRUCTOR,
            KEY,
            VALUE,
            START_DICT,
            START_ARRAY,
            END_DICT,
            END_ARRAY
        } last_method_ = POSSIBLE_METHOD::CONSTRUCTOR;

        /*
        Обработка ошибок
           В случае использования методов в неверном контексте выбросывается исключение типа
           std::logic_error с понятным сообщением об ошибке.
        Это должно происходить в следующих ситуациях :
           -  Вызов метода Build при неготовом описываемом объекте, то есть сразу
              после конструктора или при незаконченных массивах и словарях.
           -  Вызов любого метода, кроме Build, при готовом объекте.
           -  Вызов метода Key снаружи словаря или сразу после другого Key.
           -  Вызов Value, StartDict или StartArray где - либо, кроме как после конструктора,
              после Key или после предыдущего элемента массива.
           -  Вызов EndDict или EndArray в контексте другого контейнера.
        */

    }; // class Builder

    /*
    Улучшите класс json::Builder так, чтобы некоторые явные ошибки находились на этапе компиляции,
    а не выбрасывались в виде исключений при запуске программы. 
    (Ловить так все ошибки — сложная задача. Вы решите её в следующих спринтах, а пока начнёте с простого.)

    Код работы с обновлённым json::Builder не должен компилироваться в следующих ситуациях:
        1. Непосредственно после Key вызван не Value, не StartDict и не StartArray.
        2. После вызова Value, последовавшего за вызовом Key, вызван не Key и не EndDict.
        3. За вызовом StartDict следует не Key и не EndDict.
        4. За вызовом StartArray следует не Value, не StartDict, не StartArray и не EndArray.
        5. После вызова StartArray и серии Value следует не Value, не StartDict, не StartArray и не EndArray.

    Примеры кода, которые не должны компилироваться:
        json::Builder{}.StartDict().Build();  // правило 3
        json::Builder{}.StartDict().Key("1"s).Value(1).Value(1);  // правило 2
        json::Builder{}.StartDict().Key("1"s).Key(""s);  // правило 1
        json::Builder{}.StartArray().Key("1"s);  // правило 4
        json::Builder{}.StartArray().EndDict();  // правило 4
        json::Builder{}.StartArray().Value(1).Value(2).EndDict();  // правило 5 
    */

    /*
    Эту задачу можно решить только возвратом специальных вспомогательных классов,
    допускающих определённые наборы методов.

    На одном примере разберём, как гарантировать, что после StartDict вызывается Key или EndDict.
    Вернём из метода StartDict объект вспомогательного класса DictItemContext со следующими свойствами :
       - хранит ссылку на Builder;
       - поддерживает только методы Key и EndDict, делегируемые в Builder.
     Чтобы избежать дублирования кода с делегированием, удобно использовать наследование. 
    */

    // KeyContext----------------------------------------------------------------------

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

    // DictItemContext----------------------------------------------------------------------

    class DictItemContext final : Builder
    {
    public:
        DictItemContext(Builder&& builder);
        KeyContext Key(std::string key) override;
        Builder& EndDict() override;

    private:
        Builder& builder_;
    };

    // ArrayItemContext----------------------------------------------------------------------

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
