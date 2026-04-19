// edu_turtle.cpp
// Реализация черепашьей графики для Windows (аналог Logo Turtle).
// Позволяет создавать окно (Canvas) и рисовать на нём линии, точки, круги, текст,
// а также управлять черепашкой (Turtle), которая двигается по холсту и оставляет след.

#include "edu_turtle.hpp"   // заголовочный файл с объявлениями классов

#include <cmath>            // математические функции (cos, sin, lround)
#include <stdexcept>        // исключения (invalid_argument)
#include <utility>          // std::move
#include <vector>           // std::vector для хранения команд рисования
#include <string>           // std::string
#include <sstream>          // (не используется напрямую, но может понадобиться)
#include <windows.h>        // Windows API для создания окна и рисования
#include <chrono>           // для замеров времени (анимация)
#include <thread>           // для sleep (задержка между кадрами)

namespace edu {

    // ---------- Внутренние вспомогательные функции (анонимное пространство имён) ----------
    namespace {

        const double kPi = 3.14159265358979323846;   // число Пи

        // Переводит градусы в радианы (нужно для cos/sin)
        double deg_to_rad(double degrees) {
            return degrees * kPi / 180.0;
        }

        // Преобразует наш цвет Color в цвет Windows (COLORREF)
        COLORREF to_windows_color(const edu::Color& c) {
            return RGB(c.r, c.g, c.b);
        }

        // Внутренняя структура, описывающая одну команду рисования
        struct Command {
            enum Type {      // тип команды
                Line,        // линия
                Dot,         // точка (закрашенный круг)
                Circle,      // окружность (контур)
                Text         // текст
            };

            Type type;           // тип команды

            // Координаты: для линии – начало и конец; для точки/круга/текста – центр/позиция
            double x1, y1;
            double x2, y2;

            float radius;        // радиус точки или круга
            float width;         // толщина пера (линия/окружность)
            edu::Color color;    // цвет рисования
            std::string text;    // текст (для команды Text)
            int font_size;       // размер шрифта
            double speed;        // скорость выполнения команды (команд/сек). 0 = мгновенно.

            // Конструктор по умолчанию: создаёт пустую линию с параметрами по умолчанию
            Command()
                : type(Line),
                x1(0.0), y1(0.0), x2(0.0), y2(0.0),
                radius(0.0f), width(1.0f),
                color(),
                text(),
                font_size(20),
                speed(0.0) {
            }
        };

    }  // namespace

    // -------------------- Класс Color (цвет) --------------------
    // Хранит цвет в формате RGBA (red, green, blue, alpha – прозрачность)

    Color::Color() : r(0), g(0), b(0), a(255) {}  // по умолчанию чёрный, непрозрачный

    Color::Color(unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha)
        : r(red), g(green), b(blue), a(alpha) {
    }

    // Статические методы – предустановленные цвета
    Color Color::Black() { return Color(0, 0, 0); }
    Color Color::White() { return Color(255, 255, 255); }
    Color Color::Red() { return Color(230, 41, 55); }
    Color Color::Green() { return Color(0, 228, 48); }
    Color Color::Blue() { return Color(0, 121, 241); }
    Color Color::Yellow() { return Color(253, 249, 0); }
    Color Color::Orange() { return Color(255, 161, 0); }
    Color Color::Purple() { return Color(200, 122, 255); }
    Color Color::Gray() { return Color(130, 130, 130); }

    // Вспомогательная функция для создания цвета с контролем диапазона (0..255)
    Color rgb(int r, int g, int b, int a) {
        if (r < 0) r = 0; if (r > 255) r = 255;
        if (g < 0) g = 0; if (g > 255) g = 255;
        if (b < 0) b = 0; if (b > 255) b = 255;
        if (a < 0) a = 0; if (a > 255) a = 255;
        return Color(static_cast<unsigned char>(r),
            static_cast<unsigned char>(g),
            static_cast<unsigned char>(b),
            static_cast<unsigned char>(a));
    }

    // -------------------- Внутренняя реализация Canvas (PImpl-идиома) --------------------
    // Скрывает детали Windows API от пользователя

    struct Canvas::Impl {
        int width;                  // ширина холста в пикселях
        int height;                 // высота холста
        std::string title;          // заголовок окна
        edu::Color background;      // цвет фона
        std::vector<Command> commands; // список всех команд рисования

        HWND hwnd;                  // дескриптор окна Windows
        bool is_window_created;     // создано ли окно
        HDC memory_dc;              // контекст устройства для рисования в памяти (двойная буферизация)
        HBITMAP memory_bitmap;      // битмап в памяти
        HBITMAP old_bitmap;         // старый битмап для восстановления

        // Конструктор: сохраняет размеры и заголовок, остальное инициализирует нулями
        Impl(int w, int h, const std::string& t)
            : width(w), height(h), title(t), background(edu::Color::White()), commands(),
            hwnd(nullptr), is_window_created(false), memory_dc(nullptr),
            memory_bitmap(nullptr), old_bitmap(nullptr) {
        }

        // Преобразование логической координаты X в экранную (центр холста = 0)
        int screen_x(double x) const {
            return static_cast<int>(std::lround(width / 2.0 + x));
        }

        // Преобразование логической координаты Y в экранную (ось Y перевёрнута: вверх = +)
        int screen_y(double y) const {
            return static_cast<int>(std::lround(height / 2.0 - y));
        }

        // Создаёт окно Windows и подготавливает контексты для рисования
        void create_window() {
            WNDCLASSA wc = { 0 };
            wc.lpfnWndProc = DefWindowProcA;          // стандартная обработка сообщений
            wc.hInstance = GetModuleHandle(nullptr);
            wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
            wc.lpszClassName = "EduTurtleWindowClass";

            RegisterClassA(&wc);

            // Корректируем размер окна с учётом рамок
            RECT rect = { 0, 0, width, height };
            AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

            hwnd = CreateWindowA(
                "EduTurtleWindowClass", title.c_str(),
                WS_OVERLAPPEDWINDOW,
                CW_USEDEFAULT, CW_USEDEFAULT,
                rect.right - rect.left, rect.bottom - rect.top,
                nullptr, nullptr, wc.hInstance, nullptr
            );

            is_window_created = true;
            ShowWindow(hwnd, SW_SHOW);
            UpdateWindow(hwnd);

            // Создаём контекст для рисования в памяти (двойная буферизация)
            HDC screen_dc = GetDC(hwnd);
            memory_dc = CreateCompatibleDC(screen_dc);
            memory_bitmap = CreateCompatibleBitmap(screen_dc, width, height);
            old_bitmap = (HBITMAP)SelectObject(memory_dc, memory_bitmap);
            ReleaseDC(hwnd, screen_dc);
        }

        // Закрывает окно и освобождает ресурсы GDI
        void close_window() {
            if (is_window_created && hwnd) {
                if (memory_dc) {
                    SelectObject(memory_dc, old_bitmap);
                    DeleteObject(memory_bitmap);
                    DeleteDC(memory_dc);
                }
                DestroyWindow(hwnd);
                hwnd = nullptr;
                is_window_created = false;
            }
        }

        // Обрабатывает сообщения Windows (чтобы окно не зависало)
        // Возвращает false, если пришёл WM_QUIT (запрос на закрытие)
        bool process_messages() {
            MSG msg;
            while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    return false;
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            return true;
        }

        // Рисует одну команду cmd на заданном контексте hdc (используя Windows GDI)
        void draw_one(HDC hdc, const Command& cmd) const {
            switch (cmd.type) {
            case Command::Line:  // линия
            {
                HPEN pen = CreatePen(PS_SOLID, static_cast<int>(cmd.width),
                    to_windows_color(cmd.color));
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);

                MoveToEx(hdc, screen_x(cmd.x1), screen_y(cmd.y1), nullptr);
                LineTo(hdc, screen_x(cmd.x2), screen_y(cmd.y2));

                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }
            break;

            case Command::Dot:  // закрашенная точка (эллипс)
            {
                HBRUSH brush = CreateSolidBrush(to_windows_color(cmd.color));
                HPEN pen = CreatePen(PS_SOLID, 1, to_windows_color(cmd.color));
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, brush);
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);

                int x = screen_x(cmd.x1) - static_cast<int>(cmd.radius);
                int y = screen_y(cmd.y1) - static_cast<int>(cmd.radius);
                int size = static_cast<int>(cmd.radius * 2);

                Ellipse(hdc, x, y, x + size, y + size);

                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
                DeleteObject(brush);
            }
            break;

            case Command::Circle:  // окружность (не закрашенная, только контур)
            {
                HPEN pen = CreatePen(PS_SOLID, static_cast<int>(cmd.width),
                    to_windows_color(cmd.color));
                HPEN oldPen = (HPEN)SelectObject(hdc, pen);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

                int x = screen_x(cmd.x1) - static_cast<int>(cmd.radius);
                int y = screen_y(cmd.y1) - static_cast<int>(cmd.radius);
                int size = static_cast<int>(cmd.radius * 2);

                Ellipse(hdc, x, y, x + size, y + size);

                // Если толщина линии > 1, рисуем несколько концентрических окружностей (имитация толстой линии)
                if (cmd.width > 1.0f) {
                    for (int i = 1; i < static_cast<int>(cmd.width); ++i) {
                        int offset = static_cast<int>(i * 0.5f);
                        Ellipse(hdc, x - offset, y - offset,
                            x + size + offset, y + size + offset);
                    }
                }

                SelectObject(hdc, oldBrush);
                SelectObject(hdc, oldPen);
                DeleteObject(pen);
            }
            break;

            case Command::Text:  // вывод текста
            {
                SetTextColor(hdc, to_windows_color(cmd.color));
                SetBkMode(hdc, TRANSPARENT);   // прозрачный фон

                HFONT font = CreateFontA(
                    cmd.font_size, 0, 0, 0, FW_NORMAL,
                    FALSE, FALSE, FALSE, RUSSIAN_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                    DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN,
                    "MS Sans Serif"
                );
                HFONT oldFont = (HFONT)SelectObject(hdc, font);

                TextOutA(hdc, screen_x(cmd.x1), screen_y(cmd.y1),
                    cmd.text.c_str(), static_cast<int>(cmd.text.length()));

                SelectObject(hdc, oldFont);
                DeleteObject(font);
            }
            break;
            }
        }
    };

    // -------------------- Публичный класс Canvas (холст) --------------------

    // Конструктор: создаёт холст заданного размера и заголовка
    Canvas::Canvas(int width, int height, const std::string& title)
        : impl_() {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Canvas width and height must be positive");
        }
        impl_.reset(new Impl(width, height, title));
    }

    // Деструктор: закрывает окно, если оно было открыто
    Canvas::~Canvas() {
        if (impl_) {
            impl_->close_window();
        }
    }

    // Конструктор перемещения
    Canvas::Canvas(Canvas&& other) noexcept
        : impl_(std::move(other.impl_)) {
    }

    // Оператор присваивания перемещением
    Canvas& Canvas::operator=(Canvas&& other) noexcept {
        if (this != &other) {
            impl_ = std::move(other.impl_);
        }
        return *this;
    }

    // Устанавливает цвет фона холста
    void Canvas::set_background(Color color) {
        impl_->background = color;
    }

    // Очищает все команды рисования (стирает рисунок)
    void Canvas::clear() {
        impl_->commands.clear();
    }

    // Добавляет команду рисования линии от (x1,y1) до (x2,y2) указанным цветом и толщиной
    void Canvas::add_line(double x1, double y1, double x2, double y2, Color color, float width, double speed) {
        Command cmd;
        cmd.type = Command::Line;
        cmd.x1 = x1; cmd.y1 = y1;
        cmd.x2 = x2; cmd.y2 = y2;
        cmd.color = color;
        cmd.width = width;
        cmd.speed = speed;
        impl_->commands.push_back(cmd);
    }

    // Добавляет команду рисования точки (закрашенного круга) радиусом radius в позиции (x,y)
    void Canvas::add_dot(double x, double y, float radius, Color color, double speed) {
        Command cmd;
        cmd.type = Command::Dot;
        cmd.x1 = x; cmd.y1 = y;
        cmd.radius = radius;
        cmd.color = color;
        cmd.speed = speed;
        impl_->commands.push_back(cmd);
    }

    // Добавляет команду рисования окружности с центром (x,y) и радиусом radius
    void Canvas::add_circle(double x, double y, float radius, Color color, float width, double speed) {
        Command cmd;
        cmd.type = Command::Circle;
        cmd.x1 = x; cmd.y1 = y;
        cmd.radius = radius;
        cmd.color = color;
        cmd.width = width;
        cmd.speed = speed;
        impl_->commands.push_back(cmd);
    }

    // Добавляет команду вывода текста в позиции (x,y)
    void Canvas::add_text(double x, double y, const std::string& text, int font_size, Color color, double speed) {
        Command cmd;
        cmd.type = Command::Text;
        cmd.x1 = x; cmd.y1 = y;
        cmd.text = text;
        cmd.font_size = font_size;
        cmd.color = color;
        cmd.speed = speed;
        impl_->commands.push_back(cmd);
    }

    // Главный цикл: показывает окно и проигрывает все команды с учётом их скоростей
    // Завершается, когда пользователь закроет окно
    void Canvas::wait_until_close() {
        impl_->create_window();   // создаём окно и контексты

        std::size_t visible_count = 0;   // сколько команд уже показано
        double accumulator = 0.0;        // накопленное время для анимации

        auto last_time = std::chrono::steady_clock::now();

        bool running = true;
        while (running) {
            // Обрабатываем сообщения Windows (закрытие, перемещение окна и т.д.)
            if (!impl_->process_messages()) {
                break;
            }

            // Если окно было уничтожено извне, выходим
            if (!IsWindow(impl_->hwnd)) {
                break;
            }

            // Вычисляем время, прошедшее с прошлого кадра
            auto current_time = std::chrono::steady_clock::now();
            double dt = std::chrono::duration<double>(current_time - last_time).count();
            last_time = current_time;

            // Управляем анимацией: добавляем новые команды в соответствии с их скоростью
            if (visible_count < impl_->commands.size()) {
                const double current_speed = impl_->commands[visible_count].speed;

                if (current_speed <= 0.0) {
                    // скорость 0 или отрицательная → показать все оставшиеся команды мгновенно
                    visible_count = impl_->commands.size();
                }
                else {
                    accumulator += dt;
                    const double interval = 1.0 / current_speed;   // интервал между командами

                    while (visible_count < impl_->commands.size() && accumulator >= interval) {
                        accumulator -= interval;
                        ++visible_count;

                        // Если следующая команда имеет другую скорость, останавливаемся
                        if (visible_count < impl_->commands.size()) {
                            const double next_speed = impl_->commands[visible_count].speed;
                            if (std::fabs(next_speed - current_speed) > 1e-9) {
                                break;
                            }
                        }
                    }
                }
            }

            // Рисуем всё, что накопилось, в памяти
            if (impl_->memory_dc) {
                // Заливаем фон
                HBRUSH bgBrush = CreateSolidBrush(to_windows_color(impl_->background));
                RECT rect = { 0, 0, impl_->width, impl_->height };
                FillRect(impl_->memory_dc, &rect, bgBrush);
                DeleteObject(bgBrush);

                // Рисуем все видимые команды (от 0 до visible_count-1)
                for (std::size_t i = 0; i < visible_count; ++i) {
                    impl_->draw_one(impl_->memory_dc, impl_->commands[i]);
                }

                // Копируем из памяти на экран (двойная буферизация, мерцания нет)
                HDC screen_dc = GetDC(impl_->hwnd);
                BitBlt(screen_dc, 0, 0, impl_->width, impl_->height,
                    impl_->memory_dc, 0, 0, SRCCOPY);
                ReleaseDC(impl_->hwnd, screen_dc);
            }

            // Небольшая задержка, чтобы не нагружать процессор
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }

        impl_->close_window();   // закрываем окно и освобождаем ресурсы
    }

    // -------------------- Класс Turtle (Черепашка) --------------------
    // Управляет движением «черепашки» по холсту, автоматически добавляя команды рисования

    Turtle::Turtle(Canvas& canvas)
        : canvas_(&canvas),
        x_(0.0),               // начальная позиция X (центр холста)
        y_(0.0),               // начальная позиция Y
        heading_deg_(0.0),     // направление: 0° = вправо (восток)
        pen_is_down_(true),    // перо опущено (рисуем при движении)
        pen_color_(Color::Black()),  // цвет пера по умолчанию
        pen_width_(2.0f),      // толщина пера 2 пикселя
        speed_(0.0) {          // скорость по умолчанию (мгновенная)
    }

    // Движение вперёд на расстояние distance (пикселей) в текущем направлении
    void Turtle::forward(double distance) {
        const double rad = deg_to_rad(heading_deg_);
        const double new_x = x_ + std::cos(rad) * distance;
        const double new_y = y_ + std::sin(rad) * distance;

        if (pen_is_down_) {
            canvas_->add_line(x_, y_, new_x, new_y, pen_color_, pen_width_, speed_);
        }

        x_ = new_x;
        y_ = new_y;
    }

    // Движение назад (отрицательное расстояние)
    void Turtle::backward(double distance) {
        forward(-distance);
    }

    // Поворот налево (против часовой стрелки) на angle градусов
    void Turtle::left(double degrees) {
        heading_deg_ += degrees;
    }

    // Поворот направо (по часовой стрелке)
    void Turtle::right(double degrees) {
        heading_deg_ -= degrees;
    }

    // Поднять перо (дальнейшие движения не оставляют следа)
    void Turtle::pen_up() {
        pen_is_down_ = false;
    }

    // Опустить перо (дальнейшие движения рисуют)
    void Turtle::pen_down() {
        pen_is_down_ = true;
    }

    // Переместить черепашку в точку (x,y), рисуя линию, если перо опущено
    void Turtle::go_to(double x, double y) {
        if (pen_is_down_) {
            canvas_->add_line(x_, y_, x, y, pen_color_, pen_width_, speed_);
        }
        x_ = x;
        y_ = y;
    }

    // Установить направление черепашки в градусах (0° = вправо, 90° = вверх)
    void Turtle::set_heading(double degrees) {
        heading_deg_ = degrees;
    }

    // Вернуться в центр (0,0) и смотреть вправо (0°)
    void Turtle::home() {
        go_to(0.0, 0.0);
        heading_deg_ = 0.0;
    }

    // Установить цвет пера
    void Turtle::color(Color color_value) {
        pen_color_ = color_value;
    }

    // Установить толщину пера в пикселях (должна быть > 0)
    void Turtle::width(float pixels) {
        if (pixels <= 0.0f) {
            throw std::invalid_argument("Pen width must be positive");
        }
        pen_width_ = pixels;
    }

    // Установить скорость выполнения команд (команд в секунду)
    // speed = 0 – мгновенно, speed > 0 – анимация
    void Turtle::speed(double commands_per_second) {
        if (commands_per_second < 0.0) {
            throw std::invalid_argument("Speed must be >= 0");
        }
        speed_ = commands_per_second;
    }

    // Нарисовать точку (закрашенный круг) радиусом radius в текущей позиции черепашки
    void Turtle::dot(float radius) {
        if (radius <= 0.0f) {
            throw std::invalid_argument("Dot radius must be positive");
        }
        canvas_->add_dot(x_, y_, radius, pen_color_, speed_);
    }

    // Нарисовать окружность с центром в текущей позиции черепашки и заданным радиусом
    void Turtle::circle(double radius) {
        if (radius <= 0.0) {
            throw std::invalid_argument("Circle radius must be positive");
        }
        canvas_->add_circle(x_, y_, static_cast<float>(radius), pen_color_, pen_width_, speed_);
    }

    // Написать текст в текущей позиции черепашки
    void Turtle::write(const std::string& text, int font_size) {
        if (font_size <= 0) {
            throw std::invalid_argument("Font size must be positive");
        }
        canvas_->add_text(x_, y_, text, font_size, pen_color_, speed_);
    }

    // Геттер: текущая координата X
    double Turtle::x() const {
        return x_;
    }

    // Геттер: текущая координата Y
    double Turtle::y() const {
        return y_;
    }

    // Геттер: текущий угол направления (градусы)
    double Turtle::heading() const {
        return heading_deg_;
    }

}  // namespace edu
