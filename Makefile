
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

clean:
	rm -f *.o
