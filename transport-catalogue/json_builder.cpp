#include "json_builder.h"

namespace json {
    using namespace std::literals;

    // Builder ----------------------------------------------------------------------

    Builder::Builder() {
        nodes_stack_.push_back(&root_);
    }

    // Кладем в вектор пустой словарь
    // Начинает определение сложного значения - словаря.
    // Вызывается в тех же контекстах, что и Value.
    // Следующим вызовом обязательно должен быть Key или EndDict.
    DictItemContext Builder::StartDict() {
        if (nodes_stack_.empty()) throw std::logic_error("Must start with builder"s);

        if (!(last_method_ == POSSIBLE_METHOD::CONSTRUCTOR ||
            nodes_stack_.back()->IsArray() ||
            last_method_ == POSSIBLE_METHOD::KEY))
        {
            throw std::logic_error("StartDict");
        }

        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(Dict());
            Node* node = &const_cast<Array&>(nodes_stack_.back()->AsArray()).back();
            nodes_stack_.push_back(node);
        }
        else {
            *nodes_stack_.back() = Dict();
        }
        last_method_ = POSSIBLE_METHOD::START_DICT;
        return std::move(*this);
    }

    // Кладем в вектор пустой вектор
    // Начинает определение сложного значения-массива.
    // Вызывается в тех же контекстах, что и Value.
    // Следующим вызовом обязательно должен быть EndArray или любой,
    // задающий новое значение: Value, StartDict или StartArray.
    ArrayItemContext Builder::StartArray() {

        if (nodes_stack_.empty()) throw std::logic_error("Must start with builder"s);

        if (!(last_method_ == POSSIBLE_METHOD::CONSTRUCTOR ||
            nodes_stack_.back()->IsArray() ||
            last_method_ == POSSIBLE_METHOD::KEY))
        {
            throw std::logic_error("StartArray");
        }

        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(Array());
            Node* node = &const_cast<Array&>(nodes_stack_.back()->AsArray()).back();
            nodes_stack_.push_back(node);
        }
        else {
            *nodes_stack_.back() = Array();
        }
        last_method_ = POSSIBLE_METHOD::START_ARRAY;
        return std::move(*this);
    }

    // Заканчиваем словарь и добавляем его в родительский узел
    // Завершает определение сложного значения - словаря.
    // Последним незавершённым вызовом Start * должен быть StartDict.
    Builder& Builder::EndDict() {
        if (nodes_stack_.empty()) throw std::logic_error("Must start with builder"s);

        if (!nodes_stack_.back()->IsDict() || nodes_stack_.empty()) {
            throw std::logic_error("EndDict закрывает не словарь."s);
        }
        nodes_stack_.pop_back();
        last_method_ = POSSIBLE_METHOD::END_DICT;
        return *this;
    }

    // Заканчиваем вектор и добавляем его в родительский узел
    // Завершает определение сложного значения - массива.
    // Последним незавершённым вызовом Start * должен быть StartArray.
    Builder& Builder::EndArray() {
        if (nodes_stack_.empty()) throw std::logic_error("Must start with builder"s);

        if (!nodes_stack_.back()->IsArray() || nodes_stack_.empty()) {
            throw std::logic_error("EndArray закрывает не массив."s);
        }
        nodes_stack_.pop_back();
        last_method_ = POSSIBLE_METHOD::END_ARRAY;
        return *this;
    }

    // Записываем переданный ключ
    // При определении словаря задаёт строковое значение ключа для очередной
    // пары ключ-значение. Следующий вызов метода обязательно должен задавать
    // соответствующее этому ключу значение с помощью метода Value или начинать
    // его определение с помощью StartDict или StartArray.
    KeyContext Builder::Key(std::string key)
    {
        if (nodes_stack_.empty()) throw std::logic_error("Must start with builder"s);

        if ( last_method_ == POSSIBLE_METHOD::KEY ) {
            throw std::logic_error("Key");
        }

        nodes_stack_.emplace_back(&const_cast<Dict&>(nodes_stack_.back()->AsDict())[key]);
        last_method_ = POSSIBLE_METHOD::KEY;
        return std::move(*this);
    }

    // Записываем переданные значения
    // Задаёт значение, соответствующее ключу при определении словаря,
    // очередной элемент массива или, если вызвать сразу после конструктора json::Builder,
    // всё содержимое конструируемого JSON - объекта.
    // Может принимать как простой объект — число или строку — так и целый массив или словарь.
    // Здесь Node::Value — это синоним для базового класса Node,
    // шаблона variant с набором возможных типов - значений.
    Builder& Builder::Value(Node value) {
        if (nodes_stack_.empty()) throw std::logic_error("Must start with builder"s);

        bool is_array = (nodes_stack_.back()->IsArray());

        if ( !( last_method_ == POSSIBLE_METHOD::CONSTRUCTOR ||
                last_method_ == POSSIBLE_METHOD::KEY ||
                is_array) )  {
            throw std::logic_error("Value");
        }

        if (nodes_stack_.back()->IsArray()) {
            const_cast<Array&>(nodes_stack_.back()->AsArray()).push_back(value);
        }
        else {
            *nodes_stack_.back() = value;
            nodes_stack_.pop_back();
        }
        last_method_ = POSSIBLE_METHOD::VALUE;
        return *this;
    }

    // Возвращает объект json::Node, содержащий JSON, описанный предыдущими вызовами методов.
    // К этому моменту для каждого Start* должен быть вызван соответствующий End*.
    // При этом сам объект должен быть определён, то есть вызов json::Builder{}.Build() недопустим.
    Node Builder::Build() {
        if (root_.IsNull()) {
             throw std::logic_error("Calling Build on an unready Object"s);
        }
        if ( last_method_ == POSSIBLE_METHOD::CONSTRUCTOR ||
             (nodes_stack_.size() > 1 &&
             (nodes_stack_.back()->IsArray() || nodes_stack_.back()->IsDict()) ) ) {
             throw std::logic_error("Calling Build on an unready Object"s);
        }
        return std::move(root_);
    }

    // KeyContext----------------------------------------------------------------------

    KeyContext::KeyContext(Builder&& builder) : builder_(builder) {}

    DictItemContext KeyContext::Value(Node value) {
        return std::move(builder_.Value(std::move(value)));
    }

    ArrayItemContext KeyContext::StartArray() {
        return std::move(builder_.StartArray());
    }

    DictItemContext KeyContext::StartDict() {
        return std::move(builder_.StartDict());
    }

    // DictItemContext----------------------------------------------------------------------

    DictItemContext::DictItemContext(Builder&& builder) : builder_(builder) {}

    KeyContext DictItemContext::Key(std::string key) {
        return std::move(builder_.Key(std::move(key)));
    }

    Builder& DictItemContext::EndDict() {
        return builder_.EndDict();
    }

    // ArrayItemContext----------------------------------------------------------------------

    ArrayItemContext::ArrayItemContext(Builder&& builder) : builder_(builder) {}

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
