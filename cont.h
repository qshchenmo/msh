#ifndef MSH_CONT_INCLUDED
#define MSH_CONT_INCLUDED

#include <stddef.h>

/*  Takes a pointer to a member variable and computes pointer to the structure
    that contains it. 'type' is type of the structure, not the member. */
#define msh_cont(ptr, type, member) \
    (ptr ? ((type*) (((char*) ptr) - offsetof(type, member))) : NULL)

#endif

