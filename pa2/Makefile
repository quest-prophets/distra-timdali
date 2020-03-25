all:
	clang -g -std=c99 -Wall -pedantic -o pa2 -Llib64 -lruntime *.c

format:
	clang-format -i -style=Chromium *.c

run: all
	LD_LIBRARY_PATH="$LD_LIBRARY_PATH:lib64" LD_PRELOAD=libruntime.so ./pa2 -p 3 10 20 30

%:
	@:
