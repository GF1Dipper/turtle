[readme.txt](https://github.com/user-attachments/files/26868567/readme.txt)
для запуска в проекте в vs должны быть 
файл:
1.  edu_turtle.hpp
2   edu_turtle.cpp



обязательные параметры в main
    // Создаём холст 800x600 с серым фоном
    edu::Canvas canvas(800, 600, "Edu Turtle");
    canvas.set_background(edu::Color::Gray());
 
    // Создаём черепаху
    edu::Turtle t(canvas);
 
    // Настраиваем перо
    t.speed(20);                    // 20 команд в секунду
    t.color(edu::Color::Blue());    // Синий цвет
    t.width(3.0f);                  // Толщина 3 пикселя
    t.go_to(0, 0);
    |
    |
    |
    |
    |
    |
    |  [сам код]
    |
    |
    |
    |
    canvas.wait_until_close(); // Ждём закрытия окна




Создание и управление окном


1.Canvas(int width, int height, const std::string& title)
Создаёт окно для рисования.

Параметры:

width — ширина окна

height — высота окна

title — заголовок окна

Пример:
edu::Canvas canvas(800, 600, "Черепашка");



2.void wait_until_close()
Запускает окно и проигрывает все команды.

Пример:

---

3.void clear()
Удаляет все команды рисования.

пример:
canvas.clear();
___________________________________________________________________________________


КЛАСС COLOR (ЦВЕТ)
1. Создание цвета
Color(r, g, b, a = 255)
Создаёт цвет в формате RGBA.

пример:
edu::Color c(255, 0, 0); // красный


2. Готовые цвета
Color::Black()

Color::White()

Color::Red()

Color::Green()

Color::Blue()

Color::Yellow()

Color::Orange()

Color::Purple()

Color::Gray()

Пример:
t.color(edu::Color::Blue());

__________________________________________________________________________________

КЛАСС TURTLE (ЧЕРЕПАШКА)
Черепашка рисует на Canvas, выполняя команды.





РАЗДЕЛ 1 — ДВИЖЕНИЕ


void forward(double distance)
Движение вперёд.

пример:
t.forward(100);


void backward(double distance)
Движение назад.

пример:
t.backward(50);


void left(double degrees)
Поворот против часовой стрелки.

пример:
t.left(90);


void right(double degrees)
Поворот по часовой стрелке.

пример:
t.right(45);
aaaaa

void go_to(double x, double y)
Перемещение в точку (x, y).

Если перо опущено — рисует линию.

пример:
t.go_to(100, 200);



void home()
Возвращает черепашку в центр (0,0) и поворачивает вправо.

пример:
t.home();






РАЗДЕЛ 2 — ПЕРО

void pen_up()
Поднять перо — черепашка двигается, но не рисует.

пример;
t.pen_up();


void pen_down()
Опустить перо — черепашка снова рисует.

пример:
t.pen_down();


void color(Color c)
Установить цвет пера.

пример:
t.color(edu::Color::Red());


void width(float pixels)
Установить толщину линии.

пример:
t.width(3.0f);я





РАЗДЕЛ 3 — РИСОВАНИЕ


void dot(float radius)
Рисует закрашенный круг в текущей позиции.

пример:
t.dot(10);


void circle(double radius)
Рисует окружность.

пример:
t.circle(50);






АЗДЕЛ 4 — ТЕКСТ


void write(const std::string& text, int font_size)
Пишет текст.

пример:
t.write("Hello!", 24);





РАЗДЕЛ 5 — СКОРОСТЬ


void speed(double commands_per_second)
Устанавливает скорость анимации.

0 — мгновенно

>0 — количество команд в секунду

пример:
t.speed(5);
