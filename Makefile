test:
	./node_modules/.bin/sarg \
		--require=ts-node/register \
		--bail test/*.ts

release:
	./node_modules/.bin/tsc

.PHONY: test release