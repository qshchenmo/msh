#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "cont.h"
#include "cmd_tree.h"

#define CMD_NAME_SIZE 16
#define CMD_OPTION_MAX 16
#define CMD_PREFIX_MAX 8

#define CMD_NODE_F_LAST (1<<0)

typedef enum
{
    CMD_OPTION_STRING,
    CMD_OPTION_INTERER,
    CMD_OPTION_KEYWORD,
}CMD_OPTION_TYPE;

struct cmd_prefix
{
    char name[CMD_NAME_SIZE];
};

struct cmd_prefixs
{
    struct cmd_prefix prefix[CMD_PREFIX_MAX];
    int nr;
};

struct cmd_option
{
    int name[CMD_NAME_SIZE];
    int required;
};

struct cmd_options
{
    struct cmd_option opt[CMD_OPTION_MAX];
    int nr;
};

/*
 *  command register contex
 */
struct cmd_ctx
{
    //struct msh_list prefix;
    struct cmd_prefixs *prefixs;
    struct cmd_options *opts;
    int err;
};

struct cmd_node
{
    char name[CMD_NAME_SIZE]; 
    struct msh_list_item self;
    struct msh_list child;
    int flag;
    struct cmd_options *opts; /* only valid when this node is a key node */
};

/* global cmd tree root */
static struct cmd_node* root = NULL;


/*
 * Create a new node 
 */
static struct cmd_node* cmd_node_create(char* name)
{
    struct cmd_node* n = NULL;

    n = (struct cmd_node* )malloc(sizeof(struct cmd_node));
    if (NULL == n)
    {
        return NULL;
    }

    bzero(n, sizeof(struct cmd_node));

    msh_list_item_init(&n->self);
    
    if (NULL != name)
    {
        memcpy(n->name, name, strlen(name));
    }
    
    msh_list_init(&n->child);

    return n;
}

static struct cmd_node* cmd_search_prefix(struct cmd_node* r, char* name)
{
    struct msh_list_item *it;
    struct cmd_node* n = NULL;
    
    for (it = msh_list_begin(&r->child);
         it != msh_list_end (&r->child);
         it = msh_list_next (&r->child, it)) 
    {
        n = msh_cont (it, struct cmd_node, self);
        if ((strlen(n->name) == strlen(name)) && (0 == strcmp(n->name, name))) 
        {
            return n;
        }
    }
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

         c->err   = 0;
    }

    return (void*)c;
}

/*
 * destroy cmd command
 */
void cmd_ctx_destroy(void* _ctx)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    if (NULL != ctx)
    {
        if (NULL != ctx->prefixs)
        {
            free(ctx->prefixs);
        }

        if ((NULL != ctx->opts) && (ctx->err != 0))
        {
            free(ctx->opts);
        }
        free(ctx);
    }

    return;
}

/*
 * define command prefix
 * eg.
 */
void cmd_def_prefix(char* name, void* _ctx)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    
    if (ctx->err != 0)
    {
        return;
    }    

    /* FIXME: help infomation */   

    if (ctx->prefixs == NULL)
    {
        ctx->prefixs = (struct cmd_prefixs*)malloc(sizeof(struct cmd_prefixs));
    
        bzero(ctx->prefixs, sizeof(struct cmd_prefixs));    
    }
    
    struct cmd_prefix *prefix = &ctx->prefixs->prefix[ctx->prefixs->nr++];

    memcpy(prefix->name, name, strlen(name));

    return;
}

/*
 * define option
 */
void cmd_def_option(char* name, int required, void* _ctx)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    if (ctx->err != 0)
    {
        return;
    }    

    if (ctx->opts == NULL)    
    {
        ctx->opts = (struct cmd_options*)malloc(sizeof(struct cmd_options));   
        bzero(ctx->opts, sizeof(struct cmd_options));
    }

    struct cmd_option *opt = &ctx->opts->opt[ctx->opts->nr++];
    
    memcpy(opt->name, name, strlen(name));

    opt->required = required;

    return;
}

/*
 * register an command
 */
void cmd_register(void* _ctx)
{
    int i,j;
    struct cmd_node* r = root;
    struct cmd_node* s;
    struct cmd_node* n;
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    if (ctx->err != 0)
    {
        return;
    }

    /* find */
    
    
    for (i = 0; i < ctx->prefixs->nr; i++)
    {
        s = cmd_search_prefix(r, ctx->prefixs->prefix[i].name);
        if (s == NULL)
        {
            break;
        }

        r = s;
    }
    
    for (j = i; j < ctx->prefixs->nr; j++)
    {
        /* create new cmd_node*/
        n = cmd_node_create(ctx->prefixs->prefix[j].name);
        
        msh_list_insert(&r->child, &n->self, NULL);

        r = n;
    }

    /* now, @r is the last prefix */
    if (NULL != r->opts)
    {
        ctx->err = -1;
    }
    else
    {
        r->opts = ctx->opts;       
        r->flag |= CMD_NODE_F_LAST;
    }

    return;
}

static int cmd_getword_frominput(char* input, int offset, char* buf)
{
    char* ptr;
    char* start;
    int step = 0;
    
    if (offset >= strlen(input))
        return -1;

    ptr  = input + offset;
 
    
    /* skip space */
    while(*ptr == ' ')
    {
        ptr++;
    }

    start = ptr;

    do {
        ptr++;
    }while(*ptr != '\0' && *ptr != ' ');

    memcpy(buf, start, ptr - start);

    return ptr-input;
}

static void cmd_help_prefix(struct cmd_node* r, char* condition)
{
    struct msh_list_item *it;
    struct cmd_node* n = NULL;
    
    for (it = msh_list_begin(&r->child);
         it != msh_list_end (&r->child);
         it = msh_list_next (&r->child, it)) 
    {
        n = msh_cont (it, struct cmd_node, self);
        if (0 == strncmp(n->name, condition, strlen(condition))) 
        {
            printf("\n%s", n->name);
        }
    }

    return;
}

int cmd_help(char* input)
{
    char buf[CMD_NAME_SIZE];
    int step = 0;
    int offset = 0;
    struct cmd_node* r = root;
    struct cmd_node* s;
    

    for(;;)
    {
        bzero(buf, CMD_NAME_SIZE);        
        offset = cmd_getword_frominput(input, offset, buf);
        if (-1 == offset)
        {
            break; 
        }

        s = cmd_search_prefix(r, buf);
        if (NULL == s)
        {
            if (r->flag & CMD_NODE_F_LAST)
            {
                /* FIXME, help options */
            }
            else
            {
                cmd_help_prefix(r, buf);
            }
            break;
        }
        r = s;
    }
    //struct cmd_node* cmd_search_prefix(struct cmd_node* r, char* name)

   // printf("hello\n");
   return 0;
}


int test_cmd(void)
{
    int err = 0;
    
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_prefix("abc", ctx);

    cmd_def_prefix("def", ctx);

    cmd_register(ctx);
    
    cmd_ctx_destroy(ctx);

    return err;
}





int cmd_tree_init(void)
{  
    /* create root element */
    root = cmd_node_create("root");
    if (NULL == root)
    {
        return -1;
    }

    return 0;
}


