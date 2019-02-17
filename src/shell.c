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
#include "cmd.h"
#include "err.h"

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
    END_WITH_CTRLC,
}LINE_END_STATUS;



#define MSH_BUF_SIZE     256
#define MSH_PROMPT_SIZE  16
#define MSH_HISTORY_SIZE 64
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

typedef struct 
{
    char history[MSH_HISTORY_SIZE][MSH_BUF_SIZE];
    int cur;         /* current position    */
    int next;        /* next write position */
    int used;        /* saved input, must <= MSH_HISTORY_SIZE */
}msh_history;

#define MSH_KEY_CTRLC 3
#define MSH_KEY_ESC 0x1b
#define MSH_KEY_TAB   9
#define MSH_KEY_ENTER 0x0a
#define MSH_KEY_SPACE 0x20
#define MSH_KEY_SQUAE 91
#define MSH_KEY_WAVE  126
#define MSH_KEY_BACKSPACE0  8
#define MSH_KEY_BACKSPACE  127


static msh_shell msh_instance;
static msh_history msh_his;
    
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


static void msh_loadtext(char *pcString)
{
    size_t len;

    len = strlen(pcString);

    bzero(msh_instance.edit_buff, sizeof(msh_instance.edit_buff));

    (void)memcpy(&msh_instance.edit_buff[0], pcString, len);

    msh_instance.edit_end = len;
    msh_instance.edit_cursor = len;

    return;
}

/*
 * Insert one char to Editbuffer 
 */
static int msh_insert(char c){

    if (msh_instance.edit_end == (MSH_BUF_SIZE - 1))
    {
        return MSH_ERROR_FAILED;
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

    return MSH_ERROR_SUCCESS;
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

/*
 * Backspace one char
 */
static int msh_backspace(void){

    if (msh_instance.edit_cursor == 0){
        return MSH_ERROR_FAILED;
    }

    if (msh_instance.edit_cursor != msh_instance.edit_end){
        (void) memmove(&msh_instance.edit_buff[msh_instance.edit_cursor - 1], 
                       &msh_instance.edit_buff[msh_instance.edit_cursor], 
                       (size_t)(msh_instance.edit_end - msh_instance.edit_cursor));
    }

    msh_instance.edit_buff[msh_instance.edit_end - 1] = '\0';

    msh_instance.edit_cursor--;
    msh_instance.edit_end--;

    return MSH_ERROR_SUCCESS;
}

static int msh_ctrlc(void)
{
    msh_instance.edit_buff[0] = MSH_KEY_CTRLC;
    msh_instance.edit_buff[1] = '\0';
    msh_instance.edit_cursor  = 0;

    return MSH_ERROR_SUCCESS;
}

/*
 * Delete one character
 * 'abcde'  'abde'
 *    .        .
 */
static int msh_delete(void){

    if (msh_instance.edit_cursor >= msh_instance.edit_end){
        return MSH_ERROR_FAILED;
    }

    (void) memmove(&msh_instance.edit_buff[msh_instance.edit_cursor], 
                   &msh_instance.edit_buff[msh_instance.edit_cursor + 1], 
                   (size_t)(msh_instance.edit_end - msh_instance.edit_cursor));

    msh_instance.edit_end--;

    return MSH_ERROR_SUCCESS;
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

static int msh_up(void)
{        
    if (msh_his.cur == msh_his.next)
    {
        /* temp save editing buf */
        memcpy(msh_his.history[msh_his.next], msh_instance.edit_buff, MSH_BUF_SIZE);
    }
    
    if (((msh_his.used < MSH_HISTORY_SIZE) && (msh_his.cur > 0)) ||
        ((msh_his.used == MSH_HISTORY_SIZE) && ((msh_his.cur - 1 + MSH_HISTORY_SIZE) % MSH_HISTORY_SIZE != msh_his.next))
       )
    {
        msh_his.cur = (msh_his.cur - 1 + MSH_HISTORY_SIZE) % MSH_HISTORY_SIZE;
        msh_loadtext(msh_his.history[msh_his.cur]);
    }
    
    return MSH_ERROR_SUCCESS;
}

static int msh_down(void)
{
    if (msh_his.cur == msh_his.next)
    {
        return MSH_ERROR_SUCCESS;   
    }

    msh_his.cur = (msh_his.cur + 1) % MSH_HISTORY_SIZE;
    msh_loadtext(msh_his.history[msh_his.cur]);
    
    return MSH_ERROR_SUCCESS;
}

static int msh_left(void)
{
    if (0 == msh_instance.edit_cursor)
    {
        return MSH_ERROR_FAILED;
    }

    msh_instance.edit_cursor--;

    return MSH_ERROR_SUCCESS;
}

static int msh_right(void)
{
    if (msh_instance.edit_cursor >= msh_instance.edit_end)
    {
        return MSH_ERROR_FAILED;
    }

    msh_instance.edit_cursor++;

    return MSH_ERROR_SUCCESS;
}

/*
 *  ESC multiplexer 
 *  return  0: success
 *         -1: fail
 */
static int msh_esc_mux(void)
{
    int key;
    int ret = MSH_ERROR_FAILED;

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
            return MSH_ERROR_FAILED;
        }
    }

    switch (key)
    {
        case 'A' :
        {
            /* up */
            ret = msh_up();
            break;
        }
        case 'B' :
        {
            /* down */
            ret = msh_down();
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

    if (MSH_ERROR_FAILED == ret)
        msh_getchar_untilEOF();

    return ret;
}

/*
 * Read one line, store it in @cmdstring
 */
LINE_END_STATUS msh_getcmd(char* cmdstring){
    int key;
    int err = MSH_ERROR_FAILED;
    LINE_END_STATUS end = END_WITH_ENTER;
    
    for (;;){

        /* display screen */
        msh_refresh();

        err = MSH_ERROR_FAILED;
        
        /* get one char */
        key = msh_getchar(0);
        if (is_printful(key))
        {   
            err = msh_insert((char)key);
        }
        else if (MSH_KEY_ESC == key)
        {    
            err = msh_esc_mux();
        }
        else if ((MSH_KEY_BACKSPACE == key) || (MSH_KEY_BACKSPACE0 == key))
        {
            err = msh_backspace();
        }
        else if (MSH_KEY_TAB == key)
        {
            end = END_WITH_TAB;
            break;
        }
        else if (MSH_KEY_ENTER == key)
        {
            end = END_WITH_ENTER;
            break;
        }
        else if (MSH_KEY_CTRLC == key)
        {
            end = END_WITH_CTRLC;
            err = msh_ctrlc();
            break;
        }
        
        if (MSH_ERROR_SUCCESS != err) 
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

static void msh_history_save(char* usr_input)
{
    if ('\0' == usr_input[0])
    {
        return;
    }

    bzero(msh_his.history[msh_his.next], MSH_BUF_SIZE);
    
    memcpy(msh_his.history[msh_his.next], usr_input, strlen(usr_input));

    msh_his.next++;
    if (MSH_HISTORY_SIZE == msh_his.next)
    {
        msh_his.next = 0;
    }
    msh_his.cur = msh_his.next;
    
    if (MSH_HISTORY_SIZE > msh_his.used)
    {
        msh_his.used++;
    }
    
    return;
}

static void msh_preprocess(char* usr_input)
{
    char* ptr = usr_input;
    int i;

    while(*ptr == ' ')
    {
        ptr++;
    }

    i = 0;
    while(*ptr != '\0')
    {
        usr_input[i] = *ptr++;
        while((usr_input[i] == ' ') && (*ptr == ' '))
        {
            ptr++;
        }
        i++;
    }

    usr_input[i] = '\0';

    return;
}

static int msh_process(LINE_END_STATUS end, char* usr_input)
{
    int err = MSH_ERROR_SUCCESS;
    
    msh_preprocess(usr_input);

    if (end == END_WITH_TAB)
    {
        cmd_tab(usr_input);
    
        return err;
    }
    else if (end == END_WITH_ENTER)
    {
        msh_history_save(usr_input);
        
        err = cmd_exec(usr_input);
    }

    return err;
}

/*
 * start mshell
 */
void msh_startshell(void){

    char usr_input[MSH_BUF_SIZE];
    LINE_END_STATUS end;
    int err;
    
    do{

        msh_prompt();

        msh_resetbuff();
        
        if (END_WITH_TAB == end)
        {
            msh_loadtext(usr_input);
        }
        
        bzero(usr_input, MSH_BUF_SIZE);

        fflush(stdout);
        
        end = msh_getcmd(usr_input);
  
        err = msh_process(end, usr_input);
        
    }while(err != MSH_ERROR_QUIT);

    return;
}

unsigned long msh_init(void){

    bzero(&msh_instance, sizeof(msh_instance));

    bzero(&msh_his, sizeof(msh_his));
    
    msh_setprompt("msh # ");

    term_save_setting(&msh_instance.termios_backup);
    
    msh_cfg_termsetting();

    return MSH_ERROR_SUCCESS;
}


void msh_exit(void)
{
    term_restore_setting(&msh_instance.termios_backup);

    return;
}



