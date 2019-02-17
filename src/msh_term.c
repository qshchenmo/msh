#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <curses.h>
#include <sys/ioctl.h>
#include <term.h>
#include <errno.h>



#define MSH_INVALIDPTR  (char*)(-1)


unsigned int term_getcol(void)
{
    unsigned int colomn;
    struct winsize stSw;

    if ((0 == ioctl(STDIN_FILENO, TIOCGWINSZ, &stSw)) && (0 != stSw.ws_col)){
        colomn = stSw.ws_col;
    }
    else{
        colomn = tigetnum("cols");
    }

    return colomn;
}


void term_cursor_up(unsigned int line)
{
    char* tistr = NULL;
    char* emitstr = NULL;

    tistr = tigetstr("cuu");
    if ((MSH_INVALIDPTR != tistr) && (NULL != tistr)){
        emitstr = tparm(tistr, line);
        (void) putp(emitstr);
    }

    return;
}


void term_cursor_down(unsigned int line)
{
    char* tistr = NULL;
    char* emitstr = NULL;


    tistr = tigetstr("cud");
    if ((MSH_INVALIDPTR != tistr) && (NULL != tistr)){
        emitstr = tparm(tistr, line);
        (void) putp(emitstr);
    }

    return;
}

void term_cursor_back(unsigned int n)
{
    char *tistr = NULL;
    char *emitstr = NULL;

    tistr = tigetstr("cub");
    if ((MSH_INVALIDPTR != tistr) && (NULL != tistr)){
        emitstr = tparm(tistr, n);
        (void)putp(emitstr);
    }

    return;
}

void term_cursor_forward(unsigned long n)
{
    char *tistr = NULL;
    char *emitstr = NULL;

    tistr = tigetstr("cuf");
    if ((MSH_INVALIDPTR != tistr) && (NULL != tistr)){
        emitstr = tparm(tistr, n);
        (void)putp(emitstr);
    }

    return;
}


void term_erase(void)
{
    char *tistr = NULL;
    char *emitstr = NULL;

    tistr = tigetstr("el");
    if ((MSH_INVALIDPTR != tistr) && (NULL != tistr)){
        emitstr = tparm(tistr);
        (void)putp(emitstr);
    }

    return;

}

void term_bell(void)
{
    char *tistr = NULL;
    char *emitstr = NULL;

    tistr = tigetstr("bel");
    if ((MSH_INVALIDPTR != tistr) && (NULL != tistr)){
        emitstr = tparm(tistr);
        (void)putp(emitstr);
    }

    return;

}

void term_settermios(int fd, struct termios* t)
{
    int ret;

    for (;;)
    {
        ret = tcsetattr(fd, TCSANOW, t);
        if (0 == ret)
        {
            break;
        }
        else if (errno != EINTR)
        {
            break;
        }
    }

    return;
}

/*
 *  save termios setting to @t
 */
void term_save_setting(struct termios* t)
{
    int fd = fileno(stdin);
    setupterm(NULL, fd, (int*)0);
    tcgetattr(fd, t);

    return;
}

void term_restore_setting(struct termios* t)
{
    int fd = fileno(stdin);
    struct termios stNewTermios;
    int ret;

    ret = tcgetattr(fd, &stNewTermios);
    if (0 == ret)
    {
        stNewTermios.c_lflag = t->c_lflag;
        stNewTermios.c_cc[VMIN]  = t->c_cc[VMIN];
        stNewTermios.c_cc[VTIME] = t->c_cc[VTIME];

        stNewTermios.c_cc[VSUSP]  = t->c_cc[VSUSP];
        stNewTermios.c_cc[VSTART] = t->c_cc[VSTART];
        stNewTermios.c_cc[VSTOP] = t->c_cc[VSTOP];
        stNewTermios.c_cc[VQUIT] = t->c_cc[VQUIT];

        term_settermios(fd,&stNewTermios);
    }

    return;
}

