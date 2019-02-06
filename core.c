#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <curses.h>
#include <sys/ioctl.h>
#include <term.h>
#include <errno.h>

#include "msh_term.h"

typedef enum{
    ESC_UNKNOW,
    ESC_UP,
    ESC_DOWN,
    ESC_LEFT,
    ESC_RIGHT,
    ESC_DELETE,
}MSH_ESC_TYPE;

typedef enum{
    END_WITH_ENTER,
    END_WITH_TAB,
}LINE_END_STATUS;

typedef enum{
    PROCESS_CMD_EXEC,
    PROCESS_CMD_NOTFOUND,
    PROCESS_REDISPLAY,
    PROCESS_QUIT
}LINE_PROCESS_RESULT;




#define MSH_BUF_SIZE     256
#define MSH_PROMPT_SIZE  16

typedef struct
{
    char last_buff[MSH_BUF_SIZE];
    char edit_buff[MSH_BUF_SIZE];
    char prompt_buff[MSH_PROMPT_SIZE];
    unsigned int prompt_size;
    unsigned int edit_cursor;
    unsigned int edit_end;
    unsigned int last_cursor;
    unsigned int last_end;
    struct termios termios_backup;
}msh_shell;


#define MSH_KEY_ESC 0x1b
#define MSH_KEY_TAB   9
#define MSH_KEY_ENTER 0x0a
#define MSH_KEY_SPACE 0x20
#define MSH_KEY_SQUAE 91
#define MSH_KEY_WAVE  126
#define MSH_KEY_BACKSPACE  127


static msh_shell msh_instance;


static void msh_cfg_termsetting(void)
{
    struct termios stNewTermios;

    int fd = fileno(stdin);
    int ret;

    ret = tcgetattr(fd, &stNewTermios);
    if (0 == ret)
    {
        stNewTermios.c_lflag = 0;

        stNewTermios.c_cc[VMIN] = 0;
        stNewTermios.c_cc[VTIME] = 1;

        stNewTermios.c_cc[VSUSP] = _POSIX_VDISABLE;
        stNewTermios.c_cc[VSTART] = _POSIX_VDISABLE;
        stNewTermios.c_cc[VSTOP] = _POSIX_VDISABLE;
        stNewTermios.c_cc[VQUIT] = _POSIX_VDISABLE;

        term_settermios(fd,&stNewTermios);
    }

    return;
}



void COMSH_LINE_MaskSpecKey(void)
{
    
    int fd = fileno(stdin);
    struct termios stNewTermios;
    int iStatus;

    iStatus = tcgetattr(fd, &stNewTermios);

    if (0 == iStatus)
    {
    
    stNewTermios.c_cc[VSUSP] = _POSIX_VDISABLE;
     stNewTermios.c_cc[VSTART] = _POSIX_VDISABLE;
     stNewTermios.c_cc[VSTOP] = _POSIX_VDISABLE;
     stNewTermios.c_cc[VQUIT] = _POSIX_VDISABLE;
        term_settermios(fd,&stNewTermios);
    }

    return;
}


static void msh_cursor_move(unsigned int src, unsigned int dst){
    unsigned int colomn = term_getcol();
    unsigned int srcline, dstline, ulStartCols, ulEndCols;
    int line_flag = 0;

    if (src == dst){
        return;
    }

    srcline = (src + msh_instance.prompt_size) / colomn;
    dstline = (dst + msh_instance.prompt_size) / colomn;

    ulStartCols = (src + msh_instance.prompt_size) % colomn;
    ulEndCols = (dst + msh_instance.prompt_size) % colomn;

    if (srcline > dstline){
        term_cursor_up(srcline - dstline);
        line_flag = 1;
    }

    if (srcline < dstline){
        term_cursor_down(dstline - srcline);
        line_flag = 1;
    }

    if (1 == line_flag){
        term_cursor_back(colomn);
        term_cursor_forward(ulEndCols);
        return;
    }

    if (ulStartCols > ulEndCols){
        term_cursor_back(ulStartCols - ulEndCols);
    }
    else if (ulEndCols > ulStartCols){
        term_cursor_forward(ulEndCols - ulStartCols);
    }

    return;
}

static void msh_printf(char* pcStr){
    printf("%s", pcStr);

    return;
}

/*
static void msh_display(char *pcStr, unsigned long ulStartCols){
    unsigned long ulScrColumns;
    unsigned long ulPermitLenth;

    
}



void LINE_Set_SyncInput(int bSync)
{
    g_bComsh_SynInput = bSync;
    return;
}
*/

void msh_prompt(void)
{
    printf("\n");
    if ('\0' != msh_instance.prompt_buff[0])
    {
        printf("%s", msh_instance.prompt_buff);
    }
    
  
    return;
}

/*    
static void LINE_SyncInput(void){
    msh_shell *pstComshLine;

    if (0 == g_bComsh_SynInput)
        return;

    pstComshLine = &msh_instance;

    if(pstComshLine->edit_buff[0]!=0){
        printf("\r\n");
        msh_prompt();
        msh_printf(pstComshLine->edit_buff);
        msh_cursor_move(pstComshLine->edit_end,pstComshLine->edit_cursor);
    }

    g_bComsh_SynInput = 0;

    return;
}
*/

/*
 * read one byte from usr
 */
int msh_getchar(unsigned int timeout)
{
    int key;
    unsigned int to = 0;

    for (;;)
    {
        key = getchar();

        if (EOF != key)
        {
            break;
        }

        to++;

        if ((0 != timeout)&&(to >= timeout))
        {
            return -1;
        }
    }

    return key;
}


static void msh_getchar_untilEOF(void)
{
    while(EOF != msh_getchar(1))
    {
    }

    return;
}

static int msh_left()
{
    if (0 == msh_instance.edit_cursor)
    {
        return -1;
    }

    msh_instance.edit_cursor--;

    return 0;
}

static int msh_right()
{
    if (msh_instance.edit_cursor >= msh_instance.edit_end)
    {
        return -1;
    }

    msh_instance.edit_cursor++;

    return 0;
}


static void msh_loadtext(char *pcString)
{
    size_t len;
    unsigned int dst;

    len = strlen(pcString);

    dst = msh_instance.edit_cursor + len;

    if (msh_instance.edit_cursor < msh_instance.edit_end)
    {
        (void) memmove(&msh_instance.edit_buff[dst],  
                       &msh_instance.edit_buff[msh_instance.edit_cursor],  
                       (size_t)((msh_instance.edit_end + 1) - msh_instance.edit_cursor));
    }
    else 
    {
        msh_instance.edit_buff[dst] = '\0';
    }

    (void)memcpy(&msh_instance.edit_buff[msh_instance.edit_cursor], pcString, len);

    msh_instance.edit_end += len;
    msh_instance.edit_cursor += len;

    return;
}

/*
 * Insert one char to Editbuffer 
 */
static int msh_insert(char c){

    if (msh_instance.edit_end == MSH_BUF_SIZE){
        return -1;
    }

    if (msh_instance.edit_cursor != msh_instance.edit_end){
        (void) memmove(&msh_instance.edit_buff[msh_instance.edit_cursor + 1], 
                       &msh_instance.edit_buff[msh_instance.edit_cursor], 
                       (size_t)(msh_instance.edit_end - msh_instance.edit_cursor));
        
    }

    msh_instance.edit_buff[msh_instance.edit_cursor] = c;
    
    msh_instance.edit_cursor++;
    msh_instance.edit_end++;

    msh_instance.edit_buff[msh_instance.edit_end] = '\0';

    return 0;
}

/*
 * Backspace one char
 */
static int msh_backspace(void){

    if (msh_instance.edit_cursor == 0){
        return -1;
    }

    if (msh_instance.edit_cursor != msh_instance.edit_end){
        (void) memmove(&msh_instance.edit_buff[msh_instance.edit_cursor - 1], 
                       &msh_instance.edit_buff[msh_instance.edit_cursor], 
                       (size_t)(msh_instance.edit_end - msh_instance.edit_cursor));
    }

    msh_instance.edit_buff[msh_instance.edit_end - 1] = '\0';

    msh_instance.edit_cursor--;
    msh_instance.edit_end--;

    return 0;
}

/*
 * Delete one character
 * 'abcde'  'abde'
 *    .        .
 */
static int msh_delete(void){

    if (msh_instance.edit_cursor >= msh_instance.edit_end){
        return -1;
    }

    (void) memmove(&msh_instance.edit_buff[msh_instance.edit_cursor], 
                   &msh_instance.edit_buff[msh_instance.edit_cursor + 1], 
                   (size_t)(msh_instance.edit_end - msh_instance.edit_cursor));

    msh_instance.edit_end--;

    return 0;
}


/*
 *  ESC multiplexer 
 *  return  0: success
 *         -1: fail
 */
static int msh_esc_mux(void)
{
    int key;
    int ret = -1;

    key = msh_getchar(3);
    if (MSH_KEY_SQUAE == key)
    {
        key = msh_getchar(1);
    }
    else
    {
        if (('b' != key) && ('d' != key) && ('f' != key))
        {
            msh_getchar_untilEOF();
            return -1;
        }
    }

    switch (key)
    {
        case 'A' :
        {
            /* up */
            ret = 0;
            break;
        }
        case 'B' :
        {
            /* down */
            ret = 0;
            break;
        }
        case 'C':
        {
            /* right */
            ret = msh_right();
            break;
        }
        case 'D':
        {
            /* left  */
            ret = msh_left();
            break;
        }
        case '3':
        {
            key = msh_getchar(1);
            if (MSH_KEY_WAVE == key)
            {
                ret = msh_delete();
            }
            break;
        }
        default:
        {
             
             break;
        }
    }

    if (-1 == ret)
        msh_getchar_untilEOF();

    return ret;
}

static int is_printful(int key){
    if ((MSH_KEY_SPACE == key) || 
        ((key >= '0') && (key <= '9')) || 
        ((key >= 'A') && (key <= 'Z')) ||
        ((key >= 'a') && (key <= 'z'))
       ){
       return 1;
    }

    return 0;
}

static char* _msh_display(char* oldptr,
                          unsigned int freelen,
                          char* linebuff){
    char* newptr = oldptr;
    char* outptr = linebuff;
    unsigned int used = 0;

    while(('\0' != *newptr) && (used < freelen)){
        *outptr = *newptr;
        newptr++;
        outptr++;
        used++;
    }

    if (used >= freelen){
        *outptr = '\r';
        outptr++;

        *outptr = '\n';
        outptr++;
    }

    *outptr = '\0';
    
    return newptr;
}

/*
 *  display @str 
 */
static void msh_display(char* str)
{
    unsigned int column;
    unsigned int freelen;
    char* ptr;
    char linebuff[1024];

    column = term_getcol();

    bzero(linebuff, 1024);

    freelen = column - (msh_instance.prompt_size % column);

    ptr = str;

    while('\0' != *ptr)
    {
        bzero(linebuff, 1024);
        
        ptr = _msh_display(ptr, freelen, linebuff);

        msh_printf(linebuff);

        freelen = column;
    }

    return;
}

static void msh_refresh(void){

    msh_cursor_move(msh_instance.last_cursor, 0);

    term_erase();
    
    msh_display(msh_instance.edit_buff);
    
    msh_cursor_move(msh_instance.edit_end,msh_instance.edit_cursor);

    fflush(stdout);

    msh_instance.last_cursor = msh_instance.edit_cursor;
    msh_instance.last_end   = msh_instance.edit_end;
    memcpy(msh_instance.last_buff, msh_instance.edit_buff, MSH_BUF_SIZE);
    
    return;
} 

/*
 * Read one line, store it in @cmdstring
 */
LINE_END_STATUS msh_getcmd(char* cmdstring){
    int key;
    int err = -1;
    LINE_END_STATUS end = END_WITH_ENTER;
    
    for (;;){

        /* display screen */
        msh_refresh();

        err = -1;
        
        /* get one char */
        key = msh_getchar(0);
        if (is_printful(key)){
           
            err = msh_insert((char)key);
        }
        else if (MSH_KEY_ESC == key){
            
            err = msh_esc_mux();
        }
        else if (MSH_KEY_BACKSPACE == key){
            err = msh_backspace();
        }
        else if (MSH_KEY_TAB == key){
            end = END_WITH_TAB;
            break;
        }
        else if (MSH_KEY_ENTER == key){
            end = END_WITH_ENTER;
            break;
        }

        if (0 != err) 
        {
            term_bell();
        }
    }

    msh_instance.edit_buff[msh_instance.edit_cursor] = '\0';
    
    memcpy(cmdstring, msh_instance.edit_buff, msh_instance.edit_cursor);

    return end;
}

void msh_setprompt(const char* prompt){
    if (NULL == prompt){
        msh_instance.prompt_buff[0] = '\0';
        msh_instance.prompt_size = 0;
        return;
    }

    if ('\0' != *prompt){
        strncpy(msh_instance.prompt_buff, prompt, MSH_PROMPT_SIZE);
        msh_instance.prompt_size = strlen(msh_instance.prompt_buff);
    }

    return;
}

static void msh_resetbuff(void){
    msh_instance.edit_buff[0] = '\0';
    msh_instance.edit_end = 0;
    msh_instance.edit_cursor = 0;


    msh_instance.last_buff[0] = '\0';
    msh_instance.last_cursor = 0;
    msh_instance.last_end = 0;

    return;
}

static LINE_PROCESS_RESULT msh_process(LINE_END_STATUS end, char* pcCmdString)
{
    if (end == END_WITH_TAB)
    {
        return PROCESS_REDISPLAY;
    }

    return PROCESS_CMD_EXEC;

    /*
    unsigned ulCmdBufLen;
    unsigned ulRet = 0;
    char cEndChar;

    ulCmdBufLen = strlen(pcCmdString);
    cEndChar = pcCmdString[ulCmdBufLen - 1];


    if (COMSH_KEY_TAB != cEndChar)
    {

    }

    switch (cEndChar){
        case  COMSH_KEY_CTRL_Z:
        {
            printf("\r\n");
            ProcCtrlZ();
            break;
        }
        case COMSH_KEY_TAB:
        {
            printf("\r\n");
            {
                pcCmdString[ulCmdBufLen - 1] = '\0';
                COMSH_LINE_SetCmdString(pcCmdString);
            }
            break;
        }
        default:
            break;
    }
    */
}






void msh_startshell(void){

    char usr_input[256];
    LINE_END_STATUS end;
    LINE_PROCESS_RESULT result = PROCESS_CMD_EXEC;
    do{

        msh_prompt();

        msh_resetbuff();
        
        if (PROCESS_REDISPLAY == result)
        {
            msh_loadtext(usr_input);
        }
        
        bzero(usr_input, MSH_BUF_SIZE);

        fflush(stdout);
        
        end = msh_getcmd(usr_input);
        
        result = msh_process(end, usr_input);
        
    }while(result != PROCESS_QUIT);

    return;
}


unsigned long msh_init(void){
    unsigned  long i;
    unsigned  long ulRetCode;

    (void)memset(&msh_instance, 0, sizeof(msh_shell));

    msh_setprompt("msh > ");

    term_save_setting(&msh_instance.termios_backup);
    
    
    msh_cfg_termsetting();

    return 0;
}


void msh_exit(void)
{
    term_restore_setting(&msh_instance.termios_backup);

    return;
}



