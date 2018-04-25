test:
	./node_modules/.bin/sarg \
		--require=ts-node/register \
		--bail test/*.ts

run_tester: build_tester
	./build/test && echo "OK"

build_tester:
	cc \
		-g -Isrc/addon \
		src/addon/encoder.c \
		src/addon/ieee754.c \
		src/addon/decoder.c \
		test/test.c -lm \
		-o build/test
	
release:
	./node_modules/.bin/tsc

.PHONY: test release run_tester 