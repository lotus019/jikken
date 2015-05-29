
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
microdb:

test-file: test-file.o file.o
	$(CC) -o test-file $(CFLAGS) test-file.o file.o

file.o: file.c microdb.h
	$(CC) -o file.o $(CFLAGS) -c file.c

test-file.o: test-file.c microdb.h
	$(CC) -o test-file.o $(CFLAGS) -c test-file.c

test-datadef: test-datadef.o datadef.o datamanip.o file.o
	$(CC) -o test-datadef $(CFLAGS) test-datadef.o datadef.o datamanip.o file.o

datamanip.o: datamanip.c microdb.h
	$(CC) -o datamanip.o $(CFLAGS) -c datamanip.c

datadef.o: datadef.c microdb.h
	$(CC) -o datadef.o $(CFLAGS) -c datadef.c

clean:
	rm -f *.o
