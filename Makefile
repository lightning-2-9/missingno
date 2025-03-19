CC = gcc
CFLAGS = -Wall -Wextra -g

SRC = main.c missingno.c format_handler.c
OBJ = $(SRC:.c=.o)
TARGET = missingno

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)
