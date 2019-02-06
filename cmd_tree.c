#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "cmd_tree.h"

#define CMD_ELEMHELP_SIZE   64
#define CMD_ELEMNAME_SIZE   64

#define CMD_ELEMENT_MAX     9

typedef enum{
    ELEM_DUMMYROOT,
    ELEM_KW,
    ELEM_INTEGER,
    ELEM_STRING,
    ELEM_MAX
}MSH_CMDELEM_TYPE;


struct cmd_element 
{
    char name[CMD_ELEMNAME_SIZE];
    char help[CMD_ELEMHELP_SIZE];
    unsigned int id;       
};

struct cmd_ctx
{
    struct cmd_element elements[CMD_ELEMENT_MAX];
    int index;
    int err;
};

struct cmd_node
{
    struct msh_list_item self; 
    char name[CMD_ELEMNAME_SIZE];
    char help[CMD_ELEMHELP_SIZE];
    unsigned int id;
    unsigned int refcount;
    MSH_CMDELEM_TYPE type;
    struct msh_list child;
};


static struct cmd_node* root = NULL;


typedef enum{
    PATH_STATUS_NORMAL,
    PATH_STATUS_SQUARE,
    PATH_STATUS_BIG,
}PATH_STATUS;

/*
 *  check cmd path
 *  eg.  1 2 3 {4 | [ 5 } 6
 */
static int cmd_path_check(char* path, struct cmd_ctx* ctx)
{
    int err = 0;
    int pathlen  = strlen(path);
    int index_max = ctx->index - 1;
    PATH_STATUS status = PATH_STATUS_NORMAL;
    
    char* ptr = path;
    char c;
    
    while((c = *ptr++) != '\0')
    {
        if (c == ' ')
        {
            continue;
        }
        else if (c == '[')
        {
            if (status != PATH_STATUS_NORMAL)
            {
                goto check_err;
            }
            status = PATH_STATUS_SQUARE;
        }
        else if (c == ']')
        {
            if (status != PATH_STATUS_SQUARE)
            {
                goto check_err;
            }
            status = PATH_STATUS_NORMAL;
        }
        else if (c == '{')
        {
            if (status != PATH_STATUS_NORMAL)
            {
                goto check_err;
            }
            status = PATH_STATUS_BIG;           
        }
        else if (c == '}')
        {
            if (status != PATH_STATUS_BIG)
            {
                goto check_err;
            }
            status = PATH_STATUS_NORMAL;
        }        
        else if (c == '|')
        {
            if (status == PATH_STATUS_NORMAL)
            {
                goto check_err;
            }
        }
        else if((c >= '0') && (c <= '9'))
        {
            int idx = c - '0';
            if ((idx > index_max) || ((*ptr >= '0') && (*ptr <= '9')))
            {
                goto check_err;
            }
        }
        else 
        {
            goto check_err;
        }
    }

    return 0;
   
check_err:
    return -1;
    
}

/*
 * Create a new node 
 */
struct cmd_node* cmd_node_create(MSH_CMDELEM_TYPE type, char* name, char* help)
{
    struct cmd_node* n = NULL;

    n = (struct cmd_node* )malloc(sizeof(struct cmd_node));
    if (NULL == n)
    {
        return NULL;
    }

    bzero(n, sizeof(struct cmd_node));

    msh_list_item_init(&n->self);

    n->refcount = 1;
    
    if (NULL != name)
    {
        memcpy(n->name, name, strlen(name));
    }

    if (NULL != help)
    {
        memcpy(n->help, help, strlen(help));
    }
    
    n->type = type;

    msh_list_init(&n->child);

    return n;
}

/*
 * define a new element
 */
int cmd_def_element(char* name, char* help, unsigned int id, void* _ctx)
{
    struct cmd_ctx* ctx = _ctx;
    if (ctx->err != 0)
    {
        goto err_out;    
    }
    
    if (ctx->index == CMD_ELEMENT_MAX)
    {   
        goto err_out;
    }

    int namelen = strlen(name);
    int helplen = strlen(help);

    if ((namelen >= CMD_ELEMNAME_SIZE) || (helplen >= CMD_ELEMHELP_SIZE))
    {
        goto err_out;
    }

    memcpy(ctx->elements[ctx->index].name, name, namelen);
    ctx->elements[ctx->index].name[namelen] = '\0';

    memcpy(ctx->elements[ctx->index].help, help, helplen);
    ctx->elements[ctx->index].help[helplen] = '\0';

    ctx->index++;
    
    return 0;
err_out:
    ctx->err = -1;

    return -1;
}


/*
 * create cmd context
 */
void* cmd_ctx_create(void)
{
    struct cmd_ctx* c = NULL;

    c = (struct cmd_ctx* )malloc(sizeof(struct cmd_ctx));
    if (NULL != c)
    {
         bzero(c, sizeof(struct cmd_ctx));

         c->index = 0;
         c->err   = 0;
    }

    return (void*)c;
}

/*
 * destroy cmd command
 */
void cmd_ctx_destroy(void* ctx)
{
    if (NULL != ctx)
    {
        free(ctx);
    }

    return;
}

/*
 *  register one command
 */
int cmd_register(char* path, void* _ctx)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    
    if (ctx->err != 0)
    {
        return -1;
    }

    return cmd_path_check(path, ctx);
}

int test_cmd(void)
{
    int err = 0;
    
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    /* 0 */
    cmd_def_element("abc", "abc-help", 0, ctx);

    /* 1 */
    cmd_def_element("def", "def-help", 0, ctx);

    /* 2 */
    cmd_def_element("eee", "def-help", 0, ctx);

    /* 3 */
    cmd_def_element("fff", "def-help", 0, ctx);

    /* 4 */
    cmd_def_element("qqq", "def-help", 0, ctx);
    
    /* abc def */
    err = cmd_register("0 1 { [ 2 | 3 ] } 4", ctx);
    if (err == 0)
    {
        printf("register cmd1 succeed\n");
    }
    else
    {
        printf("register cmd1 failed\n");
    }
    
    cmd_ctx_destroy(ctx);

    return err;
}



int cmd_tree_init(void)
{  
    /* create root element */
    root = cmd_node_create(ELEM_DUMMYROOT, NULL, NULL);
    if (NULL == root)
    {
        return -1;
    }

    return 0;
}


