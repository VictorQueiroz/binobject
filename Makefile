test:
	npx sarg \
		--require=ts-node/register \
		--bail test/test.ts && npx sarg \
		--require=ts-node/register \
		--bail \
		test/test_bindings.ts

release:
	npx tsc

.PHONY: test release run_tester 