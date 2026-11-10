#include "slist.h"
#include <stdexcept>

using namespace std;

// Поиск узла по значению; nullptr, если не найден.
static SNode* slistFindNode(const SList& list, const string& v) {
    for (SNode* c = list.head; c; c = c->next) if (c->val == v) return c;
    return nullptr;
}

// Поиск с обязательным наличием: если не найден — исключение.
static SNode* slistMustFind(const SList& list, const string& v) {
    SNode* t = slistFindNode(list, v);
    if (!t) throw runtime_error("value not found: " + v);
    return t;
}

// Вставка в голову — O(1). Если список был пуст, tail должен указать на новый узел.
void slistPushHead(SList& list, const string& v) {
    list.head = new SNode{v, list.head};
    if (!list.tail) list.tail = list.head;
    ++list.n;
}

// Вставка в хвост — O(1) благодаря сохранённому tail.
void slistPushTail(SList& list, const string& v) {
    SNode* x = new SNode{v, nullptr};
    if (list.tail) list.tail->next = x; else list.head = x;  // пустой список — x и есть head
    list.tail = x;
    ++list.n;
}

// Вставка сразу после узла target. Если target был хвостом — обновляем tail.
void slistPushAfter(SList& list, const string& target, const string& v) {
    SNode* t = slistMustFind(list, target);
    t->next = new SNode{v, t->next};
    if (t == list.tail) list.tail = t->next;
    ++list.n;
}

// Вставка перед target. В односвязном списке нет prev — поэтому:
// target в голове — особый случай (обычная pushHead);
// иначе проходим до узла p, у которого p->next->val == target.
void slistPushBefore(SList& list, const string& target, const string& v) {
    slistMustFind(list, target);
    if (list.head->val == target) return slistPushHead(list, v);
    SNode* p = list.head;
    while (p->next->val != target) p = p->next;  // safe: target точно есть
    p->next = new SNode{v, p->next};
    ++list.n;
}

// Удаление головы — O(1). Если удалили последний узел, tail = nullptr.
string slistPopHead(SList& list) {
    if (!list.head) throw runtime_error("list is empty");
    SNode* t = list.head;
    string r = t->val;
    list.head = t->next;
    if (!list.head) list.tail = nullptr;
    delete t;
    --list.n;
    return r;
}

// Удаление хвоста. Без prev приходится идти до предпоследнего узла — O(n).
// Случай единственного элемента вырождается в popHead.
string slistPopTail(SList& list) {  
    if (!list.head) throw runtime_error("list is empty");
    if (list.head == list.tail) return slistPopHead(list);
    SNode* p = list.head;
    while (p->next != list.tail) p = p->next;   // до предпоследнего
    string r = list.tail->val;
    delete list.tail;
    list.tail = p;
    p->next = nullptr;
    --list.n;
    return r;
}

// Удаление элемента после target. Если удалили хвост — новый tail = t.
string slistDelAfter(SList& list, const string& target) {
    SNode* t = slistMustFind(list, target);
    if (!t->next) throw runtime_error("no element after " + target);
    SNode* x = t->next;
    string r = x->val;
    t->next = x->next;
    if (x == list.tail) list.tail = t;
    delete x;
    --list.n;
    return r;
}

// Удаление элемента перед target. Снова нет prev — ищем узел p,
// у которого p->next->next == t. Случай «цель — второй узел» = popHead.
string slistDelBefore(SList& list, const string& target) {
    SNode* t = slistMustFind(list, target);
    if (t == list.head) throw runtime_error("no element before " + target);
    if (list.head->next == t) return slistPopHead(list);
    SNode* p = list.head;
    while (p->next->next != t) p = p->next;   // p — узел перед удаляемым
    SNode* x = p->next;
    string r = x->val;
    p->next = t;
    delete x;
    --list.n;
    return r;
}

// Удаление первого вхождения значения. Голова — отдельный случай;
// иначе ищем p с p->next->val == v и вырезаем p->next.
// Проверка slistFindNode заранее гарантирует, что проход найдёт узел.
bool slistDelValue(SList& list, const string& v) {
    if (!slistFindNode(list, v)) return false;
    if (list.head->val == v) { slistPopHead(list); return true; }
    SNode* p = list.head;
    while (p->next->val != v) p = p->next;
    SNode* x = p->next;
    p->next = x->next;
    if (x == list.tail) list.tail = p;
    delete x;
    --list.n;
    return true;
}

// Индекс первого вхождения v (0-based) или -1. O(n).
int slistFind(const SList& list, const string& v) {
    int i = 0;
    for (SNode* c = list.head; c; c = c->next, ++i) if (c->val == v) return i;
    return -1;
}

// Доступ по индексу (0-based) с проверкой границ. O(n) — списки не индексируются.
// Возвращает ссылку на значение внутри узла — валидна, пока узел жив.
const string& slistAt(const SList& list, size_t i) {
    if (i >= list.n) throw runtime_error("index out of range: " + to_string(i));
    SNode* c = list.head;
    while (i--) c = c->next;
    return c->val;
}

// Полная очистка: удаляем с головы, пока не пусто.
void slistClear(SList& list) { while (list.head) slistPopHead(list); }