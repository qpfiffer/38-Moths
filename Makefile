CFLAGS=-Werror -Wno-missing-field-initializers -Wextra -Wall -O0 -g3
INCLUDES=-pthread -I./include/
LIBS=-lm -lrt
COMMON_OBJ=utils.o vector.o logging.o http.o
NAME=libfutility.so


all: lib test

clean:
	rm -f *.o
	rm -f greshunkel_test

test: greshunkel_test
greshunkel_test: greshunkel_test.o greshunkel.o vector.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o greshunkel_test $^

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c $<

lib: $(NAME)
$(NAME): $(COMMON_OBJ) grengine.o greshunkel.o server.o parse.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(NAME) $^ $(LIBS)
