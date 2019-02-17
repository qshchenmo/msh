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

void msh_list_insert (struct msh_list *self, struct msh_list_item *item, struct msh_list_item *it);

struct msh_list_item *msh_list_begin (struct msh_list *self);
struct msh_list_item *msh_list_end (struct msh_list *self);
struct msh_list_item *msh_list_prev (struct msh_list *self, struct msh_list_item *it);
struct msh_list_item *msh_list_next (struct msh_list *self, struct msh_list_item *it);



#endif
