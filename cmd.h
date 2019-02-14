#ifndef MSH_CMD_INCLUDED
#define MSH_CMD_INCLUDED


typedef int (*cmd_handler)(void* ctx);

/* register one cmd */
void cmd_register(void* _ctx, cmd_handler handler);

/* create new context   */
void* cmd_ctx_create(void);

/* destroy context @ctx */
void cmd_ctx_destroy(void* ctx);

void cmd_def_keyword(char* name, void* _ctx, char* helpstr);
void cmd_def_option(char* name, int required, void* _ctx);


void cmd_tab(char* input);
int cmd_exec(char* input);

int cmd_tree_init(void);



#endif
