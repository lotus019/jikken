/*
 * file.c -- ファイルアクセスモジュール 
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "microdb.h"

File *file;
int pageNum;
char page[PAGE_SIZE];
struct stat stbuf;

/* エラーメッセージ番号 */
typedef enum ErrorMessageNo {
    ERR_MSG_CREATE = 0,
    ERR_MSG_UNLINK = 1,
    ERR_MSG_OPEN = 2,
    ERR_MSG_CLOSE = 3,
    ERR_MSG_READ = 4,
    ERR_MSG_WRITE = 5,
    ERR_MSG_LSEEK = 6,
    ERR_MSG_ACCESS = 7,
    ERR_MSG_STAT = 8,
} ErrorMessageNo;

/* エラーメッセージ */
char *errorMessage[] = {
    "ファイルの作成に失敗しました。",                   /* ERR_MSG_CREATE */
    "ファイルの削除に失敗しました。",                   /* ERR_MSG_UNLINK */
    "ファイルのオープンに失敗しました。",               /* ERR_MSG_OPEN */
    "ファイルのクローズに失敗しました。",               /* ERR_MSG_CLOSE */
    "ファイルからのデータの読み出しに失敗しました。",   /* ERR_MSG_READ */
    "ファイルへのデータの書き込みに失敗しました。",     /* ERR_MSG_WRITE */
    "ファイルのアクセス位置を変更に失敗しました。",     /* ERR_MSG_LSEEK */
    "ファイルの存在のチェックに失敗しました。",         /* ERR_MSG_ACCESS */
    "ファイルの大きさのチェックに失敗しました。",       /* ERR_MSG_STAT */
};

/*
 * printErrorMessage -- エラーメッセージの表示
 *
 * 引数:
 *   messageNo: 表示するメッセージ番号
 *
 * 返り値:
 *   なし
 */
void printErrorMessage(ErrorMessageNo messageNo) {
    printf("Error: %s\n", errorMessage[messageNo]);
}


/*
 * initializeFileModule -- ファイルアクセスモジュールの初期化処理
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result initializeFileModule()
{
    return OK;
}

/*
 * finalizeFileModule -- ファイルアクセスモジュールの終了処理
 *
 * 引数:
 *	なし
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result finalizeFileModule()
{
    return OK;
}

/*
 * createFile -- ファイルの作成
 *
 * 引数:
 *	filename: 作成するファイルのファイル名
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result createFile(char *filename)
{
    if (creat(filename,S_IRUSR|S_IWUSR)==-1){
        printErrorMessage(ERR_MSG_CREATE);
        return NG;
    }
    return OK;
}

/*
 * deleteFile -- ファイルの削除
 *
 * 引数:
 *  filename: 削除するファイルのファイル名
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result deleteFile(char *filename)
{
    if (access(filename, F_OK) == 0) {
        /* ファイルを削除 */
        if (unlink(filename) == -1) {
            printErrorMessage(ERR_MSG_UNLINK);
            return NG;
        }
    }

    return OK;
}

/*
 * openFile -- ファイルのオープン
 *
 * 引数:
 *	filename: オープンするファイルのファイル名
 *
 * 返り値:
 *	オープンしたファイルのFile構造体
 *	オープンに失敗した場合にはNULLを返す
 */
File *openFile(char *filename)
{
    File *file;

    /* File構造体の用意 */
    file = malloc(sizeof(File));
    if (file == NULL) {
        /* エラー処理 */
        printErrorMessage(ERR_MSG_OPEN);
        return NULL;
    }

    /*
    *ファイルのオープン
    */
    if ((file->desc = open(filename,O_RDWR))==-1){
        printErrorMessage(ERR_MSG_OPEN);
        return NULL;
    }

    /*
    *データの入力：ファイル名
    */
    strcpy(file->name,filename);

    return file;
}

/*
 * closeFile -- ファイルのクローズ
 *
 * 引数:
 *	クローズするファイルのFile構造体
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result closeFile(File *file)
{
    if (close(file->desc) == -1) {
        printErrorMessage(ERR_MSG_CLOSE);
        return NG;
    }

    /* File構造体の解放 */
    free(file);

    return OK;
}

/*
 * readPage -- 1ページ分のデータのファイルからの読み出し
 *
 * 引数:
 *	file: アクセスするファイルのFile構造体
 *	pageNum: 読み出すページの番号
 *	page: 読み出した内容を格納するPAGE_SIZEバイトの領域
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result readPage(File *file, int pageNum, char *page)
{
    /* lseekシステムコールによる読み出し位置の移動 */
    if (lseek(file->desc,PAGE_SIZE*pageNum,SEEK_SET)==-1){
      printErrorMessage(ERR_MSG_LSEEK);
      return NG;
    }

    /* readシステムコールによるファイルへのアクセス */
    if (read(file->desc, page, PAGE_SIZE) < PAGE_SIZE) {
        /* エラー処理 */
        printErrorMessage(ERR_MSG_READ);
        return NG;
    }

    return OK;

}

/*
 * writePage -- 1ページ分のデータのファイルへの書き出し
 *
 * 引数:
 *	file: アクセスするファイルのFile構造体
 *	pageNum: 書き出すページの番号
 *	page: 書き出す内容を格納するPAGE_SIZEバイトの領域
 *
 * 返り値:
 *	成功の場合OK、失敗の場合NG
 */
Result writePage(File *file, int pageNum, char *page)
{
    /* lseekシステムコールによる読み出し位置の移動 */
    if (lseek(file->desc,PAGE_SIZE*pageNum,SEEK_SET)==-1){
      printErrorMessage(ERR_MSG_LSEEK);
      return NG;
    }

    /* writeシステムコールによるファイルへのアクセス */
    if (write(file->desc, page, PAGE_SIZE) < PAGE_SIZE) {
        /* エラー処理 */
        printErrorMessage(ERR_MSG_WRITE);
        return NG;
    }

    return OK;

}



/*
 * getNumPage -- ファイルのページ数の取得
 *
 * 引数:
 *	filename: ファイル名
 *
 * 返り値:
 *	引数で指定されたファイルの大きさ(ページ数)
 *	エラーの場合には-1を返す
 */
int getNumPages(char *filename)
{
    if (stat(filename, &stbuf) == -1) {
        printErrorMessage(ERR_MSG_READ);
        return -1;
    }

    return stbuf.st_size/PAGE_SIZE;
}
