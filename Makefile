.PHONY: build server-only run-server run-client clean

build:
	./scripts/build.sh

server-only:
	./scripts/build.sh --server-only

run-server:
	./scripts/dev-server.sh

run-client:
	./scripts/dev-client.sh

clean:
	rm -rf build
