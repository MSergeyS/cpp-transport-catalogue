#pragma once

#include <cmath>
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace svg {
    // Доработка SVG - библиотеки — добавлен новые способы задавать цвет :
    //   svg::Color color1;                               // none
    //   svg::Color color2 = svg::Rgb{ 215, 30, 25 };       // rgb(215,30,25)
    //   svg::Color color3 = svg::NoneColor;              // none
    //   svg::Color color4 = svg::Rgba{ 15, 15, 25, 0.7 };  // rgba(15,15,25,0.7)
    //   svg::Color color5 = "red"s;                      // red
    //   svg::Color color6 = svg::Rgb{};                  // rgb(0,0,0)
    //   svg::Color color7 = svg::Rgba{};                 // rgba(0,0,0,1.0);

    // **svg::Rgb**
    // Задаёт цвет в формате RGB, в виде трёх компонент red, green, blue типа uint8_t.

    /* Пример использования :

    // Задаёт цвет в виде трех компонент в таком порядке: red, green, blue
      svg::Rgb rgb{ 255, 0, 100 };
      assert(rgb.red == 255);
      assert(rgb.green == 0);
      assert(rgb.blue == 100);

    // Задаёт цвет по умолчанию: red=0, green=0, blue=0
      svg::Rgb color;
      assert(color.red == 0 && color.green == 0 && color.blue == 0);

    */
    struct Rgb {
        Rgb() = default;

        Rgb(uint8_t red, uint8_t green, uint8_t blue)
            : red(red), green(green), blue(blue) {
        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
    };

    // **svg::Rgba**
    // Задаёт цвет в формате RGBA, в виде трёх компонент red, green, blue типа uint8_t
    // и компоненты opacity типа double.
    // Компонента opacity или альфа - канал задаёт степень непрозрачности цвета
    // от 0.0 (абсолютно прозрачно) до 1.0 (абсолютно непрозрачный цвет).

    /* Пример использования :
    // Задаёт цвет в виде четырёх компонент: red, green, blue, opacity
      svg::Rgba rgba{ 100, 20, 50, 0.3 };
      assert(rgba.red == 100);
      assert(rgba.green == 20);
      assert(rgba.blue == 50);
      assert(rgba.opacity == 0.3);

    // Чёрный непрозрачный цвет: red=0, green=0, blue=0, alpha=1.0
      svg::Rgba color;
      assert(color.red == 0 && color.green == 0 && color.blue == 0 && color.opacity == 1.0);
    */

    struct Rgba {
        Rgba() = default;

        Rgba(int red, int green, int blue, double opacity)
            : red(red), green(green), blue(blue), opacity(opacity) {

        }

        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        double opacity = 1.0;
    };

    // Объявляем в библиотеке тип svg::Color как std::variant,
    // который объединяет типы std::monostate, std::string, svg::Rgb и svg::Rgba.
    // Значение std::monostate обозначает отсутствующий цвет, который выводится в виде строки "none".
    // Чтобы тип Color по умолчанию хранил значение monostate, разместите monostate первым
    // в списке типов variant.
    using Color = std::variant<std::monostate, Rgb, Rgba, std::string>;

    // Также объявите константу NoneColor, имеющую значение none :
    // Объявив в заголовочном файле константу со спецификатором inline,
    // мы сделаем так, что она будет одной на все единицы трансляции,
    // которые подключают этот заголовок.
    // В противном случае каждая единица трансляции будет использовать свою копию этой константы
    inline const Color NoneColor{ };

    struct ColorPrinter {
        void operator()(std::monostate) {
            using namespace std::literals;

            out << "none"sv;;
        }

        void operator()(Rgb rgb) {
            using namespace std::literals;

            out << "rgb("sv;
            out << std::to_string(rgb.red) << ","sv;
            out << std::to_string(rgb.green) << ","sv;
            out << std::to_string(rgb.blue) << ")"sv;
        }

        void operator()(Rgba rgba) {
            using namespace std::literals;

            out << "rgba("sv;
            out << std::to_string(rgba.red) << ","sv;
            out << std::to_string(rgba.green) << ","sv;
            out << std::to_string(rgba.blue) << ","sv;
            out << rgba.opacity << ")"sv;
        }

        void operator()(std::string text) {
            out << text;
        }

        std::ostream& out;
    };


    inline std::ostream& operator<<(std::ostream& out, Color color) {
        using namespace std::literals;
        std::visit(ColorPrinter{ out }, color);
        return out;
    }


    struct Point {
        Point() = default;
        Point(double x, double y)
            : x(x)
            , y(y) {
        }
        double x = 0;
        double y = 0;
    };

    /*
     * Вспомогательная структура, хранящая контекст для вывода SVG-документа с отступами.
     * Хранит ссылку на поток вывода, текущее значение и шаг отступа при выводе элемента
     */
    struct RenderContext {
        RenderContext(std::ostream& out)
            : out(out) {
        }

        RenderContext(std::ostream& out, int indent_step, int indent = 0)
            : out(out)
            , indent_step(indent_step)
            , indent(indent) {
        }

        RenderContext Indented() const {
            return { out, indent_step, indent + indent_step };
        }

        void RenderIndent() const {
            for (int i = 0; i < indent; ++i) {
                out.put(' ');
            }
        }

        std::ostream& out;
        int indent_step = 0;
        int indent = 0;
    };

    // Object -------------------------------------------------------------------------

    /*
     * Абстрактный базовый класс Object служит для унифицированного хранения
     * конкретных тегов SVG-документа
     * Реализует паттерн "Шаблонный метод" для вывода содержимого тега
     */
    class Object {
    public:
        void Render(const RenderContext& context) const;

        virtual ~Object() = default;

    private:
        virtual void RenderObject(const RenderContext& context) const = 0;

    protected:
        Object() = default;
    };

    enum class StrokeLineCap {
        BUTT,
        ROUND,
        SQUARE,
    };

    // Для вывода значений StrokeLineCap перегрузите оператор вывода в поток.
    //  https://www.w3.org/TR/SVG/painting.html#StrokeLinecapProperty
    inline std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap_) {
        using namespace std::literals;
        switch (static_cast<int>(line_cap_)) {
        case 0:
            out << "butt"sv;
            break;
        case 1:
            out << "round"sv;
            break;
        case 2:
            out << "square"sv;
            break;
        }
        return out;
    }

    // Типы StrokeLineCap и StrokeLineJoin объявляются с использованием enum:
    enum class StrokeLineJoin {
        ARCS,
        BEVEL,
        MITER,
        MITER_CLIP,
        ROUND,
    };

    // Для вывода значений StrokeLineJoin перегрузите оператор вывода в поток.
    // https://www.w3.org/TR/SVG/painting.html#StrokeLinejoinProperty
    inline std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join_) {
        using namespace std::literals;
        switch (line_join_) {
        case StrokeLineJoin::ARCS:
            out << "arcs"sv;
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel"sv;
            break;
        case StrokeLineJoin::MITER:
            out << "miter"sv;
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip"sv;
            break;
        case StrokeLineJoin::ROUND:
            out << "round"sv;
            break;
        }
        return out;
    }

    // Добавьте PathProps в качестве родителя к классам Circle, Text, Polyline,
    // чтобы наделить их свойствами этого класса.
    template <typename Owner>
    class PathProps {
    public:
        // Эти методы должны поддерживать method chaining,
        // то есть возвращать ссылку на тип объекта, у которого были вызваны.

        // SetFillColor(Color color) задаёт значение свойства fill — цвет заливки.
        // По умолчанию свойство не выводится.
        Owner& SetFillColor(Color color) {
            fill_color_ = std::move(color);
            return AsOwner();
        }

        // SetStrokeColor(Color color) задаёт значение свойства stroke — цвет контура.
        // По умолчанию свойство не выводится.
        Owner& SetStrokeColor(Color color) {
            stroke_color_ = std::move(color);
            return AsOwner();
        }

        // SetStrokeWidth(double width) задаёт значение свойства stroke - width — толщину линии.
        // По умолчанию свойство не выводится.
        Owner& SetStrokeWidth(double width) {
            stroke_width_ = std::move(width);
            return AsOwner();
        }

        // SetStrokeLineCap(StrokeLineCap line_cap) задаёт значение свойства stroke-linecap —
        // тип формы конца линии. По умолчанию свойство не выводится.
        Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
            line_cap_ = std::move(line_cap);
            return AsOwner();
        }

        // SetStrokeLineJoin(StrokeLineJoin line_join) задаёт значение свойства stroke-linejoin —
        // тип формы соединения линий. По умолчанию свойство не выводится.
        Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
            line_join_ = std::move(line_join);
            return AsOwner();
        }

    protected:
        ~PathProps() = default;

        void RenderAttrs(std::ostream& out) const {
            using namespace std::literals;

            if (fill_color_) {
                out << " fill=\""sv << *fill_color_ << "\""sv;
            }
            if (stroke_color_) {
                out << " stroke=\""sv << *stroke_color_ << "\""sv;
            }
            if (stroke_width_) {
                out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
            }
            if (line_cap_) {
                out << " stroke-linecap=\""sv << *line_cap_ << "\""sv;
            }
            if (line_join_) {
                out << " stroke-linejoin=\""sv << *line_join_ << "\""sv;
            }
        }

    private:
        Owner& AsOwner() {
            // static_cast безопасно преобразует *this к Owner&,
            // если класс Owner — наследник PathProps
            return static_cast<Owner&>(*this);
        }

        // Чтобы сохранять значения новых свойств класса PathProps только
        // при вызове соответствующих Set-методов, храните значения свойств в optional.
        // Для тех свойств, значения которых равны std::nullopt, Set-методы не вызывались.
        // Значит, выводить их в выходной поток не нужно.
        std::optional<Color> fill_color_;
        std::optional<Color> stroke_color_;
        std::optional<double> stroke_width_;
        std::optional<StrokeLineCap> line_cap_;
        std::optional<StrokeLineJoin> line_join_;
    };

    // Circle ------------------------------------------------------------------------

    /*
     * Класс Circle моделирует элемент <circle> для отображения круга
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/circle
     */
    class Circle final : public Object, public PathProps<Circle> {
    public:
        Circle& SetCenter(Point center);
        Circle& SetRadius(double radius);

    private:
        void RenderObject(const RenderContext& context) const override;

        Point center_ = { 0, 0 };
        double radius_ = 1.0;
    };

    // Polyline -----------------------------------------------------------------------

    /*
     * Класс Polyline моделирует элемент <polyline> для отображения ломаных линий
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/polyline
     */
    class Polyline final : public Object, public PathProps<Polyline> {
    public:
        // Добавляет очередную вершину к ломаной линии
        Polyline& AddPoint(Point point);

    private:
        std::vector<Point> points_;

        void RenderObject(const RenderContext& context) const override;
    };

    // Text ----------------------------------------------------------------------------

    /*
     * Класс Text моделирует элемент <text> для отображения текста
     * https://developer.mozilla.org/en-US/docs/Web/SVG/Element/text
     */
    class Text final : public Object, public PathProps<Text> {
    public:
        // Задаёт координаты опорной точки (атрибуты x и y)
        Text& SetPosition(Point pos);

        // Задаёт смещение относительно опорной точки (атрибуты dx, dy)
        Text& SetOffset(Point offset);

        // Задаёт размеры шрифта (атрибут font-size)
        Text& SetFontSize(uint32_t size);

        // Задаёт название шрифта (атрибут font-family)
        Text& SetFontFamily(std::string font_family);

        // Задаёт толщину шрифта (атрибут font-weight)
        Text& SetFontWeight(std::string font_weight);

        // Задаёт текстовое содержимое объекта (отображается внутри тега text)
        Text& SetData(std::string data);

    private:
        Point pos_ = { 0, 0 };
        Point offset_ = { 0, 0 };
        uint32_t font_size_ = 1;
        std::string font_family_;
        std::string font_weight_;
        std::string data_ = "";

        void PrintText(const RenderContext& context) const;
        void RenderObject(const RenderContext& context) const override;
    };

    // ObjectContainer -----------------------------------------------------------------------------

    class ObjectContainer {
    public:
        template <typename Obj>
        void Add(Obj obj) {
            objects_.push_back(std::make_shared<Obj>(std::move(obj)));
        }

        virtual void AddPtr(std::shared_ptr<Object>&& obj) = 0;

    protected:
        std::vector<std::shared_ptr<Object>> objects_;

        virtual ~ObjectContainer() = default;
    };

    class Drawable {
    public:
        virtual void Draw(ObjectContainer& container) const = 0;

        virtual ~Drawable() = default;
    };

    // Document ------------------------------------------------------------------------------------

    class Document : public ObjectContainer {
    public:
        /*
         Метод Add добавляет в svg-документ любой объект-наследник svg::Object.
         Пример использования:
         Document doc;
         doc.Add(Circle().SetCenter({20, 30}).SetRadius(15));
        */

        Document() = default;

        // Добавляет в svg-документ объект-наследник svg::Object
        void AddPtr(std::shared_ptr<Object>&& obj);

        // Выводит в ostream svg-представление документа
        void Render(std::ostream& out) const;

        void Clear();
    };

}  // namespace svg
