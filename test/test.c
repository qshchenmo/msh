#include <stdio.h>
#include <msh.h>

#define OPT0 0
#define OPT1 1
#define OPT2 2
#define OPT3 3

static int test1_cmd_handler(void* ctx)
{
    void* para = NULL;
    int id = MSH_INVALID_ID;
    
    printf("this is test1 cmd handler\n");
    
    CMD_OPT_SCAN(ctx ,id, para)
    {
        switch(id)
        {
            case OPT0:
            {
                printf("opt0 %s\n", cmd_getpara_string(para));
                break;
            }
            case OPT1:
            {
                printf("opt1 %u\n", cmd_getpara_interger(para));
                break;
            }
            default:
            {
                return MSH_USER_ERROR_FAILED;
            }
        }
    }
    
    return MSH_USER_ERROR_SUCCESS;
}

static void test1_cmd_install(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("test1", ctx, "test1_helper");    

    cmd_def_option("opt0", OPT0, 0, "string", ctx, "this is opt0's helpstr");

    cmd_def_option("opt1", OPT1, 0, "interger", ctx, "this is opt1's helpstr");

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


