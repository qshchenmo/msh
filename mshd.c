#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "shell.h"
#include "cmd.h"

int main(int argc, char* argv[]){

    cmd_init();
    
    /* Terminal Subsystem Init */    
    msh_init();
    
    /* */
    msh_startshell();

    /* */
    msh_exit();
    
    return 0; 
}




