#ifndef MSH_LIST_INCLUDED
#define MSH_LIST_INCLUDED

struct msh_list_item {
    struct msh_list_item *next;
    struct msh_list_item *prev;
};

struct msh_list {
    struct msh_list_item *first;
    struct msh_list_item *last;
};

/*  Undefined value for initializing a list item which is not part of a list. */
#define MSH_LIST_NOTINLIST ((struct msh_list_item*)-1) 

void msh_list_init(struct msh_list *self);
void msh_list_item_init (struct msh_list_item *self);


#if 0
/*  Undefined value for initializing a list item which is not part of a list. */
#define NN_LIST_NOTINLIST ((struct nn_list_item*) -1)

/*  Use for initializing a list item statically. */
#define NN_LIST_ITEM_INITIALIZER {NN_LIST_NOTINLIST, NN_LIST_NOTINLIST}

/*  Initialise the list. */
void nn_list_init (struct nn_list *self);

/*  Terminates the list. Note that all items must be removed before the
    termination. */
void nn_list_term (struct nn_list *self);

/*  Returns 1 is list has zero items, 0 otherwise. */
int nn_list_empty (struct nn_list *self);

/*  Returns iterator to the first item in the list. */
struct nn_list_item *nn_list_begin (struct nn_list *self);

/*  Returns iterator to one past the last item in the list. */
struct nn_list_item *nn_list_end (struct nn_list *self);

/*  Returns iterator to an item prior to the one pointed to by 'it'. */
struct nn_list_item *nn_list_prev (struct nn_list *self,
    struct nn_list_item *it);

/*  Returns iterator to one past the item pointed to by 'it'. */
struct nn_list_item *nn_list_next (struct nn_list *self,
    struct nn_list_item *it);

/*  Adds the item to the list before the item pointed to by 'it'. Priot to
    insertion item should not be part of any list. */
void nn_list_insert (struct nn_list *self, struct nn_list_item *item,
    struct nn_list_item *it);

/*  Removes the item from the list and returns pointer to the next item in the
    list. Item must be part of the list. */
struct nn_list_item *nn_list_erase (struct nn_list *self,
    struct nn_list_item *item);

/*  Initialize a list item. At this point it is not part of any list. */
void nn_list_item_init (struct nn_list_item *self);

/*  Terminates a list item. Item must not be part of any list before it's
    terminated. */
void nn_list_item_term (struct nn_list_item *self);

/*  Returns 1 is the item is part of a list, 0 otherwise. */
int nn_list_item_isinlist (struct nn_list_item *self);

#endif

#endif
