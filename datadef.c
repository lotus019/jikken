/*
 * datadef.c - データ定義モジュール
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "microdb.h"

/*
 * DEF_FILE_EXT -- データ定義ファイルの拡張子
 */
#define DEF_FILE_EXT ".def"

/*
 * initializeDataDefModule -- データ定義モジュールの初期化
 *
 * 引数:
 *	なし
 *
 * 返り値;
 *	成功ならOK、失敗ならNGを返す
 */
Result initializeDataDefModule()
{
}

/*
 * finalizeDataDefModule -- データ定義モジュールの終了処理
 *
 * 引数:
 *	なし
 *
 * 返り値;
 *	成功ならOK、失敗ならNGを返す
 */
Result finalizeDataDefModule()
{
}

/*
 * createTable -- 表(テーブル)の作成
 *
 * 引数:
 *	tableName: 作成する表の名前
 *	tableInfo: データ定義情報
 *
 * 返り値:
 *	成功ならOK、失敗ならNGを返す
 *
 * データ定義ファイルの構造(ファイル名: tableName.def)
 *   +-------------------+----------------------+-------------------+----
 *   |フィールド数       |フィールド名          |データ型           |
 *   |(sizeof(int)バイト)|(MAX_FIELD_NAMEバイト)|(sizeof(int)バイト)|
 *   +-------------------+----------------------+-------------------+----
 * 以降、フィールド名とデータ型が交互に続く。
 */
Result createTable(char *tableName, TableInfo *tableInfo)
{ 

  int i,len;
  char *filename;
  char page[PAGE_SIZE];
  char *p;

  /* [tableName].defという文字列を作る */
  len = strlen(tableName) + strlen(DEF_FILE_EXT) + 1;
  if ((filename = malloc(len)) == NULL) {
      return NG;
  }
  snprintf(filename, len, "%s%s", tableName, DEF_FILE_EXT);


  if (createFile(filename) != OK) {
    return NG;
  }

  File *file;

  if ((file = openFile(filename)) == NULL) {
    return NG;
  }

  /* ページの内容をクリアする */
  memset(page, 0, PAGE_SIZE);
  p = page;

　/* ページの先頭にフィールド数を保存する */
  memcpy(p, &tableInfo->numField, sizeof(tableInfo->numField));
  p += sizeof(tableInfo->numField);

  /* それぞれのフィールドについて、フィールド名とデータ型をpageに記録する */
  for (i = 0; i < tableInfo->numField; i++){
    memcpy(p, &tableInfo->fieldInfo[i]->name, MAX_FIELD_NAME);
    p += MAX_FIELD_NAME;
    memcpy(p, &tableInfo->fieldInfo[i].dataType, sizeof(tableInfo->fieldInfo.dataType))
    p += sizeof(tableInfo->fieldInfo.dataType);
  }

  /* ファイルの先頭ページ(ページ番号0)に1ページ分のデータを書き込む */
  if (writePage(file, 0, page) != OK) {
      return NG;
  }

  if (closeFile(file) == NG) {
  return NG;
  }

  return OK;

}

/*
 * dropTable -- 表(テーブル)の削除
 *
 * 引数:
 *	??????
 *
 * 返り値:
 *	??????
 */
Result dropTable(char *tableName)
{
  strcat(tableName,DEF_FILE_EXT);
  deleteFile();
  if (deleteFile(tableName) != OK) {
    return NG;
  }

  return OK;
}

/*
 * getTableInfo -- 表のデータ定義情報を取得する関数
 *
 * 引数:
 *	tableName: 情報を表示する表の名前
 *
 * 返り値:
 *	tableNameのデータ定義情報を返す
 *	エラーの場合には、NULLを返す
 *
 * ***注意***
 *	この関数が返すデータ定義情報を収めたメモリ領域は、不要になったら
 *	必ずfreeTableInfoで解放すること。
 */
TableInfo *getTableInfo(char *tableName)
{
  int i,len;
  char *filename;
  char page[PAGE_SIZE];
  File *file;
  char *p

  /* [tableName].defという文字列を作る */
  len = strlen(tableName) + strlen(DEF_FILE_EXT) + 1;
  if ((filename = malloc(len)) == NULL) {
      return NG;
  }
  snprintf(filename, len, "%s%s", tableName, DEF_FILE_EXT);
  if ((file = openFile(filename)) == NULL) {
    return NULL;
  }

  TableInfo *table 
  table = malloc(sizeof(TableInfo));
  if (table == NULL) {
    return NULL;
  }

  if (readPage(file, 0, page) != OK) {
    return NULL;
  }

  p = page;

  memcpy(&table->numField, p, sizeof(table->numField));
  p += sizeof(tableInfo->numField);  

  for (i = 0; i < tableInfo->numField; i++){
    memcpy(&tableInfo->fieldInfo[i]->name, p, MAX_FIELD_NAME);
    p += MAX_FIELD_NAME;
    memcpy(&tableInfo->fieldInfo[i].dataType, p, sizeof(tableInfo->fieldInfo.dataType))
    p += sizeof(tableInfo->fieldInfo.dataType);
  }

  if (closeFile(file) == NG) {
  return NULL;
  }

  return table;
}

/*
 * freeTableInfo -- データ定義情報を収めたメモリ領域の解放
 *
 * 引数:
 *	tableNameのデータ定義情報
 *
 * 返り値:
 *	なし
 *
 * ***注意***
 *	関数getTableInfoが返すデータ定義情報を収めたメモリ領域は、
 *	不要になったら必ずこの関数で解放すること。
 */
void freeTableInfo(TableInfo *table)
{ 
  free(table);
}
