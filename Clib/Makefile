# simple_tree_sitter: the static library for macOS and Linux.
#
#   make           build libsimple_tree_sitter.a
#   make test      build and run the C tests
#   make queries   rebuild sts_queries.c after changing a file in ../queries
#   make clean
#
# Windows uses build.bat.

CC ?= cc
AR ?= ar
CFLAGS ?= -O2
STS_CFLAGS = -std=c11 -fPIC -fvisibility=hidden -D_POSIX_C_SOURCE=200112L -D_DEFAULT_SOURCE
TS = tree-sitter/lib
LANGUAGES = eiffel c json xml

LIB = libsimple_tree_sitter.a
OBJECTS = build/simple_tree_sitter.o build/sts_queries.o build/ts_lib.o \
	build/eiffel_parser.o build/c_parser.o build/json_parser.o \
	build/xml_parser.o build/xml_scanner.o

all: $(LIB)

$(LIB): $(OBJECTS)
	rm -f $@
	$(AR) rcs $@ $(OBJECTS)

build:
	mkdir -p build

build/simple_tree_sitter.o: simple_tree_sitter.c simple_tree_sitter.h | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -Wall -Wextra -I$(TS)/include -c simple_tree_sitter.c -o $@

build/sts_queries.o: sts_queries.c | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -c sts_queries.c -o $@

build/ts_lib.o: $(TS)/src/lib.c | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -I$(TS)/include -I$(TS)/src -c $(TS)/src/lib.c -o $@

build/eiffel_parser.o: grammars/eiffel/src/parser.c | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -Igrammars/eiffel/src -c grammars/eiffel/src/parser.c -o $@

build/c_parser.o: grammars/c/src/parser.c | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -Igrammars/c/src -c grammars/c/src/parser.c -o $@

build/json_parser.o: grammars/json/src/parser.c | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -Igrammars/json/src -c grammars/json/src/parser.c -o $@

build/xml_parser.o: grammars/xml/src/parser.c | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -Igrammars/xml/src -c grammars/xml/src/parser.c -o $@

build/xml_scanner.o: grammars/xml/src/scanner.c grammars/common/scanner.h | build
	$(CC) $(CFLAGS) $(STS_CFLAGS) -Igrammars/xml/src -c grammars/xml/src/scanner.c -o $@

queries: | build
	$(CC) $(CFLAGS) tools/embed_queries.c -o build/embed_queries
	build/embed_queries ../queries $(LANGUAGES) > build/sts_queries.c.new
	mv build/sts_queries.c.new sts_queries.c

build/sts_test: test/sts_test.c simple_tree_sitter.h $(LIB)
	$(CC) $(CFLAGS) -std=c11 -Wall -Wextra -I. test/sts_test.c $(LIB) -o $@

test: build/sts_test
	build/sts_test ../testing/fixtures ../queries

clean:
	rm -rf build $(LIB)

.PHONY: all queries test clean
