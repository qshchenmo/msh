#ifndef _MSH_TERM_H_
#define _MSH_TERM_H_

void term_bell(void);

void term_settermios(int fd, struct termios* t);

void term_save_setting(struct termios* t);
void term_restore_setting(struct termios* t);

unsigned int term_getcol(void);
void term_cursor_up(unsigned int line);
void term_cursor_down(unsigned int line);
void term_cursor_back(unsigned int n);
void term_cursor_forward(unsigned long n);
void term_erase(void);





#endif
