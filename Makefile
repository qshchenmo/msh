SOURCE   = $(wildcard ./*.c)
OBJ      = $(patsubst %.c, %.o, $(SOURCE))
INCLUDES = -I./h
LIBS     = -lcurses
#DEBUG    = -D_MACRO
CFLAGS   = -Wall -c -g
TARGET   = mshd
CC       = gcc

$(TARGET): $(OBJ)	
	$(CC) $(OBJ) $(LIB_PATH) $(LIBS) -o $(TARGET)
%.o: %.c
	$(CC) $(INCLUDES) $(DEBUG) $(CFLAGS) $< -o $@
.PHONY: clean
clean:
	rm -rf $(OBJ) $(TARGET) 


