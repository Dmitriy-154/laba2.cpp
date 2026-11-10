#include "set.h"
#include <iostream>
#include <stdexcept>
#include <limits>
#include <cstdlib>

struct Partition {
    Array values;          // исходные числа (строки) в порядке добавления
    Array current;         // текущая маска: "1" — в группе 1, "0" — в группе 0
    Array best;            // найденная маска
    long long total = 0;   // сумма всех чисел
    long long difference = std::numeric_limits<long long>::max(); // лучшая разница
};

// Рекурсивный перебор: для элемента index решаем, включить его в группу 1
// (sum += value, count++) или в группу 0 (sum, count без изменений).
// sum — сумма уже набранной группы 1; count — сколько элементов в ней.
static void search(Partition& state, std::size_t index, long long sum, std::size_t count) {
    // Отсечение: идеальная разница — 0 (total чётно) или 1 (total нечётно).
    // Если она уже достигнута, дальше искать бессмысленно.
    if (state.difference == state.total % 2) return;

    // Дошли до конца: рассмотрели все элементы.
    if (index == state.values.len) {
        // Не допускаем «пустую» группу 0 (все элементы в группе 1) — иначе
        // разбиение не считается полноценным.
        if (count == state.values.len) return;
        // Разница сумм: |total - 2*sum|, где sum — сумма группы 1.
        long long difference = std::llabs(state.total - 2 * sum);
        if (difference < state.difference) {
            // Запоминаем лучшую маску, копируя current → best поэлементно.
            state.difference = difference;
            for (std::size_t i = 0; i < state.values.len; ++i)
                arraySet(state.best, i, arrayGet(state.current, i));
        }
        return;
    }

    // Ветвь 1: включаем элемент index в группу 1.
    arraySet(state.current, index, "1");
    search(state, index + 1, sum + std::stoi(arrayGet(state.values, index)), count + 1);

    // Ветвь 2: оставляем элемент index в группе 0.
    arraySet(state.current, index, "0");
    search(state, index + 1, sum, count);
}

// Основная процедура: подготовка данных, запуск перебора и вывод результата.
static void partition(const Set& set, Partition& state) {
    if (set.n < 2) throw std::runtime_error("нужно хотя бы два различных натуральных числа");

    // Переносим числа из Set в Array (в произвольном порядке — множество не упорядочено).
    setValues(set, state.values);

    // Проверяем натуральность, считаем total и инициализируем маски нулями.
    for (std::size_t i = 0; i < state.values.len; ++i) {
        int value = std::stoi(arrayGet(state.values, i));
        if (value <= 0) throw std::runtime_error("нужны натуральные числа");
        state.total += value;
        arrayPush(state.current, "0");
        arrayPush(state.best, "0");
    }

    // Симметрия: фиксируем первый элемент в группе 1.
    // Любое разбиение и его «зеркало» дают одинаковую разницу, поэтому
    // можно не перебирать вариант, где первый элемент попал в группу 0.
    arraySet(state.current, 0, "1");
    search(state, 1, std::stoi(arrayGet(state.values, 0)), 1);

    // сначала группа 1 (маска "1"), затем группа 0 (маска "0").
    for (int group = 1; group >= 0; --group) {
        std::cout << '{';
        bool first = true;
        for (std::size_t i = 0; i < state.values.len; ++i)
            if (std::stoi(arrayGet(state.best, i)) == group) {
                std::cout << (first ? "" : ", ") << arrayGet(state.values, i);
                first = false;
            }
        std::cout << "}\n";
    }
    std::cout << "Разница: " << state.difference << '\n';
}

int main() {
    Set set;
    Partition state;
    int status = 0;
    try {
        std::cout << "Введите количество чисел, затем элементы:\n";

        // Строгий разбор количества: std::stoi допускает хвост, проверяем end.
        std::string countToken; std::size_t countEnd;
        if (!(std::cin >> countToken)) throw std::runtime_error("нет количества");
        int n = std::stoi(countToken, &countEnd);
        if (countEnd != countToken.size() || n < 2) throw std::runtime_error("неверное количество");

        // Читаем n натуральных чисел. Дубликаты отбрасываются setAdd —
        // в переборе участвуют только различные числа.
        for (int i = 0; i < n; ++i) {
            std::string token; std::size_t end;
            if (!(std::cin >> token)) throw std::runtime_error("не хватает чисел");
            int value = std::stoi(token, &end);
            if (end != token.size() || value <= 0) throw std::runtime_error("нужны натуральные числа");
            setAdd(set, value);
        }

        partition(set, state);
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        status = 1;
    }

    // Освобождаем все динамические структуры (Array/Set) в любом исходе.
    arrayClear(state.values); arrayClear(state.current); arrayClear(state.best); setClear(set);
    return status;
}
