SOURCE   = $(wildcard *.c)
OBJ      = $(patsubst %.c, %.o, $(SOURCE))
TARGET   = libtest.cli
CFLAGS	 = -Wall -c -g
EXDIR    = /etc/msh/external
CC       = gcc


$(TARGET): $(OBJ)	
	$(CC) -fPIC -shared -o $(TARGET) $(SOURCE)
	mkdir -p $(EXDIR)
	cp $(TARGET) $(EXDIR)
%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $< -o $@
.PHONY: clean
clean:
	rm -rf $(OBJ) $(TARGET) 
	rm -rf $(EXDIR)/$(TARGET)
