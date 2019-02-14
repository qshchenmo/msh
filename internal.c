#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "shell.h"
#include "err.h"

#define MSH_VERSION_MAJOR 0
#define MSH_VERSION_MINOR 1

static int test_cmd1_handler(void* ctx)
{
    printf("\ntest 1 is exec\n");

    return MSH_ERROR_SUCCESS;
}

static void test_cmd1(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("test1", ctx, "this is test1 help string");

    cmd_def_keyword("test2", ctx, "this is test2 help string");

    cmd_register(ctx, test_cmd1_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

static int test_cmd2_handler(void* ctx)
{
    printf("\ntest 2 is exec\n");

    return  MSH_ERROR_SUCCESS;
}

static void test_cmd2(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("test1", ctx, "this is test1 help string");

    cmd_def_keyword("test3", ctx, "this is test3 help string");

    cmd_register(ctx, test_cmd2_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

static int quit_handler(void* ctx)
{
    printf("\n bye bye~\n");
    return MSH_ERROR_QUIT;
}

static void quit_cmd(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("quit", ctx, "quit mini shell");

    cmd_register(ctx, quit_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

static int version_handler(void* ctx)
{
    printf("\n  mini shell version %d.%d", MSH_VERSION_MAJOR, MSH_VERSION_MINOR);
    return MSH_ERROR_SUCCESS;
}

static void version_cmd(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("version", ctx, "show mini shell");

    cmd_register(ctx, version_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

void internal_cmd_register(void)
{   
    test_cmd1();

    test_cmd2();

    quit_cmd();

    version_cmd();
    
//    internal_quit_register();
}

