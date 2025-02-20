# make              # to compile files and create the executables
# make pgm          # to download example images to the pgm/ dir
# make setup        # to setup the test files in test/ dir
# make tests        # to run basic tests
# make clean        # to cleanup object files and executables
# make cleanobj     # to cleanup object files only

CFLAGS = -Wall -Wextra -Wpedantic -O3 -g

PROGS = imageTool imageTest benchmark

TESTS = test1 test2 test3 test4 test5 test6 test7 test8 test9

# Default rule: make all programs
all: $(PROGS)

benchmark: benchmark.o image8bit.o instrumentation.o error.o

imageTest: imageTest.o image8bit.o instrumentation.o error.o

imageTest.o: image8bit.h instrumentation.h

imageTool: imageTool.o image8bit.o instrumentation.o error.o

imageTool.o: image8bit.h instrumentation.h

IMAGE_TOOL_RUN = ./imageTool

# Rule to make any .o file dependent upon corresponding .h file
%.o: %.h

pgm:
	wget -O- https://sweet.ua.pt/jmr/aed/pgm.tgz | tar xzf -

.PHONY: setup valgrind
setup: test/

test/:
	wget -O- https://sweet.ua.pt/jmr/aed/test.tgz | tar xzf -
	@#mkdir -p $@
	@#curl -s -o test/aed-trab1-test.zip https://sweet.ua.pt/mario.antunes/aed/test/aed-trab1-test.zip
	@#unzip -q -o test/aed-trab1-test.zip -d test/

valgrind:
	$(eval IMAGE_TOOL_RUN := valgrind $(IMAGE_TOOL_RUN))

test1: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm neg save neg.pgm
	cmp neg.pgm test/neg.pgm

test2: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm thr 128 save thr.pgm
	cmp thr.pgm test/thr.pgm

test3: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm bri .33 save bri.pgm
	cmp bri.pgm test/bri.pgm

test4: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm rotate save rotate.pgm
	cmp rotate.pgm test/rotate.pgm

test5: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm mirror save mirror.pgm
	cmp mirror.pgm test/mirror.pgm

test6: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm crop 100,100,100,100 save crop.pgm
	cmp crop.pgm test/crop.pgm

test7: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/small.pgm test/original.pgm paste 100,100 save paste.pgm
	cmp paste.pgm test/paste.pgm

test8: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/small.pgm test/original.pgm blend 100,100,.33 save blend.pgm
	cmp blend.pgm test/blend.pgm

test9: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm tic blur 7,7 toc save blur.pgm
	cmp blur.pgm test/blur.pgm

# Custom Tests
testCreate: $(PROGS) setup
	$(IMAGE_TOOL_RUN) create 10,10 save black.pgm
	cmp black.pgm test/black.pgm

testInfo: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/original.pgm info | cmp - test/info.out

testLocate: $(PROGS) setup
	$(IMAGE_TOOL_RUN) test/black.pgm test/original.pgm locate | cmp - test/locate.out

.PHONY: tests
tests: $(TESTS)

# Make uses builtin rule to create .o from .c files.

cleanobj:
	rm -f *.o

clean: cleanobj
	rm -f $(PROGS)

