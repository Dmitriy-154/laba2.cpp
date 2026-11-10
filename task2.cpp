#include "set.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <cerrno>

// Строгий разбор целого числа: std::stoi принимает префикс ("12abc" → 12),
// поэтому дополнительно проверяем, что разобрана вся строка token.
static int integer(const std::string& token) {
    std::size_t end;
    int value = std::stoi(token, &end);
    if (end != token.size()) throw std::runtime_error("ожидалось целое число");
    return value;
}

// Выполнение одной команды над множеством.
// Формат запроса: ровно "<команда> <целое число>" 
// changed = true только если множество реально изменилось (SETADD добавил,
// SETDEL удалил). Для SET_AT файл перезаписывать не нужно.
static bool execute(Set& set, const std::string& query, bool& changed) {
    std::istringstream input(query);
    std::string command, token, extra;

    // Требуем ровно два токена: команду и число. Третий токен — ошибка.
    if (!(input >> command >> token) || (input >> extra)) throw std::runtime_error("команда и одно число");

    int value = integer(token);
    bool result;
    changed = false;

    if (command == "SETADD") result = changed = setAdd(set, value);      // true, если добавили
    else if (command == "SETDEL") result = changed = setDel(set, value); // true, если удалили
    else if (command == "SET_AT") result = setAt(set, value);            // проверка наличия, без изменения
    else throw std::runtime_error("команды: SETADD, SETDEL, SET_AT");

    return result;
}

int main(int argc, char* argv[]) {
    Set set;

    try {
        std::string filename, query;

        // Ручной разбор аргументов: поддерживаются ровно --file <путь> и --query <запрос>.
        // Повторный --file/--query или неизвестный аргумент — ошибка.
        for (int i = 1; i < argc; ++i) {
            std::string option = argv[i];
            if (option == "--file" && i + 1 < argc && filename.empty()) filename = argv[++i];
            else if (option == "--query" && i + 1 < argc && query.empty()) query = argv[++i];
            else throw std::runtime_error("./task2 --file путь --query 'команда число'");
        }

        if (filename.empty() || query.empty()) throw std::runtime_error("нужны --file и --query");

        // Открываем файл. Если файла нет (ENOENT) — это допустимо: считаем множество пустым.
        // Любая другая ошибка открытия — исключение.
        errno = 0;
        std::ifstream file(filename);
        if (!file && errno != ENOENT) throw std::runtime_error("не удалось открыть файл");

        // Файл содержит целые числа, разделённые пробельными символами.
        // setAdd сам отбрасывает дубликаты.
        std::string token;
        while (file >> token) setAdd(set, integer(token));

        // Проверяем именно bad(): fail()/eof() при штатном конце файла — норма.
        if (file.bad()) throw std::runtime_error("ошибка чтения файла");
        file.close();

        bool changed;
        bool result = execute(set, query, changed);

        // Перезаписываем файл только если множество изменилось.
        // Порядок элементов — внутренний порядок хеш-таблицы (не сортировка).
        if (changed) {
            std::ofstream output(filename);
            if (!output) throw std::runtime_error("не удалось записать файл");

            for (std::size_t i = 0; i < set.buckets.size(); ++i)
                for (SNode* node = set.buckets[i].head; node; node = node->next)
                    output << node->val << ' ';
            output << '\n';
            output.close();

            if (!output) throw std::runtime_error("ошибка записи файла");
        }

        // Результат команды: TRUE/FALSE.
        std::cout << (result ? "TRUE\n" : "FALSE\n");

        setClear(set);
    } catch (const std::exception& error) {
        // При любой ошибке освобождаем ресурсы множества и сообщаем в stderr.
        setClear(set);
        std::cerr << "Ошибка: " << error.what() << '\n';
        return 1;
    }
}