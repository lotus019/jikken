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
 * NUM_BUFFER -- ファイルアクセスモジュールが管理するバッファの大きさ(ページ数)
 */
#define NUM_BUFFER 4
#define UNDEFINED -1

/*
 * modifyFlag -- 変更フラグ
 */
typedef enum { UNMODIFIED = 0, MODIFIED = 1 } modifyFlag;

/*
 * Buffer -- 1ページ分のバッファを記憶する構造体
 */
typedef struct Buffer Buffer;
struct Buffer {
  File *file;   /* バッファの内容が格納されたファイル */
                    /* file == NULLならこのバッファは未使用 */
  int pageNum;            /* ページ番号 */
  char page[PAGE_SIZE];       /* ページの内容を格納する配列 */
  struct Buffer *prev;        /* 一つ前のバッファへのポインタ */
  struct Buffer *next;        /* 一つ後ろのバッファへのポインタ */
  modifyFlag modified;        /* ページの内容が更新されたかどうかを示すフラグ */
};

static Result initializeBufferList();
static Result finalizeBufferList();
static void moveBufferToListHead(Buffer *buf);

/*
 * bufferListHead -- LRUリストの先頭へのポインタ
 */
static Buffer *bufferListHead = NULL;

/*
 * bufferListTail -- LRUリストの最後へのポインタ
 */
static Buffer *bufferListTail = NULL;

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
 *  なし
 *
 * 返り値:
 *  成功の場合OK、失敗の場合NG
 */
Result initializeFileModule()
{
  initializeBufferList();
  return OK;
}

/*
 * finalizeFileModule -- ファイルアクセスモジュールの終了処理
 *
 * 引数:
 *  なし
 *
 * 返り値:
 *  成功の場合OK、失敗の場合NG
 */
Result finalizeFileModule()
{
  finalizeBufferList();
  return OK;
}

/*
 * createFile -- ファイルの作成
 *
 * 引数:
 *  filename: 作成するファイルのファイル名
 *
 * 返り値:
 *  成功の場合OK、失敗の場合NG
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
 *  成功の場合OK、失敗の場合NG
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
 *  filename: オープンするファイルのファイル名
 *
 * 返り値:
 *  オープンしたファイルのFile構造体
 *  オープンに失敗した場合にはNULLを返す
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
 *  クローズするファイルのFile構造体
 *
 * 返り値:
 *  成功の場合OK、失敗の場合NG
 */
Result closeFile(File *file)
{
  int i;
  Buffer *buf=bufferListHead;

  for (i = 0; i < NUM_BUFFER; i++){
    if (file==buf->file){
      if (lseek(file->desc,PAGE_SIZE*buf->pageNum,SEEK_SET)==-1){
        printErrorMessage(ERR_MSG_LSEEK);
        return NG;
      }
      /* writeシステムコールによるファイルへのアクセス */
      if (write(file->desc, buf->page, PAGE_SIZE) < PAGE_SIZE) {
        /* エラー処理 */
        printErrorMessage(ERR_MSG_WRITE);
        return NG;
      }
      buf->modified=UNMODIFIED;  
    }
    buf=buf->next;
  }

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
 *  file: アクセスするファイルのFile構造体
 *  pageNum: 読み出すページの番号
 *  page: 読み出した内容を格納するPAGE_SIZEバイトの領域
 *
 * 返り値:
 *  成功の場合OK、失敗の場合NG
 */
Result readPage(File *file, int pageNum, char *page)
{
  int i;
  Buffer *buf=bufferListHead;
  Buffer *emptyBuffer=NULL;

  /*バッファの中にページがあるか探す*/
  for (i = 0; i < NUM_BUFFER; i++){
    if (buf->pageNum==pageNum && file==buf->file){
      if ((memcpy(page,buf->page,PAGE_SIZE))==NULL) {
        /* エラー処理 */
        printErrorMessage(ERR_MSG_READ);
        return NG;
      }
      moveBufferToListHead(buf);
      return OK;
    }
    if (buf->modified==UNMODIFIED && emptyBuffer==NULL){
      emptyBuffer = buf;
    }
    buf=buf->next;
  }

  /*なかったらバッファに読み込む*/
  if (emptyBuffer==NULL){
    /* lseekシステムコールによる読み出し位置の移動 */
    if (lseek(bufferListTail->file->desc,PAGE_SIZE*bufferListTail->pageNum,SEEK_SET)==-1){
      printErrorMessage(ERR_MSG_LSEEK);
      return NG;
    }
    /* writeシステムコールによるファイルへのアクセス */
    if (write(bufferListTail->file->desc, bufferListTail->page, PAGE_SIZE) < PAGE_SIZE) {
      /* エラー処理 */
      printErrorMessage(ERR_MSG_WRITE);
      return NG;
    }
    bufferListTail->modified = UNMODIFIED;
    emptyBuffer = bufferListTail;  
  }

  /*bufferListTailの内容を変更する*/
  emptyBuffer->file = file;
  emptyBuffer->pageNum=pageNum;
  /* lseekシステムコールによる読み出し位置の移動 */
  if (lseek(file->desc,PAGE_SIZE*pageNum,SEEK_SET)==-1){
    printErrorMessage(ERR_MSG_LSEEK);
    return NG;
  }

  /* readシステムコールによるファイルへのアクセス */
  if (read(file->desc, emptyBuffer->page, PAGE_SIZE) < PAGE_SIZE) {
    /* エラー処理 */
    printErrorMessage(ERR_MSG_READ);
    return NG;
  }

  if ((memcpy(page,emptyBuffer->page,PAGE_SIZE))==NULL) {
    /* エラー処理 */
    printErrorMessage(ERR_MSG_READ);
    return NG;
  }
  moveBufferToListHead(emptyBuffer);

  return OK;

}

/*
 * writePage -- 1ページ分のデータのファイルへの書き出し
 *
 * 引数:
 *  file: アクセスするファイルのFile構造体
 *  pageNum: 書き出すページの番号
 *  page: 書き出す内容を格納するPAGE_SIZEバイトの領域
 *
 * 返り値:
 *  成功の場合OK、失敗の場合NG
 */
Result writePage(File *file, int pageNum, char *page)
{
  int i;
  Buffer *buf=bufferListHead;
  Buffer *emptyBuffer=NULL;
  /*バッファの中にページがあるか探す*/
  for (i = 0; i < NUM_BUFFER; i++){
    if (buf->pageNum==pageNum && file==buf->file){
      if ((memcpy(buf->page,page,PAGE_SIZE))==NULL) {
        /* エラー処理 */
        printErrorMessage(ERR_MSG_WRITE);
        return NG;
      }
      buf->modified = MODIFIED;
      moveBufferToListHead(buf);
      return OK;
    }
    if (buf->modified==UNMODIFIED && emptyBuffer==NULL){
      emptyBuffer = buf;
    }
    buf=buf->next;
  }
  
  /*なかったらbufferListTailを書き戻し、読み込む*/
  if (emptyBuffer==NULL){
    /* lseekシステムコールによる読み出し位置の移動 */
    if (lseek(bufferListTail->file->desc,PAGE_SIZE*bufferListTail->pageNum,SEEK_SET)==-1){
      printErrorMessage(ERR_MSG_LSEEK);
      return NG;
    }
    /* writeシステムコールによるファイルへのアクセス */
    if (write(bufferListTail->file->desc, bufferListTail->page, PAGE_SIZE) < PAGE_SIZE) {
      /* エラー処理 */
      printErrorMessage(ERR_MSG_WRITE);
      return NG;
    }  
    bufferListTail->modified=UNMODIFIED;
    emptyBuffer=bufferListTail;
  }

  /*bufferListTailの内容を変更する*/
  emptyBuffer->file = file;
  emptyBuffer->pageNum=pageNum;
  
  /*バッファに書き込む*/
  if ((memcpy(emptyBuffer->page,page,PAGE_SIZE))==NULL) {
    /* エラー処理 */
    printErrorMessage(ERR_MSG_WRITE);
    return NG;
  }
  emptyBuffer->modified = MODIFIED;
  moveBufferToListHead(emptyBuffer);
  return OK;

}



/*
 * getNumPage -- ファイルのページ数の取得
 *
 * 引数:
 *  filename: ファイル名
 *
 * 返り値:
 *  引数で指定されたファイルの大きさ(ページ数)
 *  エラーの場合には-1を返す
 */
int getNumPages(char *filename)
{
  if (stat(filename, &stbuf) == -1) {
    printErrorMessage(ERR_MSG_READ);
    return -1;
  }

  return stbuf.st_size/PAGE_SIZE;
}

/*
 * initializeBufferList -- バッファリストの初期化
 *
 * **注意**
 *  この関数は、ファイルアクセスモジュールを使用する前に必ず一度だけ呼び出すこと。
 *  (initializeFileModule()から呼び出すこと。)
 *
 * 引数:
 *  なし
 *
 * 返り値:
 *  初期化に成功すればOK、失敗すればNGを返す。
 */
static Result initializeBufferList()
{
  Buffer *oldBuf = NULL;
  Buffer *buf;
  int i;

  /*
   * NUM_BUFFER個分のバッファを用意し、
   * ポインタをつないで両方向リストにする
  */
  for (i = 0; i < NUM_BUFFER; i++) {
    /* 1個分のバッファ(Buffer構造体)のメモリ領域の確保 */
    if ((buf = (Buffer *) malloc(sizeof(Buffer))) == NULL) {
      /* メモリ不足なのでエラーを返す */
      return NG;
    }

    /* Buffer構造体の初期化 */
    buf->file = NULL;
    buf->pageNum = UNDEFINED;
    buf->modified = UNMODIFIED;
    memset(buf->page, 0, PAGE_SIZE);
    buf->prev = NULL;
    buf->next = NULL;

    /* ポインタをつないで両方向リストにする */
    if (oldBuf != NULL) {
        oldBuf->next = buf;
    }
    buf->prev = oldBuf;

    /* リストの一番最初の要素へのポインタを保存 */
    if (buf->prev == NULL) {
        bufferListHead = buf;
    }

    /* リストの一番最後の要素へのポインタを保存 */
    if (i == NUM_BUFFER - 1) {
        bufferListTail = buf;
    }

    /* 次のループのために保存 */
    oldBuf = buf;
  }

  return OK;
}

/*
 * finalizeBufferList -- バッファリストの終了処理
 *
 * **注意**
 *  この関数は、ファイルアクセスモジュールの使用後に必ず一度だけ呼び出すこと。
 *  (finalizeFileModule()から呼び出すこと。)
 *
 * 引数:
 *  なし
 *
 * 返り値:
 *  終了処理に成功すればOK、失敗すればNGを返す。
 */
static Result finalizeBufferList()
{      
  int i;
  Buffer *buf=bufferListHead;   
  Buffer *nbuffer = NULL;
  for (i = 0; i < NUM_BUFFER; i++){
    if (buf->modified==1){
      if(writePage(buf->file,buf->pageNum,buf->page)==NG){
        return NG;
      }        
    }

    nbuffer=buf->next;
    free(buf);
    buf=nbuffer;
    bufferListHead=buf;
  }
  return OK;
}

/*
 * moveBufferToListHead -- バッファをリストの先頭へ移動
 *
 * 引数:
 *  buf: リストの先頭に移動させるバッファへのポインタ
 *
 * 返り値:
 *  なし
 */
static void moveBufferToListHead(Buffer *buf)
{
  int i;
  Buffer *nbuffer;
  if (bufferListHead==buf){
     
  }else if (bufferListTail==buf){
    bufferListTail = bufferListTail->prev;
    bufferListTail->next = NULL;
    buf->next = bufferListHead;
    bufferListHead -> prev = buf;
    bufferListHead = buf;
  }else{
    buf->prev->next = buf->next;
    buf->next->prev = buf->prev;
    buf->next=bufferListHead;
    bufferListHead->prev=buf;
    buf->prev = NULL;
    bufferListHead=buf;
  }
}