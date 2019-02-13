#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "cmd.h"
#include "internal.h"

int main(int argc, char* argv[]){

    cmd_tree_init();

    /* register internal command */
    internal_cmd_register();
    
    /* Terminal Subsystem Init */    
    msh_init();
    
    /* */
    msh_startshell();

    /* */
    msh_exit();
    
    return 0; 
}




