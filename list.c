#include <stddef.h>

#include "list.h"
//#include "err.h"
//#include "attr.h"

void msh_list_init(struct msh_list *self)
{
    self->first = NULL;
    self->last = NULL;
}

int msh_list_empty(struct msh_list *self)
{
    return self->first ? 0 : 1;
}

void msh_list_item_init (struct msh_list_item *self)
{
    self->prev = MSH_LIST_NOTINLIST;
    self->next = MSH_LIST_NOTINLIST;
}


struct msh_list_item *msh_list_begin (struct msh_list *self)
{
    return self->first;
}

struct msh_list_item *msh_list_end (struct msh_list *self)
{
    return NULL;
}


struct msh_list_item *msh_list_prev (struct msh_list *self,
    struct msh_list_item *it)

{
    if (!it)
        return self->last;
    return it->prev;
}

struct msh_list_item *msh_list_next (struct msh_list *self,
    struct msh_list_item *it)
{
    return it->next;
}



/*
 * insert @item before @it, if @it is NULL, append to @self's tail
 */
void msh_list_insert (struct msh_list *self, struct msh_list_item *item, struct msh_list_item *it)
{
    item->prev = it ? it->prev : self->last;
    item->next = it;
    if (item->prev)
        item->prev->next = item;
    if (item->next)
        item->next->prev = item;
    if (!self->first || self->first == it)
        self->first = item;
    if (!it)
        self->last = item;
}

/*
struct msh_list_item *msh_list_erase (struct msh_list *self,
    struct msh_list_item *item)
{
    struct msh_list_item *next;

    nn_assert (msh_list_item_isinlist (item));

    if (item->prev)
        item->prev->next = item->next;
    else
        self->first = item->next;
    if (item->next)
        item->next->prev = item->prev;
    else
        self->last = item->prev;

    next = item->next;

    item->prev = NN_LIST_NOTINLIST;
    item->next = NN_LIST_NOTINLIST;

    return next;
}


void msh_list_item_term (struct msh_list_item *self)
{
    nn_assert (!msh_list_item_isinlist (self));
}

int msh_list_item_isinlist (struct msh_list_item *self)
{
    return self->prev == NN_LIST_NOTINLIST ? 0 : 1;
}
*/
