#include "hashtable.h"
#include <stdexcept>
#include <limits>

// Вычисление индекса для ключа. which = 0 — «первая» хеш-функция, which = 1 — «вторая»
// Если вторая функция дала тот же индекс, что и первая, — сдвигаем на 1 по модулю,
// иначе у кукушкина элемента было бы только одно возможное место.
static std::size_t hashIndex(const HashTable& table, const std::string& key, unsigned int which) {
    unsigned int hash = 2166136261u ^ (table.seed * 2654435761u + which * 2246822519u);
    for (unsigned char c : key) { hash ^= c; hash *= 16777619u; }
    hash ^= hash >> 16; hash *= 2246822519u; hash ^= hash >> 13;
    std::size_t index = hash % table.capacity;
    if (which && index == hashIndex(table, key, 0)) index = (index + 1) % table.capacity;
    return index;
}

// Безопасное удвоение capacity: защита от переполнения size_t.
static std::size_t doubled(std::size_t capacity) {
    if (capacity > std::numeric_limits<std::size_t>::max() / 2) throw std::runtime_error("слишком большая таблица");
    return capacity * 2;
}

// Полное освобождение ресурсов: чистим все цепочки (или ячейки), обнуляем счётчики.
// Используется и в hashInit, и при rebuild (перед подменой содержимого).
void hashClear(HashTable& table) {
    for (std::size_t i = 0; i < table.buckets.size(); ++i) slistClear(table.buckets[i]);
    table.buckets.clear(); table.cells.clear();
    table.n = table.capacity = 0;
}

// Инициализация: минимальная ёмкость 4, seed = 1, счётчики в ноль.
// Аллоцируется ровно один из двух массивов — buckets или cells — по режиму.
void hashInit(HashTable& table, std::size_t capacity, bool cuckoo) {
    hashClear(table);
    if (capacity < 4) capacity = 4;
    table.cuckoo = cuckoo; table.capacity = capacity;
    table.seed = 1; table.probes = table.rebuilds = 0;
    if (cuckoo) table.cells.resize(capacity);
    else table.buckets.resize(capacity);
}

// Разбор строки "key\nvalue" из цепочного узла в структуру HashCell.
// Формат фиксирован: первый '\n' отделяет ключ от значения (ключ не содержит '\n').
static HashCell unpack(const std::string& text) {
    std::size_t separator = text.find('\n');
    return HashCell{text.substr(0, separator), std::stoi(text.substr(separator + 1)), true};
}

// Поиск узла в цепочке по ключу; попутно инкрементит probes.
// Возвращает узел (для последующей правки/удаления) или nullptr.
static SNode* findChain(HashTable& table, const std::string& key) {
    SList& chain = table.buckets[hashIndex(table, key, 0)];
    for (SNode* node = chain.head; node; node = node->next) {
        ++table.probes;
        if (node->val.substr(0, node->val.find('\n')) == key) return node;
    }
    return nullptr;
}

// Кукушкина вставка: пробуем положить incoming в его первое место;
// если занято — вытесняем «жильца», кладём incoming на его место,
// а вытесненный элемент пытаемся устроить на его альтернативное место.
// Так продолжается до свободной ячейки или до исчерпания шагов (цикл вытеснений).
// Возвращает false, если за 2*capacity шагов место не нашлось — вызывающему нужен rebuild.
static bool place(HashTable& table, HashCell& incoming) {
    std::size_t index = hashIndex(table, incoming.key, 0);
    for (std::size_t step = 0; step < table.capacity * 2; ++step) {
        ++table.probes;
        if (!table.cells[index].used) { table.cells[index] = incoming; return true; }
        HashCell displaced = table.cells[index];
        table.cells[index] = incoming;
        incoming = displaced;
        // Альтернативное место: если сейчас элемент стоит на «первом» слоте,
        // пытаемся переставить его на «второй», и наоборот.
        std::size_t first = hashIndex(table, incoming.key, 0);
        index = index == first ? hashIndex(table, incoming.key, 1) : first;
    }
    return false;
}

// Полная перестройка таблицы: собираем все текущие элементы 
// затем вставляем их в свежую таблицу. Если кукушкина вставка не удалась —
// меняем seed (получаем другие хеш-функции) и пробуем снова
static void rebuild(HashTable& table, std::size_t capacity, const HashCell* missing = nullptr) {
    std::vector<HashCell> entries;
    if (table.cuckoo) {
        for (std::size_t i = 0; i < table.capacity; ++i)
            if (table.cells[i].used) entries.push_back(table.cells[i]);
    } else {
        for (std::size_t i = 0; i < table.capacity; ++i)
            for (SNode* node = table.buckets[i].head; node; node = node->next) entries.push_back(unpack(node->val));
    }
    if (missing) entries.push_back(*missing);
    HashTable fresh;
    unsigned int seed = table.seed;
    try {
        for (int attempt = 0;; ++attempt) {
            hashInit(fresh, capacity, table.cuckoo);
            fresh.seed = ++seed;                  // новый seed → другие индексы
            bool success = true;
            for (std::size_t i = 0; i < entries.size(); ++i) {
                if (fresh.cuckoo) {
                    HashCell cell = entries[i];
                    if (!place(fresh, cell)) { success = false; break; }
                } else slistPushHead(fresh.buckets[hashIndex(fresh, entries[i].key, 0)],
                                     entries[i].key + "\n" + std::to_string(entries[i].value));
            }
            table.probes += fresh.probes;
            if (success) break;
            if (attempt % 8 == 7) capacity = doubled(capacity); // 8 неудач подряд — увеличиваем
        }
        // Атомарная (с точки зрения API) подмена содержимого.
        hashClear(table);
        table.buckets.swap(fresh.buckets); table.cells.swap(fresh.cells);
        table.capacity = fresh.capacity; table.n = entries.size(); table.seed = fresh.seed;
        ++table.rebuilds;
    } catch (...) { hashClear(fresh); throw; } // при исключении не теряем данные
}

// Поиск значения по ключу. В цепочном режиме — обход цепочки; в кукушкином —
// проверяем ровно две возможные ячейки (по одной на каждую хеш-функцию).
bool hashGet(HashTable& table, const std::string& key, int& value) {
    if (!table.capacity) throw std::runtime_error("таблица не инициализирована");
    if (!table.cuckoo) {
        SNode* node = findChain(table, key);
        if (!node) return false;
        value = unpack(node->val).value;
        return true;
    }
    for (unsigned int which = 0; which < 2; ++which) {
        ++table.probes;
        const HashCell& cell = table.cells[hashIndex(table, key, which)];
        if (cell.used && cell.key == key) { value = cell.value; return true; }
    }
    return false;
}

// Вставка или обновление. Ключ не должен быть пустым и не должен содержать '\n'.
void hashSet(HashTable& table, const std::string& key, int value) {
    if (!table.capacity || key.empty() || key.find('\n') != std::string::npos)
        throw std::runtime_error("неверная таблица или ключ");
    if (!table.cuckoo) {
        // Цепочный режим: если ключ есть — обновляем значение, иначе — в голову цепочки.
        SNode* node = findChain(table, key);
        if (node) { node->val = key + "\n" + std::to_string(value); return; }
        slistPushHead(table.buckets[hashIndex(table, key, 0)], key + "\n" + std::to_string(value));
        ++table.n;
    } else {
        // Кукушкин режим: если ключ уже лежит в одной из двух ячеек — обновляем.
        for (unsigned int which = 0; which < 2; ++which) {
            ++table.probes;
            HashCell& cell = table.cells[hashIndex(table, key, which)];
            if (cell.used && cell.key == key) { cell.value = value; return; }
        }
        HashCell cell{key, value, true};
        if (place(table, cell)) ++table.n;
        else rebuild(table, table.capacity, &cell); // цикл вытеснений — перестройка с новым seed
    }
    // Порог заполнения ~90%: n >= capacity - capacity/10 — перерастаем.
    if (table.n >= table.capacity - table.capacity / 10) rebuild(table, doubled(table.capacity));
}

// Удаление по ключу. В кукушкином режиме ячейку просто помечаем used = false
// (без «дыр»-цепочек — открытая адресация). В цепочном — вырезаем узел из SList,
// аккуратно поправив head и tail, если узел крайний.
bool hashDel(HashTable& table, const std::string& key) {
    if (!table.capacity) throw std::runtime_error("таблица не инициализирована");
    if (table.cuckoo) {
        for (unsigned int which = 0; which < 2; ++which) {
            ++table.probes;
            HashCell& cell = table.cells[hashIndex(table, key, which)];
            if (cell.used && cell.key == key) { cell.used = false; --table.n; return true; }
        }
        return false;
    }
    SList& chain = table.buckets[hashIndex(table, key, 0)];
    SNode* before = nullptr;
    for (SNode* node = chain.head; node; node = node->next) {
        ++table.probes;
        if (node->val.substr(0, node->val.find('\n')) == key) {
            if (before) before->next = node->next; else chain.head = node->next; // правим head
            if (chain.tail == node) chain.tail = before;                          // правим tail
            delete node; --chain.n; --table.n;
            return true;
        }
        before = node;
    }
    return false;
}