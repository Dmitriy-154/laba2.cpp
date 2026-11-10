#include "massiv.h"
#include <iostream>
#include <sstream>
#include <stdexcept>

// Строгий разбор целого числа.
// std::stoi разбирает только префикс ("12abc" → 12), поэтому проверяем,
// что разобрана вся строка token.
static int integer(const std::string& token) {
    std::size_t end;
    int value = std::stoi(token, &end);
    if (end != token.size()) throw std::runtime_error("ожидалось целое число");
    return value;
}

// Используется полный перебор: left — начало, right — конец.
// Сумма накапливается инкрементально при движении right, поэтому
// сложность O(n^2) по времени и O(1) дополнительной памяти
// Для каждого найденного отрезка печатается
// его содержимое в формате [a, b, c].
static void findSubarrays(const Array& array, long long target) {
    bool found = false;

    for (std::size_t left = 0; left < array.len; ++left) {
        long long sum = 0;
        for (std::size_t right = left; right < array.len; ++right) {
            // Array хранит строки, поэтому каждый элемент приводится к int.
            sum += std::stoi(arrayGet(array, right));

            if (sum != target) continue;

            found = true;
            std::cout << '[';
            for (std::size_t i = left; i <= right; ++i)
                std::cout << (i == left ? "" : ", ") << arrayGet(array, i);
            std::cout << "]\n";
        }
    }

    if (!found) std::cout << "Подмассивов с такой суммой нет\n";
}

int main() {
    Array array;
    int status = 0;
    try {
        std::cout << "Введите массив одной строкой:\n";
        std::string line, token;
        if (!std::getline(std::cin, line)) throw std::runtime_error("нет массива");

        // Разбираем строку на токены и складываем в Array.
        // Числа нормализуются через std::to_string, т.к. Array работает со строками.
        std::istringstream input(line);
        while (input >> token) arrayPush(array, std::to_string(integer(token)));

        std::cout << "Введите искомую сумму:\n";
        if (!std::getline(std::cin, line)) throw std::runtime_error("нет суммы");

        // Для суммы требуется ровно один токен: лишние слова — ошибка.
        std::istringstream targetInput(line);
        std::string extra;
        if (!(targetInput >> token) || (targetInput >> extra)) throw std::runtime_error("нужна одна сумма");

        // target имеет тип long long, но integer возвращает int —
        // фактически допустимы только суммы, помещающиеся в int.
        findSubarrays(array, integer(token));
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        status = 1;
    }

    // Освобождаем динамический массив в любом исходе.
    arrayClear(array);
    return status;
}
