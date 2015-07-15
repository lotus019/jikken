
CC = cc
CFLAGS = -g

# すべてのプログラムを作るルール
all: microdb all-test

# すべてのテストプログラムを作るルール
all-test: test-file test-datadef test-datamanip test-buffer

# すべてのテストプログラムを実行するルール
do-test: test-file test-datadef test-datamanip
	./test-file
	./test-datadef
	./test-datamanip

# 「microdb」を作成するためのルールは、今後追加される予定
# とりあえず、今のところは「何もしない」という設定にしておく。
microdb:main.o file.o datadef.o datamanip.o
	$(CC) -o microdb $(CFLAGS) main.o datadef.o datamanip.o file.o -lreadline -lcurses

main.o: main.c microdb.h
	$(CC) -o main.o $(CFLAGS) -c main.c

file.o: file2.c microdb.h
	$(CC) -o file.o $(CFLAGS) -c file2.c

test-file: test-file.o file.o
	$(CC) -o test-file $(CFLAGS) test-file.o file.o

test-file.o: test-file.c microdb.h
	$(CC) -o test-file.o $(CFLAGS) -c test-file.c

datadef.o: datadef.c microdb.h
	$(CC) -o datadef.o $(CFLAGS) -c datadef.c

test-datadef.o: test-datadef.c microdb.h
	$(CC) -o test-datadef.o $(CFLAGS) -c test-datadef.c

test-datadef: test-datadef.o datadef.o datamanip.o file.o
	$(CC) -o test-datadef $(CFLAGS) test-datadef.o datadef.o datamanip.o file.o

datamanip.o: datamanip.c microdb.h
	$(CC) -o datamanip.o $(CFLAGS) -c datamanip.c

test-datamanip.o: test-datamanip2.c microdb.h
	$(CC) -o test-datamanip.o $(CFLAGS) -c test-datamanip2.c

test-datamanip: test-datamanip.o datadef.o datamanip.o file.o
	$(CC) -o test-datamanip $(CFLAGS) test-datamanip.o datadef.o datamanip.o file.o

test-buffer: test-buffer.o file.o
	$(CC) -o test-buffer $(CFLAGS) test-buffer.o file.o

test-buffer.o: test-buffer.c microdb.h
	$(CC) -o test-buffer.o $(CFLAGS) -c test-buffer.c

clean:
	rm -f *.o
