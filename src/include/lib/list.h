#pragma once
#include <common/types.h>

/* clang-format off */
// double-linked list
//   TODO: use your own list design!!
typedef struct list_node {
    struct list_node *next, *prev;
} list_node_t;

typedef list_node_t list_head;

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	list_head name = LIST_HEAD_INIT(name)

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

#define container_of(ptr, type, member) ({          \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
    (type *)( (char *)__mptr - offsetof(type,member) );})


#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

static inline void init_list_head(list_head *list)
{
	list->next = list;
	list->prev = list;
}

static inline void list_init_with_null(list_head *list){
	list->next = NULL;
	list->prev = NULL;
}

// add
static inline void __list_add(list_head *new,list_head *prev,list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(list_head *new, list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(list_head *new, list_head *head)
{
	__list_add(new, head->prev, head);
}

// del
static inline void __list_del(list_head * prev, list_head * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void __list_del_entry(list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static inline void list_del(list_head *entry)
{
	__list_del_entry(entry);
	entry->next = NULL;
	entry->prev = NULL;
}

static inline void list_move(list_head *list, list_head *head)
{
	__list_del_entry(list);
	list_add(list, head);
}

static inline void list_move_tail(list_head *list, list_head *head)
{
	__list_del(list->prev, list->next);
	list_add_tail(list, head);
}


// judge
static inline int list_is_first(list_head *list, const list_head *head)
{
	return list->prev == head;
}

static inline int list_is_last(list_head *list, list_head *head)
{
	return list->next == head;
}

static inline int list_is_empty(list_head *head)
{
	return head->next == head;
}

/* clang-format on */