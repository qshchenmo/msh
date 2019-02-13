#include <stdio.h>
#include <stdlib.h>
#include "cmd.h"
#include "shell.h"


static void test_cmd1_handler(void* ctx)
{
    printf("\ntest 1 is exec\n");
}

static void test_cmd2_handler(void* ctx)
{
    printf("\ntest 2 is exec\n");
}

static void test_cmd3_handler(void* ctx)
{
    printf("\ntest 3 is exec\n");
}


static void test_cmd1(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("abc", ctx);

    cmd_def_keyword("def", ctx);

    cmd_register(ctx, test_cmd1_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

static void test_cmd2(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("bbc", ctx);

    cmd_def_keyword("def", ctx);

    cmd_register(ctx, test_cmd2_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}

static void test_cmd3(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("abc", ctx);

    cmd_def_keyword("d", ctx);

    cmd_register(ctx, test_cmd3_handler);
    
    cmd_ctx_destroy(ctx);

    return;
}


static void quit_handler(void* ctx)
{
    printf("\ntest 3 is exec\n");
}

static void quit_cmd(void)
{
    void* ctx = NULL;

    ctx = cmd_ctx_create();

    cmd_def_keyword("quit", ctx);

    cmd_register(ctx, quit_handler());
    
    cmd_ctx_destroy(ctx);

    return;
}


void internal_cmd_register(void)
{   
    test_cmd1();

    test_cmd2();

    test_cmd3();

    quit_cmd();
//    internal_quit_register();
}

