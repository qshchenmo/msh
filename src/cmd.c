#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <dlfcn.h>
#include "list.h"
#include "cont.h"
#include "msh.h"
#include "cmd.h"
#include "pub.h"
#include "err.h"

#define CMD_NAME_SIZE  16
#define CMD_VALUE_SIZE 48
#define CMD_HELP_SIZE  32
#define CMD_OPTION_MAX 16
#define CMD_KEYWORD_MAX 8

#define CMD_NODE_F_ROOT (1<<0)   /* root node */
#define CMD_NODE_F_LAST (1<<1)   /* this node is the last node in one command */

#define FULLPATH_MAX_SIZE 256
#define EXTERNAL_COMMAND_DIR  "/etc/mshd/external/"
#define MSH_OPT_VALUEHINT_DEFAULT "value"

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
    char helpstr[CMD_HELP_SIZE];
};

struct cmd_keywords
{
    struct cmd_keyword keyword[CMD_KEYWORD_MAX];
    int nr;
};

struct cmd_option
{
    char name[CMD_NAME_SIZE];
    char helpstr[CMD_HELP_SIZE];
    char valuehint[CMD_NAME_SIZE];
    int id;
    int flag;
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
    char helpstr[CMD_HELP_SIZE];
    struct cmd_node* parent;
    struct msh_list_item self;
    struct msh_list child;
    int flag;
    struct cmd_options *opts; /* only valid when this node is last node */
    cmd_handler handler;      /* only valid when this node is last node */
};

struct cmd_opt_pair
{
    int opt_id; 
    char opt_value[CMD_VALUE_SIZE];
};


struct cmd_exec_ctx
{
    struct cmd_opt_pair opts[CMD_OPTION_MAX];
    int nr;
    int step;
};

/* global cmd tree root */
static struct cmd_node* root = NULL;


/*
 * Create a new node 
 */
static struct cmd_node* cmd_node_create(struct cmd_keyword* kw)
{
    struct cmd_node* n = NULL;

    n = (struct cmd_node* )malloc(sizeof(struct cmd_node));
    if (NULL == n)
    {
        return NULL;
    }

    bzero(n, sizeof(struct cmd_node));

    msh_list_item_init(&n->self);
    
    if (NULL != kw->name)
    {
        memcpy(n->name, kw->name, strlen(kw->name));
    }

    if (NULL != kw->helpstr)
    {
        memcpy(n->helpstr, kw->helpstr, strlen(kw->helpstr));     
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
    int i;
    int flag = 0;
    
    if ((NULL == r) || !(r->flag & CMD_NODE_F_LAST))
    {
        return;
    }

    bzero(buf, sizeof(buf));

    printf("\n[usage] ");
    if (cmd_usage(r,buf) > 0)
    {
        printf("%s", (char*)buf);
    }

    for (i = 0; i < r->opts->nr; i++)
    {
        flag = r->opts->opt[i].flag;
        if (flag & MSH_OPT_F_NOPARA)
        {
            printf(" [%s]", (char*)r->opts->opt[i].name);
        }
        else
        {
            printf(" [%s %s]", (char*)r->opts->opt[i].name, (char*)r->opts->opt[i].valuehint);
        }
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
void cmd_def_keyword(char* name, void* _ctx, char* helpstr)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    
    if (ctx->err != MSH_ERROR_SUCCESS)
    {
        return;
    }    

    if ((NULL == name) || (NULL == helpstr))
    {
        ctx->err = MSH_ERROR_FAILED;
        return;
    }

    if (ctx->keywords == NULL)
    {
        ctx->keywords = (struct cmd_keywords*)malloc(sizeof(struct cmd_keywords));
    
        bzero(ctx->keywords, sizeof(struct cmd_keywords));    
    }
    
    struct cmd_keyword *keyword = &ctx->keywords->keyword[ctx->keywords->nr++];

    memcpy(keyword->name, name, strlen(name));
    memcpy(keyword->helpstr, helpstr, strlen(helpstr));
    
    return;
}

/*
 * define option
 */
void cmd_def_option(char* name, int id, int flag, char* valuehint, void* _ctx, char* helpstr)
{
    struct cmd_ctx* ctx = (struct cmd_ctx*)_ctx;
    if (ctx->err != MSH_ERROR_SUCCESS)
    {
        return;
    }    

    if ((NULL == name) || (NULL == helpstr))
    {
        ctx->err = MSH_ERROR_FAILED;
        return;
    }
    
    if (ctx->opts == NULL)    
    {
        ctx->opts = (struct cmd_options*)malloc(sizeof(struct cmd_options));   
        bzero(ctx->opts, sizeof(struct cmd_options));
    }

    struct cmd_option *opt = &ctx->opts->opt[ctx->opts->nr++];
    
    memcpy(opt->name, name, strlen(name));
    memcpy(opt->helpstr, helpstr, strlen(helpstr));

    if (!(flag & MSH_OPT_F_NOPARA))
    {
        if (valuehint)
        {
            memcpy(opt->valuehint, valuehint, strlen(valuehint));    
        }
        else
        {
            memcpy(opt->valuehint, MSH_OPT_VALUEHINT_DEFAULT, strlen(MSH_OPT_VALUEHINT_DEFAULT));  
        }
    }
    
    opt->id = id;
    opt->flag = flag;

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
    if ((0 != ctx->err) || (NULL == handler) || (NULL == ctx->keywords))
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
        n = cmd_node_create(&ctx->keywords->keyword[j]);
        
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
            if (strlen(r->opts->opt[i].name) <= CMD_NAME_SIZE/2)
            {
                printf("\n %-8s %s", (char*)r->opts->opt[i].name, (char*)r->opts->opt[i].helpstr);
            }
            else
            {
                printf("\n %-16s %s", (char*)r->opts->opt[i].name, (char*)r->opts->opt[i].helpstr);
            }
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
            if (strlen(n->name) < 8)
            {
                printf("\n  %-8s %s", n->name, n->helpstr);    
            }
            else
            {
                printf("\n  %-16s %s", n->name, n->helpstr);  
            }
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

static struct cmd_option *cmd_search_opt(struct cmd_node* s, char* name)
{
    struct cmd_option* opt = NULL;
    int i;

    for(i = 0; i < s->opts->nr; i++)
    {
        if (0 == strcmp(s->opts->opt[i].name, name))
        {
            opt = &s->opts->opt[i];
            break;
        }
    }

    return opt;
}

char* cmd_getpara_string(void* para)
{
    return (char*)para;
}

int cmd_getpara_interger(void* para)
{
    return atoi((char*)para);
}

void* cmd_getpara(void* ctx, int* id)
{
    char* para = NULL;
    struct cmd_exec_ctx* exec_ctx;

    exec_ctx = (struct cmd_exec_ctx*)ctx;
    if (exec_ctx->step >= exec_ctx->nr)
    {
        return NULL;
    }

    *id = exec_ctx->opts[exec_ctx->step].opt_id;
    para = exec_ctx->opts[exec_ctx->step].opt_value;

    exec_ctx->step++;

    return para;
}

static int cmd_exec_opt(struct cmd_node* s, char* input, int offset)
{
    int err = MSH_ERROR_SUCCESS;
    struct cmd_exec_ctx exec_ctx;
    int inputlen = strlen(input);
    struct cmd_option* opt;
    char buf[CMD_NAME_SIZE];
    char vbuf[CMD_VALUE_SIZE];
    
    bzero(&exec_ctx, sizeof(exec_ctx));
    
    while(offset <= inputlen)
    {
        bzero(buf, CMD_NAME_SIZE);
        cmd_parse_input(input, &offset, buf);

        if ('\0' == buf[0])
        {
            break;
        }

        opt = cmd_search_opt(s, buf);
        if (NULL == opt)
        {
            err = MSH_ERROR_INVALID_OPT;
            goto err_out;
        }

        exec_ctx.opts[exec_ctx.nr].opt_id = opt->id;
        if (!(opt->flag & MSH_OPT_F_NOPARA))
        {
            bzero(vbuf, CMD_NAME_SIZE);
            cmd_parse_input(input, &offset, vbuf);
            if ('\0' == vbuf[0])
            {
                err = MSH_ERROR_INCOMPLETE_OPT;
                goto err_out;
            }           
            memcpy(exec_ctx.opts[exec_ctx.nr].opt_value, vbuf, strlen(vbuf));
        }

        exec_ctx.nr++;
    }

    err = s->handler((void*)&exec_ctx);

err_out:
    
    return err;
}

/*
 * try exec one command
 */
int cmd_exec(char* input)
{
    char buf[CMD_NAME_SIZE];
    int offset = 0;
    struct cmd_node* r = root;
    struct cmd_node* s;
    int err = MSH_ERROR_SUCCESS;
    
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
                err = cmd_exec_opt(s, input, offset);

                break;
            }
            else
            {
                r = s;
            }
            
        }
        else
        {
            err = MSH_ERROR_NOTFOUND;
            break;
        }
    }
    
    return err;    
}

static int cmd_quit_handler(void* ctx)
{
    printf("\n bye bye~\n");
    return MSH_ERROR_QUIT;
}

static void cmd_quit_install(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("quit", ctx, "quit mini shell");

    cmd_register(ctx, cmd_quit_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}


static int cmd_version_handler(void* ctx)
{
    printf("\n  mini shell version %d.%d", MSH_VERSION_MAJOR, MSH_VERSION_MINOR);
    return MSH_ERROR_SUCCESS;
}

static void cmd_version_install(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("version", ctx, "show mini shell");

    cmd_register(ctx, cmd_version_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

static void cmd_internal_register(void)
{   
    cmd_quit_install();

    cmd_version_install();

    return;
}

static void cmd_external_scan(void)
{
    DIR* dir;
    struct dirent* dentry;
    struct stat sb;
    void* handle;
    char fullpath[FULLPATH_MAX_SIZE];

    dir = opendir(EXTERNAL_COMMAND_DIR);
    if (NULL == dir)
    {
        return;    
    }

    for(dentry = readdir(dir); dentry != NULL; dentry = readdir(dir))
    {
        if (NULL == strstr(dentry->d_name, ".cli"))
        {
            continue;
        }

        bzero(fullpath, sizeof(fullpath));
        sprintf(fullpath, "%s%s", EXTERNAL_COMMAND_DIR, dentry->d_name);

        if (stat(fullpath, &sb) == -1)
        {
            perror("stat");
            continue;
        }

        if (S_ISDIR(sb.st_mode))
        {
            continue;
        }

        handle = dlopen(fullpath, RTLD_LAZY);
        if (!handle)
        {
            fprintf(stderr, "%s\n", dlerror());
        }
    }
    
    return;    
}


/*
 * init cmd tree
 */
int cmd_init(void)
{  
    struct cmd_keyword root_kw = {
        .name = "root",
        .helpstr = "help root",
    };
    
    /* create root element */
    root = cmd_node_create(&root_kw);
    if (NULL == root)
    {
        return -1;
    }

    root->flag |= CMD_NODE_F_ROOT;

    cmd_internal_register();

    cmd_external_scan();
    
    return 0;
}


