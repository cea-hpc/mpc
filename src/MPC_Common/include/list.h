#ifndef LIST_H
#define LIST_H

typedef struct mpc_list_elem {
        struct mpc_list_elem *next;
        struct mpc_list_elem *prev;
} mpc_list_elem_t;

static inline void mpc_list_init_head(mpc_list_elem_t *head) {
        head->prev = head->next = head;
}

static inline int mpc_list_is_empty(mpc_list_elem_t *head) {
        return head->next == head;
}

static inline void mpc_list_del(mpc_list_elem_t *elem) {
        elem->prev->next = elem->next;
        elem->next->prev = elem->prev;
}

static inline void mpc_list_insert(mpc_list_elem_t *prev,
                                   mpc_list_elem_t *next,
                                   mpc_list_elem_t *new_elem) {
       new_elem->next = next;
       new_elem->prev = prev;
       prev->next     = new_elem;
       next->prev     = new_elem;
}

static inline void mpc_list_insert_after(mpc_list_elem_t *elem,
                                         mpc_list_elem_t *new_elem) {
        mpc_list_insert(elem, elem->next, new_elem);
}

static inline void mpc_list_insert_before(mpc_list_elem_t *elem,
                                          mpc_list_elem_t *new_elem) {
        mpc_list_insert(elem->next, elem, new_elem);
}

#define mpc_list_push_head(_head, _elem) \
        mpc_list_insert_after(_head, _elem)
#define mpc_list_push_tail(_head, _elem) \
        mpc_list_insert_before(_head, _elem)

#define mpc_list_prev(_elem, _type, _member) \
        (mpc_container_of((_elem)->prev, _type, _member))

#define mpc_list_next(_elem, _type, _member) \
        (mpc_container_of((_elem)->next, _type, _member))

#define mpc_list_head mpc_list_next
#define mpc_list_tail mpc_list_prev

#define mpc_list_for_each(_elem, _head, _type, _member) \
        for (_elem = mpc_container_of((_head)->next, _type, _member); \
             &(_elem)->_member != (_head); \
             _elem = mpc_container_of((_elem)->_member.next, _type, _member))

#endif
