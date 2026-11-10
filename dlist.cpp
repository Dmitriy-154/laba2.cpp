#include "dlist.h"
#include <stdexcept>

using namespace std;

// Поиск узла по значению; возвращает nullptr, если не найден.
static DNode* dlistFindNode(const DList& list, const string& v) {
    for (DNode* c = list.head; c; c = c->next) if (c->val == v) return c;
    return nullptr;
}

// Поиск узла с обязательным наличием: если не найден — исключение.
static DNode* dlistMustFind(const DList& list, const string& v) {
    DNode* t = dlistFindNode(list, v);
    if (!t) throw runtime_error("value not found: " + v);
    return t;
}

// Отвязывает узел x от списка и удаляет его, возвращая его значение.
// корректную подмену head/tail, если удаляется крайний элемент.
static string dlistUnlink(DList& list, DNode* x) {  
    string r = x->val;
    if (x->prev) x->prev->next = x->next; else list.head = x->next; // x был первым?
    if (x->next) x->next->prev = x->prev; else list.tail = x->prev; // x был последним?
    delete x;
    --list.n;
    return r;
}

// Вставка нового узла со значением v сразу после узла t (t не должен быть nullptr).
// Обновляет tail, если t был последним.
static void dlistInsertAfter(DList& list, DNode* t, const string& v) {
    DNode* x = new DNode{v, t, t->next};
    if (t->next) t->next->prev = x; else list.tail = x;
    t->next = x;
    ++list.n;
}

// Вставка в начало списка.
void dlistPushHead(DList& list, const string& v) {
    DNode* x = new DNode{v, nullptr, list.head};
    if (list.head) list.head->prev = x; else list.tail = x; // если был пуст — x и есть tail
    list.head = x;
    ++list.n;
}

// Вставка в конец списка.
void dlistPushTail(DList& list, const string& v) {
    DNode* x = new DNode{v, list.tail, nullptr};
    if (list.tail) list.tail->next = x; else list.head = x; // если был пуст — x и есть head
    list.tail = x;
    ++list.n;
}

// Вставка после первого узла со значением target.
void dlistPushAfter(DList& list, const string& target, const string& v) { dlistInsertAfter(list, dlistMustFind(list, target), v); }

// Вставка перед первым узлом со значением target.
// Если target — голова, проще вставить в голову; иначе — после предыдущего.
void dlistPushBefore(DList& list, const string& target, const string& v) {
    DNode* t = dlistMustFind(list, target);
    if (t == list.head) dlistPushHead(list, v); else dlistInsertAfter(list, t->prev, v);
}

// Удаление первого элемента (с проверкой на пустоту).
string dlistPopHead(DList& list) { if (!list.head) throw runtime_error("list is empty"); return dlistUnlink(list, list.head); }

// Удаление последнего элемента (проверка на пустоту по head, т.к. head==nullptr ⇔ пусто).
string dlistPopTail(DList& list) { if (!list.head) throw runtime_error("list is empty"); return dlistUnlink(list, list.tail); }

// Удаление элемента, следующего за target.
string dlistDelAfter(DList& list, const string& target) {
    DNode* t = dlistMustFind(list, target);
    if (!t->next) throw runtime_error("no element after " + target);
    return dlistUnlink(list, t->next);
}

// Удаление элемента, предшествующего target.
string dlistDelBefore(DList& list, const string& target) {
    DNode* t = dlistMustFind(list, target);
    if (!t->prev) throw runtime_error("no element before " + target);
    return dlistUnlink(list, t->prev);
}

// Удаление первого вхождения значения v. Возвращает true, если что-то удалили.
bool dlistDelValue(DList& list, const string& v) {
    DNode* x = dlistFindNode(list, v);
    if (x) dlistUnlink(list, x);
    return x != nullptr;
}

// Индекс первого вхождения значения v (0-based) или -1, если не найдено.
int dlistFind(const DList& list, const string& v) {
    int i = 0;
    for (DNode* c = list.head; c; c = c->next, ++i) if (c->val == v) return i;
    return -1;
}

// Доступ по индексу (0-based) с проверкой выхода за границы.
// Возвращает ссылку на значение внутри узла — валидна, пока узел жив.
const string& dlistAt(const DList& list, size_t i) {
    if (i >= list.n) throw runtime_error("index out of range: " + to_string(i));
    DNode* c = list.head;
    while (i--) c = c->next;
    return c->val;
}

// Полная очистка списка: удаляем с головы, пока не опустеет.
void dlistClear(DList& list) { while (list.head) dlistPopHead(list); }