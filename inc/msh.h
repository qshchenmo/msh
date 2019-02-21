#ifndef MSH_CMDAPI_INCLUDED
#define MSH_CMDAPI_INCLUDED

#define MSH_USER_ERROR_SUCCESS          0
#define MSH_USER_ERROR_FAILED         (-1)


#define MSH_INVALID_ID  (-1)

#define MSH_OPT_F_NOPARA   (1<<0)

typedef int (*cmd_handler)(void* ctx);

/* 
 * get string from @para
 */
char* cmd_getpara_string(void* para);

/* 
 * get interger from @para
 */
int cmd_getpara_interger(void* para);

void* cmd_getpara(void* ctx, int* id);
#define CMD_OPT_SCAN(ctx ,id, para) \
    for(para = cmd_getpara(ctx, &(id)); \
        NULL != para; \
        para = cmd_getpara(ctx, &(id)))

/* register one cmd */
void cmd_register(void* ctx, cmd_handler handler);

/* 
 * create new context   
 */
void* cmd_ctx_create(void);

/* 
 * destroy context @ctx
 */
void cmd_ctx_destroy(void* ctx);

/* 
 * cmd_def_keyword
 * @name: keyword name
 * @ctx:  create by cmd_ctx_create()
 * @helpstr: help string
 */
void cmd_def_keyword(char* name, void* ctx, char* helpstr);

/* 
 * cmd_def_option
 * @name: opt name
 * @id:   opt alias id, for parse scan
 * @valuehint: value hint for help
 * @ctx:  create by cmd_ctx_create()
 * @helpstr: help string
 */
void cmd_def_option(char* name, int id, int flag, char* valuehint, void* ctx, char* helpstr);

#endif

