test:
	./node_modules/.bin/sarg \
		--require=ts-node/register \
		--bail test/*.ts

valgrind: build_tester
	valgrind \
		-v \
		--track-origins=yes \
		--leak-check=full \
		--show-leak-kinds=all \
		./build/test

run_tester: build_tester
	./build/test && echo "OK"

build_tester:
	cc \
		-g -Isrc \
		src/encoder.c \
		src/ieee754.c \
		src/decoder.c \
		test/test.c -lm \
		-o build/test
	
release:
	./node_modules/.bin/tsc

.PHONY: test release run_tester 