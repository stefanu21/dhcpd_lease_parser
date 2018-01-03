/*
 * dllist.h
 *
 *  Created on: 19. Nov. 2015
 *      Author: sursella
 */

#ifndef _DLLIST_H_
#define _DLLIST_H_

#include <stddef.h>

#define wv_container_of(ptr, sample, member)                            \
        (__typeof__(sample))((char *)(ptr) -                            \
                             offsetof(__typeof__(*sample), member))

#define dllist_for_each(pos, head, member)                              \
        for (pos = wv_container_of((head)->next, pos, member);  \
             &pos->member != (head);                                    \
             pos = wv_container_of(pos->member.next, pos, member))

#define dllist_for_each_safe(pos, tmp, head, member)                    \
        for (pos = wv_container_of((head)->next, pos, member),          \
             tmp = wv_container_of((pos)->member.next, tmp, member);    \
             &pos->member != (head);                                    \
             pos = tmp,                                                 \
             tmp = wv_container_of(pos->member.next, tmp, member))

#define dllist_for_each_reverse(pos, head, member)                      \
        for (pos = wv_container_of((head)->prev, pos, member);  \
             &pos->member != (head);                                    \
             pos = wv_container_of(pos->member.prev, pos, member))

#define dllist_for_each_reverse_safe(pos, tmp, head, member)            \
        for (pos = wv_container_of((head)->prev, pos, member),  \
             tmp = wv_container_of((pos)->member.prev, tmp, member);    \
             &pos->member != (head);                                    \
             pos = tmp,                                                 \
             tmp = wv_container_of(pos->member.prev, tmp, member))


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
 *
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
 *
 * The list now looks like [item2, item3, item1]
 *
 * Iterate the list in ascending order:
 *
 * item_t *item;
 * dllist_for_each(item, foo_list, link) {
 *      Do_something_with_item(item);
 * }
 */

struct dllist
{
  struct dllist *prev;
  struct dllist *next;
};

void dllist_init(struct dllist *list);
void dllist_insert(struct dllist *list, struct dllist *elm);
void dllist_remove(struct dllist *elm);
int dllist_length(const struct dllist *list);
int dllist_empty(const struct dllist *list);
void dllist_insert_list(struct dllist *list, struct dllist *other);

// return struct where ptr points to member
/*
 * struct foo{
 *      int a;
 *      int b;
 *      struct dllist link;
 * }
 *
 * struct foo data;
 * data.a = 5;
 * dllist list_foo;
 * dllist_init(&list_foo);
 * dllist_insert(&list_foo, &data.link)
 *
 * struct foo* foo_ = container_of(list_foo.next, struct foo, link)
 * printf("%d", foo_->a);   -->> return 5;
 */

#define container_of(ptr, type, member) ({                              \
        const __typeof__( ((type *)0)->member ) *__mptr = (ptr);        \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#endif /* _DLLIST_H_ */
