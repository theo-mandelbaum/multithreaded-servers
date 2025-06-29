#
# Simple Test Makefile
# Mike Lam, James Madison University, August 2016
#
# This version of the Makefile includes support for building a test suite. The
# recommended framework is Check (http://check.sourceforge.net/). To build and
# run the test suite, execute the "test" target. The test suite must be located
# in a module called "testsuite". The MODS, LIBS, and OBJS variables work as
# they do in the main Makefile.
#
# To change the default build target (which executes when you just type
# "make"), change the right-hand side of the definition of the "default"
# target.
#
# By default, this makefile will build the project with debugging symbols and
# without optimization. To change this, edit or remove the "-g" and "-O0"
# options in CFLAGS and LDFLAGS accordingly.
#
# By default, this makefile build the application using the GNU C compiler,
# adhering to the C99 standard with all warnings enabled.


# application-specific settings and run target

EXE=../interp
TEST=testsuite
MODS=public.o
OBJS=../port_utils.o
LIBS=

UTESTOUT=utests.txt
ITESTOUT=itests.txt
SCHECKOUT=style.txt

default: $(TEST)



$(EXE):
	make -C ../

test: utest itest style
	@echo "========================================"

utest: $(EXE) $(TEST)
	@echo "========================================"
	@echo "             UNIT TESTS"
	@./$(TEST) 2>/dev/null >$(UTESTOUT)
	@cat $(UTESTOUT) | sed -n -e '/Checks/,$$p' | sed -e 's/^private.*:[EF]://g'

itest: $(EXE)
	@echo "========================================"
	@echo "          INTEGRATION TESTS"
	@./integration.sh | tee $(ITESTOUT)

style: $(EXE)
	@echo "========================================"
	@echo "          CODING STYLE CHECK"
	@./style.sh 2>/dev/null >$(SCHECKOUT)
	@cat $(SCHECKOUT) 

# compiler/linker settings

CC=gcc
CFLAGS=-g -O0 -Wall --std=gnu99 -pedantic
LDFLAGS=-g -O0

#CKCFLAGS=
CKLIBS+=-lcheck -lm -lpthread

ifeq ($(shell uname -s),Linux)
	CKLIBS+=-lrt -lsubunit
endif


# build targets

$(TEST): $(TEST).o $(MODS) $(OBJS)
	$(CC) -c testsuite.c -o testsuite.o
	$(CC) $(LDFLAGS) -o $(TEST) $^ $(CKLIBS)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

clean:
	rm -rf $(TEST) $(TEST).o $(MODS) $(UTESTOUT) $(ITESTOUT) $(SCHECKOUT) outputs valgrind ckstyle $(COBJS)

.PHONY: default clean test unittest inttest

