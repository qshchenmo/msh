#ifndef MSH_CMDTREE_INCLUDED
#define MSH_CMDTREE_INCLUDED



/* register one cmd */
void cmd_register(void* _ctx);


/* create new context   */
void* cmd_ctx_create(void);

/* destroy context @ctx */
void cmd_ctx_destroy(void* ctx);


//void test_cmd(void);
int test_cmd(void);

int cmd_help(char* input);

int cmd_tree_init(void);



#endif
