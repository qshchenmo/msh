#include <stdio.h>
#include "../inc/api.h"
#include "../inc/err.h"

#define OPT0 0

static int test1_cmd_handler(void* ctx)
{
    void* para = NULL;
    char buf[32];
    int id = MSH_INVALID_ID;
    
    printf("this is test1 cmd handler\n");
    
    CMD_OPT_SCAN(ctx ,id, para)
    {
        switch(id)
        {
            case OPT0:
            {
                cmd_getpara_string(para, buf, 32);
                printf("opt0 %s\n", (char*)buf);
                break;
            }
            default:
            {
                return MSH_ERROR_INVALID_OPT;
            }
        }
    }
    
    return MSH_ERROR_SUCCESS;
}

static void test1_cmd_install(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("test1", ctx, "test1_helper");    

    cmd_def_option("opt0", OPT0, 0, 0, ctx);
    
    cmd_register(ctx, test1_cmd_handler);

    cmd_ctx_destroy(ctx);

    return;
}

void __attribute__ ((constructor)) test_init(void)
{
    test1_cmd_install();

    return;
}


void __attribute__ ((destructor)) test_fini(void)
{
    return;
}


