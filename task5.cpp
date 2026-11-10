#include "tree.h"
#include "queue.h"
#include "stack.h"
#include <iostream>
#include <stdexcept>

static void zigzag(const CBTree& tree) {
    std::vector<TNode*> nodes = treeNodes(tree); // узлы в порядке уровней (0 — корень)
    Queue queue;      // индексы узлов текущего уровня
    Stack reversed;   // буфер для разворота уровня

    try {
        // Пустое дерево — печатаем только перевод строки.
        if (!nodes.empty()) queuePush(queue, "0"); 

        bool reverse = false;  // чётные уровни (0, 2, 4...) — слева направо
        bool first = true;     // для пробелов между числами в выводе

        // в цикле for мы вынимаем ровно width узлов, а добавляем — детей
        // (они окажутся уже в следующем «слое» очереди).
        while (queue.l.n) {
            std::size_t width = queue.l.n;

            for (std::size_t i = 0; i < width; ++i) {
                // Индекс приходит строкой (Queue строковая) — приводим обратно.
                std::size_t index = static_cast<std::size_t>(std::stoi(queuePop(queue)));

                // Чётный уровень — печатаем сразу; нечётный — откладываем в стек.
                if (reverse) stackPush(reversed, nodes[index]->key);
                else {
                    std::cout << (first ? "" : " ") << nodes[index]->key;
                    first = false;
                }

                // Дети добавляются в порядке left→right, чтобы на следующем
                // уровне (если он будет «прямым») порядок остался корректным.
                // Для «обратного» уровня reverse-стек сам развернёт порядок.
                if (nodes[index]->left)  queuePush(queue, std::to_string(index * 2 + 1));
                if (nodes[index]->right) queuePush(queue, std::to_string(index * 2 + 2));
            }

            // Сливаем отложенный (перевёрнутый) уровень в вывод.
            while (reversed.l.n) {
                std::cout << (first ? "" : " ") << stackPop(reversed);
                first = false;
            }

            reverse = !reverse; // следующий уровень — в другую сторону
        }
        std::cout << '\n';
    } catch (...) {
        // При исключении освобождаем вспомогательные структуры и пробрасываем.
        queueClear(queue); stackClear(reversed); throw;
    }
}

int main() {
    CBTree tree;
    int status = 0;
    try {
        std::cout << "Введите количество узлов, затем значения по уровням:\n";

        // Строгий разбор количества: std::stoi допускает хвост — проверяем end.
        std::string countToken; std::size_t countEnd;
        if (!(std::cin >> countToken)) throw std::runtime_error("нет количества");
        int n = std::stoi(countToken, &countEnd);
        if (countEnd != countToken.size() || n < 0) throw std::runtime_error("неверное количество");

        // Вставляем n значений в дерево. treeInsert сам поддерживает
        // инвариант полного дерева (вставка в первый свободный слот по уровням).
        for (int i = 0; i < n; ++i) {
            std::string token; std::size_t end;
            if (!(std::cin >> token)) throw std::runtime_error("не хватает узлов");
            int value = std::stoi(token, &end);
            if (end != token.size()) throw std::runtime_error("нужны целые числа");
            treeInsert(tree, std::to_string(value));
        }

        zigzag(tree);
    } catch (const std::exception& error) {
        std::cerr << "Ошибка: " << error.what() << '\n';
        status = 1;
    }

    treeClear(tree); // освобождаем дерево в любом исходе
    return status;
}