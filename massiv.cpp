#include "massiv.h"
#include <stdexcept>

// Проверка индекса для операций доступа (get/set/erase):
// допустимы только существующие элементы, index < len.
static void check(const Array& array, std::size_t index) {
    if (index >= array.len) throw std::runtime_error("индекс вне диапазона");
}

// Вставка элемента v в позицию index. Допускает index == len (вставка в конец).
// При нехватке места буфер перевыделяется удвоением (начиная с 4) — это даёт
// амортизированную O(1) на push и O(len) на произвольную вставку (сдвиг вправо).
void arrayInsert(Array& array, std::size_t index, const std::string& v) {
    if (index > array.len) throw std::runtime_error("индекс вне диапазона");
    std::string value = v;                 // копия: v может ссылаться внутрь array.data
    if (array.len == array.cap) {
        // Рост: cap = max(4, cap * 2). Сильная гарантия исключений: сначала
        // выделяем и копируем в новый буфер; только после успеха освобождаем старый.
        std::size_t capacity = array.cap ? array.cap * 2 : 4;
        std::string* data = new std::string[capacity];
        try {
            for (std::size_t i = 0; i < array.len; ++i) data[i] = array.data[i];
        } catch (...) { delete[] data; throw; }   // при сбое копирования не теряем данные
        delete[] array.data;
        array.data = data;
        array.cap = capacity;
    }
    // Сдвиг хвоста [index, len) на одну позицию вправо, чтобы освободить место.
    for (std::size_t i = array.len; i > index; --i) array.data[i] = array.data[i - 1];
    array.data[index] = value;
    ++array.len;
}

// Вставка в конец — переиспользует arrayInsert (index == len).
void arrayPush(Array& array, const std::string& value) { arrayInsert(array, array.len, value); }

// Доступ по индексу: возвращает ссылку на элемент внутри массива —
// валидна, пока элемент не удалён и массив не перевыделен.
const std::string& arrayGet(const Array& array, std::size_t index) { check(array, index); return array.data[index]; }

// Присваивание по индексу (элемент уже существует, длина не меняется).
void arraySet(Array& array, std::size_t index, const std::string& value) { check(array, index); array.data[index] = value; }

// Удаление элемента по индексу: возвращает удалённое значение,
// затем сдвигает хвост [index+1, len) влево на одну позицию.
// Ёмкость не уменьшается; пустые слоты остаются сконструированными (не разрушаются).
std::string arrayErase(Array& array, std::size_t index) {
    check(array, index);
    std::string value = array.data[index];        // копия до сдвига, иначе значение затрётся
    for (std::size_t i = index; i + 1 < array.len; ++i) array.data[i] = array.data[i + 1];
    --array.len;
    return value;
}

// Полное освобождение: буфер удаляется, массив приводится к пустому (len = cap = 0).
// После clear массив снова способен принимать элементы (arrayInsert сработает заново).
void arrayClear(Array& array) {
    delete[] array.data;
    array.data = nullptr;
    array.len = array.cap = 0;
}