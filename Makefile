VERSION=0.1
CFLAGS=-Werror -Wno-missing-field-initializers -Wextra -Wall -O2 -g3
INCLUDES=-pthread -I./include/
LIBS=-lm -lrt
COMMON_OBJ=utils.o vector.o logging.o
NAME=lib38moths.so

PREFIX?=/usr/local
INSTALL_LIB=$(PREFIX)/lib/
INSTALL_INCLUDE=$(PREFIX)/include/38-moths/


all: lib test

clean:
	rm -f *.o
	rm -f greshunkel_test
	rm -f $(NAME)

test: greshunkel_test
greshunkel_test: greshunkel_test.o greshunkel.o vector.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o greshunkel_test $^

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c -fPIC $<

lib: $(NAME)
$(NAME): $(COMMON_OBJ) grengine.o greshunkel.o server.o parse.o
	$(CC) $(CLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(NAME) -shared $^ $(LIBS)

install: lib
	@mkdir -p $(INSTALL_LIB)
	@mkdir -p $(INSTALL_INCLUDE)
	@install $(NAME) $(INSTALL_LIB)$(NAME).$(VERSION)
	@ln -fs $(INSTALL_LIB)$(NAME).$(VERSION) $(INSTALL_LIB)$(NAME)
	@ln -fs $(INSTALL_LIB)$(NAME).$(VERSION) $(INSTALL_LIB)$(NAME).0
	@install ./include/*.h $(INSTALL_INCLUDE)
	@ldconfig $(INSTALL_LIB)
	@echo "38-moths installed to $(PREFIX) :^)."

uninstall:
	rm -rf $(INSTALL_LIB)$(NAME).$(VERSION)
	rm -rf $(INSTALL_LIB)$(NAME)
	rm -rf $(INSTALL_LIB)$(NAME).0
	rm -rf $(INSTALL_INCLUDE)
