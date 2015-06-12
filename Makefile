
CC = cc
CFLAGS = -g

# すべてのプログラムを作るルール
all: microdb all-test

# すべてのテストプログラムを作るルール
all-test: test-file

# すべてのテストプログラムを実行するルール
do-test: test-file
	./test-file

# 「microdb」を作成するためのルールは、今後追加される予定
# とりあえず、今のところは「何もしない」という設定にしておく。
microdb:main.o file.o datadef.o datamanip.o
	$(CC) -o microdb $(CFLAGS) main.o datadef.o datamanip.o file.o

main.o: main.c microdb.h
	$(CC) -o main.o $(CFLAGS) -c main.c

file.o: file.c microdb.h
	$(CC) -o file.o $(CFLAGS) -c file.c

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

test-datamanip.o: test-datamanip.c microdb.h
	$(CC) -o test-datamanip.o $(CFLAGS) -c test-datamanip.c

test-datamanip: test-datamanip.o datadef.o datamanip.o file.o
	$(CC) -o test-datamanip $(CFLAGS) test-datamanip.o datadef.o datamanip.o file.o


clean:
	rm -f *.o
