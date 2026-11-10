#include "stack.h"
#include <iostream>
#include <stdexcept>
#include <cctype>

// Приоритеты операций:
//   !  — 3 (унарный, самый высокий)
//   &  — 2
//   |  — 1
//   ^  — 1
//   a | b ^ c вычисляется как (a | b) ^ c.
static int priority(char operation) {
    if (operation == '!') return 3;
    return operation == '&' ? 2 : 1;
}

// Применение одной операции с вершины стека операций к операндам.
// Для унарного ! снимаем один операнд; для &, |, ^ — два.
// Операнды снимаются в обратном порядке: сначала правый, затем левый.
static void apply(Stack& values, Stack& operations) {
    char operation = stackPop(operations)[0];
    int right = std::stoi(stackPop(values));

    // Унарный !: логическое отрицание, результат 0/1.
    if (operation == '!') { stackPush(values, std::to_string(!right)); return; }

    int left = std::stoi(stackPop(values));
    // Бинарные операции над 0/1; для таких операндов &, |, ^ дают 0/1.
    int result = operation == '&' ? (left & right) : operation == '|' ? (left | right) : (left ^ right);
    stackPush(values, std::to_string(result));
}

// Вычисление выражения по алгоритму «два стека» (shunting-yard):
// values — операнды (строки "0"/"1"), operations — операции и скобки.
// Инвариант needValue:
//   true  — сейчас ожидается операнд (начало, после операции, после '(' или '!');
//   false — только что прочитан операнд или закрывающая скобка.
static int evaluate(const std::string& expression) {
    Stack values, operations;
    bool needValue = true;

    try {
        for (char c : expression) {
            if (std::isspace(static_cast<unsigned char>(c))) continue;

            if (c == '0' || c == '1') {
                // Число допустимо только там, где ожидается операнд.
                if (!needValue) throw std::runtime_error("нет операции между числами");
                stackPush(values, std::string(1, c));
                needValue = false;
            } else if (c == '(' || c == '!') {
                // '(' и унарный '!' допустимы только перед операндом.
                if (!needValue) throw std::runtime_error("нет операции перед знаком или скобкой");
                stackPush(operations, std::string(1, c));
                // needValue остаётся true: после '(' или '!' всё ещё ждём операнд.
            } else if (c == ')') {
                // Перед ')' должен быть операнд, иначе скобка пустая/неверная.
                if (needValue) throw std::runtime_error("пустая или неверная скобка");

                // Выполняем все операции до соответствующей '('.
                while (operations.l.n && stackGet(operations) != "(") apply(values, operations);
                if (!operations.l.n) throw std::runtime_error("лишняя закрывающая скобка");
                stackPop(operations); // удаляем '('
                // needValue остаётся false: результат скобки — готовый операнд.
            } else if (c == '&' || c == '|' || c == '^') {
                // Бинарная операция требует готового левого операнда.
                if (needValue) throw std::runtime_error("нет левого операнда");

                // Снимаем со стека операции с приоритетом >= текущей.
                // '(' — барьер, его не трогаем. Равенство приоритетов даёт
                // левую ассоциативность для | и ^.
                while (operations.l.n && stackGet(operations) != "(" &&
                       priority(stackGet(operations)[0]) >= priority(c)) apply(values, operations);

                stackPush(operations, std::string(1, c));
                needValue = true; // после операции снова ждём операнд
            } else {
                throw std::runtime_error("допустимы только 0, 1, !, &, |, ^ и скобки");
            }
        }

        // Выражение не должно заканчиваться на ожидании операнда.
        if (needValue) throw std::runtime_error("выражение не закончено");

        // Довыполняем оставшиеся операции; незакрытая '(' — ошибка.
        while (operations.l.n) {
            if (stackGet(operations) == "(") throw std::runtime_error("скобка не закрыта");
            apply(values, operations);
        }

        int result = std::stoi(stackPop(values));
        stackClear(values); stackClear(operations);
        return result;
    } catch (...) {
        // При любой ошибке очищаем оба стека и пробрасываем исключение.
        stackClear(values); stackClear(operations);
        throw;
    }
}

int main() {
    std::string expression;
    while (true) {
        std::cout << "Введите выражение:\n";
        if (!std::getline(std::cin, expression) || expression == "EXIT" || expression == "exit") break;
        try {
            std::cout << evaluate(expression) << '\n';
        } catch (const std::exception& error) {
            std::cerr << "Ошибка: " << error.what() << '\n';
        }
    }
    return 0;
}