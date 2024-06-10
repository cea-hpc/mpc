#ifndef QUEUE_H
#define QUEUE_H

#include <stdlib.h>

typedef struct mpc_queue_elem {
        struct mpc_queue_elem *next;
} mpc_queue_elem_t;

typedef struct mpc_queue_head {
        struct mpc_queue_elem *head;
        struct mpc_queue_elem **tail;
} mpc_queue_head_t;

typedef struct mpc_queue_elem **mpc_queue_iter_t;

static inline void mpc_queue_init_head(mpc_queue_head_t *queue) {
        queue->head = (mpc_queue_elem_t *)queue;
        queue->tail = &queue->head;
}

static inline size_t mpc_queue_length(mpc_queue_head_t *queue) {
        size_t length = 0;
        mpc_queue_elem_t **pelem;

        for (pelem = &queue->head; pelem != queue->tail; pelem = &(*pelem)->next) {
                length++;
        }
        return length;
}

static inline int mpc_queue_is_tail(mpc_queue_head_t *queue,
                                    mpc_queue_elem_t *elem) {
        return &elem->next == queue->tail;
}

static inline int mpc_queue_is_empty(mpc_queue_head_t *queue) {
        return queue->tail == &queue->head;
}

static inline void mpc_queue_push(mpc_queue_head_t *queue,
                                  mpc_queue_elem_t *elem) {

        *queue->tail = elem;
        queue->tail  = &elem->next;
}

static inline void mpc_queue_push_head(mpc_queue_head_t *queue,
                                       mpc_queue_elem_t *elem) {
        elem->next = queue->head;
        queue->head = elem;
        if (mpc_queue_is_empty(queue)) {
                queue->tail = &elem->next;
        }
}

static inline mpc_queue_elem_t *mpc_queue_pull(mpc_queue_head_t *queue) {
        mpc_queue_elem_t *elem;

        if (mpc_queue_is_empty(queue))
                return NULL;

        elem = queue->head;
        queue->head = queue->head->next;
        if (mpc_queue_is_tail(queue, elem)) {
                queue->tail = &queue->head;
        }

        return elem;
}

static inline void mpc_queue_del_iter(mpc_queue_head_t *queue,
                                      mpc_queue_iter_t iter)
{
        if (mpc_queue_is_tail(queue, *iter)) {
                queue->tail = iter;
                *iter       = NULL;
        } else {
                *iter       = (*iter)->next;
        }
}

#define mpc_queue_for_each_safe(elem, iter, type, queue, member) \
        for (iter = &(queue)->head, \
             elem = mpc_container_of(*iter, type, member); \
             iter != (queue)->tail; \
             iter = (*iter == &elem->member) ? &(*iter)->next : iter, \
             elem = mpc_container_of(*iter, type, member))


#define mpc_queue_pull_elem(queue, type, member) \
        mpc_container_of(mpc_queue_pull(queue), type, member)

#endif
