#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"
#include "cont.h"
#include "cmd.h"

#define CMD_NAME_SIZE 16
#define CMD_OPTION_MAX 16
#define CMD_KEYWORD_MAX 8

#define CMD_NODE_F_ROOT (1<<0)   /* root node */
#define CMD_NODE_F_LAST (1<<1)   /* this node is the last node in one command */


typedef enum
{
    CMD_OPTION_STRING,
    CMD_OPTION_INTERER,
    CMD_OPTION_KEYWORD,
}CMD_OPTION_TYPE;

typedef enum
{
    CMD_PARSE_FULLWORD,
    CMD_PARSE_END,
}CMD_PARSE_RESULT;

struct cmd_keyword
{
    char name[CMD_NAME_SIZE];
};

struct cmd_keywords
{
    struct cmd_keyword keyword[CMD_KEYWORD_MAX];
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
    //struct msh_list keyword;
    struct cmd_keywords *keywords;
    struct cmd_options *opts;
    int err;
};

struct cmd_node
{
    char name[CMD_NAME_SIZE]; 
    struct cmd_node* parent;
    struct msh_list_item self;
    struct msh_list child;
    int flag;
    struct cmd_options *opts; /* only valid when this node is last node */
    cmd_handler handler;      /* only valid when this node is last node */
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

static struct cmd_node* cmd_search_child(struct cmd_node* r, char* name)
{
    struct msh_list_item *it;
    struct cmd_node* n = NULL;

    if (0 == strlen(name))
    {
        return NULL;
    }
    
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

    return NULL;
}

static struct cmd_node* cmd_search_child_fuzzy(struct cmd_node* r, char* name, int* nb_match)
{
    struct msh_list_item *it;
    struct cmd_node* n = NULL;
    struct cmd_node* s = NULL;
    int namelen = strlen(name);
    *nb_match = 0;
    
    for (it = msh_list_begin(&r->child);
         it != msh_list_end (&r->child);
         it = msh_list_next (&r->child, it)) 
    {
        n = msh_cont (it, struct cmd_node, self);

        if (strlen(n->name) < namelen)
        {
            continue;
        }
        if ((0 == namelen) || (0 == strncmp(n->name, name, namelen))) 
        {
            *nb_match = *nb_match + 1;
            s = n;
        }
    }

    if (1 == *nb_match)
    {
        return s;
    }
    else
    {
        return NULL;
    }
}

static int cmd_usage(struct cmd_node* r, char buf[])
{
    int offset = 0;

    if ((NULL != r->parent) && !(r->parent->flag & CMD_NODE_F_ROOT))
    {
        offset = cmd_usage(r->parent, buf);
    }

    return sprintf(buf + offset, "%s ", (char*)r->name);
}

static void cmd_print_usage(struct cmd_node* r)
{
    char buf[256];

    if ((NULL == r) || !(r->flag & CMD_NODE_F_LAST))
    {
        return;
    }

    bzero(buf, sizeof(buf));

    if (cmd_usage(r,buf) > 0)
    {
        printf("\n%s", (char*)buf);
    }

    return;
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
        if (NULL != ctx->keywords)
        {
            free(ctx->keywords);
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
 * define command keyword
 * eg.
 */
void cmd_def_keyword(char* name, void* _ctx)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    
    if (ctx->err != 0)
    {
        return;
    }    

    /* FIXME: help infomation */   

    if (ctx->keywords == NULL)
    {
        ctx->keywords = (struct cmd_keywords*)malloc(sizeof(struct cmd_keywords));
    
        bzero(ctx->keywords, sizeof(struct cmd_keywords));    
    }
    
    struct cmd_keyword *keyword = &ctx->keywords->keyword[ctx->keywords->nr++];

    memcpy(keyword->name, name, strlen(name));

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
void cmd_register(void* _ctx, cmd_handler handler)
{
    int i,j;
    struct cmd_node* r = root;
    struct cmd_node* s;
    struct cmd_node* n;
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    if ((0 != ctx->err) || (NULL == handler))
    {
        return;
    }

    /* find */
    for (i = 0; i < ctx->keywords->nr; i++)
    {
        s = cmd_search_child(r, ctx->keywords->keyword[i].name);
        if (s == NULL)
        {
            break;
        }

        r = s;
    }
    
    for (j = i; j < ctx->keywords->nr; j++)
    {
        /* create new cmd_node*/
        n = cmd_node_create(ctx->keywords->keyword[j].name);
        
        msh_list_insert(&r->child, &n->self, NULL);

        n->parent = r;
        
        r = n;
    }

    /* now, @r is the last keyword */
    if (NULL != r->opts)
    {
        ctx->err = -1;
    }
    else
    {
        r->opts = ctx->opts;       
        r->flag |= CMD_NODE_F_LAST;
        r->handler = handler;
    }

    return;
}

static CMD_PARSE_RESULT cmd_parse_input(char* input, int* offset, char* buf)
{
    char* ptr;
    char* start;
    int off = *offset;
    CMD_PARSE_RESULT ret = CMD_PARSE_END;
    
    if (off >= strlen(input))
        return ret;

    ptr  = input + off;
     
    /* skip space */
    while(*ptr == ' ')
    {
        ptr++;
    }

    start = ptr;

    while(*ptr != '\0' && *ptr != ' ')
    {
        ptr++;
    }

    memcpy(buf, start, ptr - start);

    if ((*ptr == ' ') && strlen(buf) > 0)
    {
        ret = CMD_PARSE_FULLWORD;
    }

    *offset = ptr - input;

    return ret;
}

static void cmd_complete(struct cmd_node* r, char* input, char* matchpart)
{
    char* dst = input + strlen(input);
    char* src = r->name + strlen(matchpart);
    int copylen = strlen(r->name) - strlen(matchpart);

    while(copylen > 0)
    {
        *dst++ = *src++;
        copylen--;
    }

    *dst = ' ';

    return;
}

static void cmd_help_option(struct cmd_node* r)
{
    int i;
    
    if(r->opts)
    {
        cmd_print_usage(r);
        for(i = 0; i < r->opts->nr; i++)
        {
            printf("\n%s", (char*)r->opts->opt[i].name);
        }
    }

    return;
}

static void cmd_help_keyword(struct cmd_node* r, char* condition)
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

void cmd_tab(char* input)
{
    char buf[CMD_NAME_SIZE];
    int offset = 0;
    struct cmd_node* r = root;
    struct cmd_node* s;
    CMD_PARSE_RESULT result;
    int nb_match = 0;
    
    for(;;)
    {
        bzero(buf, CMD_NAME_SIZE);        
        result = cmd_parse_input(input, &offset, buf);
        if (result == CMD_PARSE_FULLWORD)
        {
            s = cmd_search_child(r,buf);
        }
        else
        {
            if (!(r->flag & CMD_NODE_F_LAST))
            {
                s = cmd_search_child_fuzzy(r, buf, &nb_match);
                if (0 == nb_match)
                {
                    return;
                }
                else if (1 == nb_match)
                {
                    cmd_complete(s, input, buf);
                    return;
                }
            }
            s = NULL;
        }

        if (NULL == s)
        {
            if (r->flag & CMD_NODE_F_LAST)
            {
                /* FIXME, help options */
                cmd_help_option(r);
            }
            else
            {
                cmd_help_keyword(r, buf);
            }
            break;
        }
        r = s;
    }
    
    return;
}

static LINE_PROCESS_RESULT cmd_exec_opt(struct cmd_node* s, char* input, int offset)
{
    LINE_PROCESS_RESULT ret = PROCESS_CMD_EXEC;
    void* ctx = NULL;
    
    if (NULL == s->opts)    
    {
        s->handler(ctx);
    }

    return ret;
}

/*
 * try exec one command
 */
LINE_PROCESS_RESULT cmd_exec(char* input)
{
    char buf[CMD_NAME_SIZE];
    int offset = 0;
    struct cmd_node* r = root;
    struct cmd_node* s;
    LINE_PROCESS_RESULT ret = PROCESS_CMD_EXEC;
    
    for(;;)
    {
        bzero(buf, CMD_NAME_SIZE);        

        cmd_parse_input(input, &offset, buf);

        s = cmd_search_child(r,buf);
        if (NULL != s)
        {
            if (s->flag & CMD_NODE_F_LAST)
            {
                /* command is matched */
                ret = cmd_exec_opt(s, input, offset);

                break;
            }
            else
            {
                r = s;
            }
            
        }
        else
        {
            ret = PROCESS_CMD_NOTFOUND;
            break;
        }
    }
    
    return ret;    
}

int cmd_tree_init(void)
{  
    /* create root element */
    root = cmd_node_create("root");
    if (NULL == root)
    {
        return -1;
    }

    root->flag |= CMD_NODE_F_ROOT;
    
    return 0;
}


