#ifndef MSH_CMDTREE_INCLUDED
#define MSH_CMDTREE_INCLUDED



/* register one cmd */
int cmd_register(char* path, void* ctx);

/* define new command element */
int cmd_def_element(char* name, char* help, unsigned int id, void* ctx);

/* create new context   */
void* cmd_ctx_create(void);

/* destroy context @ctx */
void cmd_ctx_destroy(void* ctx);


//void test_cmd(void);
int test_cmd(void);


int cmd_tree_init(void);



#endif
