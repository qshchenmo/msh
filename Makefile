SOURCE   = $(wildcard ./src/*.c)
OBJ      = $(patsubst %.c, %.o, $(SOURCE))
BINDIR   = /usr/local/bin
INCLUDES = -I ./inc
LIBS     = -lcurses -ldl
CFLAGS   = -Wall -c -g 
TARGET   = mshd
CC       = gcc

$(TARGET): $(OBJ)	
	$(CC) -rdynamic $(OBJ) $(LIB_PATH) $(LIBS) -o $(TARGET)
%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $< -o $@
.PHONY: install clean
install:
	install -d $(BINDIR)
	install -m 0755 $(TARGET) $(BINDIR)
clean:
	rm -rf $(OBJ) $(TARGET) 


