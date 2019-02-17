SOURCE   = $(wildcard ./src/*.c)
OBJ      = $(patsubst %.c, %.o, $(SOURCE))
INCLUDES = -I ./inc
LIBS     = -lcurses -ldl
CFLAGS   = -Wall -c -g 
TARGET   = mshd
CC       = gcc

$(TARGET): $(OBJ)	
	$(CC) -rdynamic $(OBJ) $(LIB_PATH) $(LIBS) -o $(TARGET)
%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $< -o $@
.PHONY: clean
clean:
	rm -rf $(OBJ) $(TARGET) 


