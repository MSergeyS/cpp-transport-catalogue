// Объявите этот макрос в самом начале файла, чтобы при подключении <cmath>
// были объявлены макросы M_PI и другие
#define _USE_MATH_DEFINES

#include "svg.h"

namespace svg {

    using namespace std::literals;

    // Object -------------------------------------------------------------------------

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();

        // Делегируем вывод тега своим подклассам
        RenderObject(context);

        context.out << std::endl;
    }

    // Circle -------------------------------------------------------------------------

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\" "sv;
        // Выводим атрибуты, унаследованные от PathProps
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // Polyline -----------------------------------------------------------------------

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(std::move(point));
        return *this;
    }

    // Между координатами в значении свойства points тега polyline
    // должен быть ровно один пробел.
    // В начале и конце списка координат пробелов быть не должно :
    // <polyline points = "20,40 22.9389,45.9549 29.5106,46.9098" / >
    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;
        for (size_t i = 0; i < points_.size(); ++i) {
            if (i != 0) {
                out << " "sv;
            }
            const Point& point = points_.at(i);
            out << point.x << ","sv << point.y;
        }
        out << "\""sv;
        // Выводим атрибуты, унаследованные от PathProps
        RenderAttrs(context.out);
        out << "/>"sv;
    }

    // Text ----------------------------------------------------------------------------

    Text& Text::SetPosition(Point pos) {
        pos_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = font_family;
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = font_weight;
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = data;
        return *this;
    }

    // Текст отображается следующим образом: строка “<text ”,
    // затем через пробел свойства в произвольном порядке,
    // затем символ “>”, содержимое надписи и, наконец,
    // закрывающий тег “</text>”.
    // Символы ", <, >, ' и & имеют особое значение и при выводе должны экранироваться.
    // Например, текст “Hello, <UserName>.Would you like some "M&M's" ? ”
    // в SVG файле будет представлен так :
    // <text>Hello, & lt; UserName& gt; .Would you like some& quot; M& amp; M& apos; s& quot; ? < / text>
    void Text::PrintText(const RenderContext& context) const {
        auto& out = context.out;
        for (char c : data_) {
            switch (c) {
            case '\"':
                out << "&quot;";
                break;
            case '\'':
                out << "&apos;";
                break;
            case '<':
                out << "&lt;";
                break;
            case '>':
                out << "&gt;";
                break;
            case '&':
                out << "&amp;";
                break;
            default:
                out << c;
            }
        }
    }

    // <text x = "35" y = "20" dx = "0" dy = "6" font - size = "12" font - family = "Verdana" font - weight = "bold">Hello C++< / text>
    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text";
        // Выводим атрибуты, унаследованные от PathProps
        RenderAttrs(context.out);
        out << " x=\"" << pos_.x << "\"";
        out << " y=\"" << pos_.y << "\"";
        out << " dx=\"" << offset_.x << "\"";
        out << " dy=\"" << offset_.y << "\"";
        out << " font-size=\"" << font_size_ << "\"";
        if (font_family_ != "") {
            out << " font-family=\"" << font_family_ << "\"";
        }
        if (font_weight_ != "") {
            out << " font-weight=\"" << font_weight_ << "\"";
        }
        out << ">";
        Text::PrintText(context);
        out << "</text>";
    }

    // Document ------------------------------------------------------------------------------------

    void Document::AddPtr(std::shared_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    // Содержимое, выводимое методом svg::Document::Render,
    // должно состоять из следующих частей :
    // < ? xml version = "1.0" encoding = "UTF-8" ? >
    //  <svg xmlns = "http://www.w3.org/2000/svg" version = "1.1">
    //  Объекты, добавленные с помощью svg::Document::Add, в порядке их добавления
    //  < / svg>
    void Document::Render(std::ostream& out) const {
        RenderContext render_context(out,2,2);

        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

        for (const auto& ptr : objects_) {
            out << "  ";
            ptr.get()->Render(render_context);
        }

        out << "</svg>"sv;
    }

    void Document::Clear() {
        objects_.clear();
    }

}  // namespace svg
