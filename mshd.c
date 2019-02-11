#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core.h"
#include "cmd_tree.h"

int main(int argc, char* argv[]){

    cmd_tree_init();

    test_cmd();
    
    /* Terminal Subsystem Init */    
    msh_init();
    
    /* */
    msh_startshell();

    /* */
    msh_exit();
    
    return 0; 
}




