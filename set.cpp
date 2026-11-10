#include "set.h"

// приводим к неотрицательному, прибавляя capacity. 
// защищает от переполнения при касте size_t.
static std::size_t bucketOf(int value, std::size_t capacity) {
    long long index = static_cast<long long>(value) % static_cast<long long>(capacity);
    return static_cast<std::size_t>(index < 0 ? index + static_cast<long long>(capacity) : index);
}

// Перестройка (rehash) при превышении load factor. Создаём вдвое больше бакетов
// в новые цепочки переустановкой указателей next, что даёт O(n) без аллокаций.
// Каждый узел перемещается в голову новой цепочки 
static void rehash(Set& set) {
    std::vector<SList> fresh(set.buckets.size() * 2 + 1);
    for (std::size_t i = 0; i < set.buckets.size(); ++i) {
        SNode* node = set.buckets[i].head;
        while (node) {
            SNode* next = node->next;                 // запоминаем следующий ДО переустановки next
            SList& chain = fresh[bucketOf(std::stoi(node->val), fresh.size())];
            node->next = chain.head;                  // вставка в голову новой цепочки
            chain.head = node;
            if (!chain.tail) chain.tail = node;       // первый элемент цепочки — он же tail
            ++chain.n;
            node = next;
        }
    }
    set.buckets.swap(fresh);                          // старые (уже пустые) бакеты уйдут в fresh
}

// Проверка принадлежности value множеству. Ищем в единственной возможной
// цепочке линейным проходом SList по строковому представлению.
bool setAt(const Set& set, int value) {
    return slistFind(set.buckets[bucketOf(value, set.buckets.size())], std::to_string(value)) >= 0;
}

// Добавление value. Возвращает true, если значение реально добавлено
// (false — если уже было). Порог перестройки: n + 1 > buckets.size(),
// load factor > 1.0 — средняя длина цепочки больше 1, поиск начинает
// деградировать. rehash удваивает число бакетов, возвращая load factor 
bool setAdd(Set& set, int value) {
    if (setAt(set, value)) return false;
    if (set.n + 1 > set.buckets.size()) rehash(set);
    slistPushHead(set.buckets[bucketOf(value, set.buckets.size())], std::to_string(value));
    ++set.n;
    return true;
}

// Удаление value. slistDelValue сам найдёт и вырежет узел в нужной цепочке
// (с правкой head/tail), а setDel лишь скорректирует счётчик n.
bool setDel(Set& set, int value) {
    if (!slistDelValue(set.buckets[bucketOf(value, set.buckets.size())], std::to_string(value))) return false;
    --set.n;
    return true;
}

// Выгрузка всех значений множества в Array (в произвольном порядке —
// обход бакетов и цепочек). Результат — строки, как они хранятся внутри.
// Старое содержимое values очищается (arrayClear).
void setValues(const Set& set, Array& values) {
    arrayClear(values);
    for (std::size_t i = 0; i < set.buckets.size(); ++i)
        for (SNode* node = set.buckets[i].head; node; node = node->next) arrayPush(values, node->val);
}

// Полная очистка содержимого: освобождаем все цепочки, n = 0.
// Вектор бакетов при этом НЕ сжимается — capacity сохраняется для переиспользования.
void setClear(Set& set) {
    for (std::size_t i = 0; i < set.buckets.size(); ++i) slistClear(set.buckets[i]);
    set.n = 0;
}