#pragma once

#include <memory>
#include <string>

namespace edu {

    // -------------------- Color --------------------
    struct Color {
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;

        Color();
        Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha = 255);

        static Color Black();
        static Color White();
        static Color Red();
        static Color Green();
        static Color Blue();
        static Color Yellow();
        static Color Orange();
        static Color Purple();
        static Color Gray();
    };

    // Короткие имена цветов (ГЛОБАЛЬНЫЕ inline‑переменные)
    inline Color Black = Color::Black();
    inline Color White = Color::White();
    inline Color Red = Color::Red();
    inline Color Green = Color::Green();
    inline Color Blue = Color::Blue();
    inline Color Yellow = Color::Yellow();
    inline Color Orange = Color::Orange();
    inline Color Purple = Color::Purple();
    inline Color Gray = Color::Gray();

    Color rgb(int r, int g, int b, int a = 255);

    // -------------------- Предварительное объявление --------------------
    class Turtle;

    // -------------------- Canvas --------------------
    class Canvas {
    public:
        Canvas(int width = 800, int height = 600, const std::string& title = "edu turtle");
        ~Canvas();

        Canvas(const Canvas&) = delete;
        Canvas& operator=(const Canvas&) = delete;

        Canvas(Canvas&& other) noexcept;
        Canvas& operator=(Canvas&& other) noexcept;

        void set_background(Color color);
        void clear();
        void wait_until_close();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;

        friend class Turtle;

        void add_line(double x1, double y1, double x2, double y2, Color color, float width, double speed);
        void add_dot(double x, double y, float radius, Color color, double speed);
        void add_circle(double x, double y, float radius, Color color, float width, double speed);
        void add_text(double x, double y, const std::string& text, int font_size, Color color, double speed);
    };

    // -------------------- Turtle --------------------
    class Turtle {
    public:
        explicit Turtle(Canvas& canvas);

        void forward(double distance);
        void backward(double distance);

        void left(double degrees);
        void right(double degrees);

        void pen_up();
        void pen_down();

        void go_to(double x, double y);
        void set_heading(double degrees);
        void home();

        void color(Color color);
        void width(float pixels);
        void speed(double commands_per_second);

        void dot(float radius = 4.0f);
        void circle(double radius);
        void write(const std::string& text, int font_size = 20);

        double x() const;
        double y() const;
        double heading() const;

    private:
        Canvas* canvas_;
        double x_;
        double y_;
        double heading_deg_;
        bool pen_is_down_;
        Color pen_color_;
        float pen_width_;
        double speed_;
    };

}  // namespace edu
