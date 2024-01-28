VERSION=0.5.6
CFLAGS=-Werror -Wno-ignored-qualifiers -Wno-missing-field-initializers -Wextra -Wall -O2 -ffunction-sections -fdata-sections
INCLUDES=-pthread -I./include/
LIBS=-lm -lrt
COMMON_OBJ=simple_sparsehash.o utils.o vector.o logging.o

SONAME=lib38moths.so
REALNAME=lib38moths.so.$(VERSION)

DESTDIR?=
PREFIX?=/usr/local
LIBDIR?=lib
INSTALL_LIB=$(DESTDIR)$(PREFIX)/$(LIBDIR)/
INSTALL_INCLUDE=$(DESTDIR)$(PREFIX)/include/38-moths/


all: lib test

clean:
	rm -f *.o
	rm -f greshunkel_test
	rm -f $(REALNAME)

test: greshunkel_test unit_test
greshunkel_test: greshunkel_test.o greshunkel.o logging.o vector.o
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -o greshunkel_test $^

unit_test: $(COMMON_OBJ) grengine.o utests.o greshunkel.o vector.o parse.o
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -o unit_test $^ -lm -lrt

%.o: ./src/%.c
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -c -fPIC $<

lib: $(REALNAME)
$(REALNAME): $(COMMON_OBJ) grengine.o greshunkel.o server.o parse.o
	$(CC) $(CFLAGS) $(LIB_INCLUDES) $(INCLUDES) -o $(REALNAME) -shared -Wl,-soname,${SONAME} $^ $(LIBS)

install: all
	@mkdir -p $(INSTALL_LIB)
	@mkdir -p $(INSTALL_INCLUDE)
	@install $(REALNAME) $(INSTALL_LIB)$(REALNAME)
	@cd $(INSTALL_LIB) && ln -fs $(REALNAME) $(SONAME)
	@cd $(INSTALL_LIB) && ln -fs $(REALNAME) $(SONAME).0
	@install ./include/*.h $(INSTALL_INCLUDE)
	@echo "38-moths installed to $(DESTDIR)$(PREFIX) :^)."

amalgamated:
	mkdir $@

vendor: all
	@cat include/utils.h include/vector.h include/greshunkel.h include/38-moths.h include/grengine.h include/logging.h include/parse.h include/server.h > amalgamated/38-moths.h
	@sed -i 's/\#include ".*"$$//g' ./amalgamated/38-moths.h
	@cat  src/vector.c src/utils.c src/grengine.c src/greshunkel.c src/logging.c src/parse.c src/server.c > ./amalgamated/38-moths.c
	@sed -i 's/\#include ".*"$$/#include "38-moths.h"/g' ./amalgamated/38-moths.c
	@cp Makefile.vendor ./amalgamated/Makefile
	@echo "38-moths amalgamated into ./amalgamated :^)."

uninstall:
	rm -rf $(INSTALL_LIB)$(REALNAME)
	rm -rf $(INSTALL_LIB)$(SONAME).0
	rm -rf $(INSTALL_INCLUDE)
