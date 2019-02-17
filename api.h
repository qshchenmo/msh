#ifndef MSH_CMDAPI_INCLUDED
#define MSH_CMDAPI_INCLUDED


typedef int (*cmd_handler)(void* ctx);

void* cmd_getpara(void* ctx, int* id);
#define CMD_OPT_SCAN(ctx ,id, para) \
    for(para = cmd_getpara(ctx, &(id)); \
        NULL != para; \
        para = cmd_getpara(ctx, &(id)))

/* register one cmd */
void cmd_register(void* _ctx, cmd_handler handler);

/* create new context   */
void* cmd_ctx_create(void);

/* destroy context @ctx */
void cmd_ctx_destroy(void* ctx);

void cmd_def_keyword(char* name, void* _ctx, char* helpstr);
void cmd_def_option(char* name, int id, int required, int nopara, void* _ctx);

#endif

