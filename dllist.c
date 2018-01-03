#include "dllist.h"

/**
 *
 * \brief doubly-linked list
 *
 * The list head is of "struct dllist" type, and must be initialized
 * using dllist_init().  All entries in the list must be of the same
 * type.  The item type must have a "struct dllist" member. This
 * member will be initialized by dllist_insert(). There is no need to
 * call dllist_init() on the individual item. To query if the list is
 * empty in O(1), use dllist_empty().
 *
 * Let's call the list reference "struct dllist foo_list", the item type as
 * "item_t", and the item member as "struct dllist link".
 *
 * The following code will initialize a list:
 * struct dllist foo_list;
 *
 * struct item_t {
 *      int foo;
 *      struct dllist link;
 * };
 * struct item_t item1, item2, item3;
 *
 * dllist_init(&foo_list);
 * dllist_insert(&foo_list, &item1.link);       // Pushes item1 at the head
 * dllist_insert(&foo_list, &item2.link);       // Pushes item2 at the head
 * dllist_insert(&item2.link, &item3.link);     // Pushes item3 after item2
 *
 * The list now looks like [item2, item3, item1]
 *
 * Iterate the list in ascending order:
 * item_t *item;
 * dllist_for_each(item, foo_list, link) {
 *      Do_something_with_item(item);
 * }
 */


void dllist_init(struct dllist *list)
{
  list->prev = list;
  list->next = list;
}

void dllist_insert(struct dllist *list, struct dllist *elm)
{
  elm->prev = list;
  elm->next = list->next;
  list->next = elm;
  elm->next->prev = elm;
}

void dllist_remove(struct dllist *elm)
{
  elm->prev->next = elm->next;
  elm->next->prev = elm->prev;
}

int dllist_length(const struct dllist *list)
{
  struct dllist *e;
  int count;
  count = 0;
  e = list->next;
  while(e != list)
  {
    e = e->next;
    count++;
  }
  return count;
}

int dllist_empty(const struct dllist *list)
{
  return list->next == list;
}

void dllist_insert_list(struct dllist *list, struct dllist *other)
{
  other->next->prev = list;
  other->prev->next = list->next;
  list->next->prev = other->prev;
  list->next = other->next;
}


