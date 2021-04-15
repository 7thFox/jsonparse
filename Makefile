bin/:
	mkdir bin

build: bin/
	g++ -o bin/jsonparse src/parse.cpp

bench: build
	bin/jsonparse -bench -noprint tests.json
	bin/jsonparse -bench -noprint default_systems.json