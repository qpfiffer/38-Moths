CFLAGS=-Werror -Wno-missing-field-initializers -Wextra -Wall -O0 -g3
INCLUDES=-pthread -I./include/
LIBS=-lm -lrt
COMMON_OBJ=benchmark.o http.o models.o db.o parson.o utils.o vector.o logging.o


all: bin test $(NAME)

clean:
	rm -f *.o
	rm -f greshunkel_test

test: greshunkel_test
greshunkel_test: greshunkel_test.o greshunkel.o vector.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o greshunkel_test $^

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c $<

bin: $(NAME)
$(NAME): $(COMMON_OBJ) grengine.o greshunkel.o server.o stack.o parse.o main.o parson.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(NAME) $^ $(LIBS)
