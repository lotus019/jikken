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
  return OK;
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
  return OK;
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
  if (createDataFile(tableName) != OK){
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
    memcpy(p, &tableInfo->fieldInfo[i].name, MAX_FIELD_NAME);
    p += MAX_FIELD_NAME;
    memcpy(p, &tableInfo->fieldInfo[i].dataType, sizeof(tableInfo->fieldInfo[i].dataType));
    p += sizeof(tableInfo->fieldInfo[i].dataType);
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
  int len;
  char *filename;

  /* [tableName].defという文字列を作る */
  len = strlen(tableName) + strlen(DEF_FILE_EXT) + 1;
  if ((filename = malloc(len)) == NULL) {
      return NG;
  }  
  snprintf(filename, len, "%s%s", tableName, DEF_FILE_EXT);


  if (deleteFile(filename) != OK) {
    return NG;
  }
  if (deleteDataFile(tableName) != OK){
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
  char *p;

  /* [tableName].defという文字列を作る */
  len = strlen(tableName) + strlen(DEF_FILE_EXT) + 1;
  if ((filename = malloc(len)) == NULL) {
      return NULL;
  }
  snprintf(filename, len, "%s%s", tableName, DEF_FILE_EXT);
  if ((file = openFile(filename)) == NULL) {
    return NULL;
  }

  TableInfo *table; 
  table = malloc(sizeof(TableInfo));
  if (table == NULL) {
    return NULL;
  }

  if (readPage(file, 0, page) != OK) {
    return NULL;
  }

  p = page;

  memcpy(&table->numField, p, sizeof(table->numField));
  p += sizeof(table->numField);  

  for (i = 0; i < table->numField; i++){
    memcpy(&table->fieldInfo[i].name, p, MAX_FIELD_NAME);
    p += MAX_FIELD_NAME;
    memcpy(&table->fieldInfo[i].dataType, p, sizeof(table->fieldInfo[i].dataType));
    p += sizeof(table->fieldInfo[i].dataType);
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

/*
 * printTableInfo -- テーブルのデータ定義情報を表示する(動作確認用)
 *
 * 引数:
 *	tableName: 情報を表示するテーブルの名前
 *
 * 返り値:
 *	なし
 */
void printTableInfo(char *tableName)
{
    TableInfo *tableInfo;
    int i;

    /* テーブル名を出力 */
    printf("\nTable %s\n", tableName);

    /* テーブルの定義情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
	/* テーブル情報の取得に失敗したので、処理をやめて返る */
	return;
    }

    /* フィールド数を出力 */
    printf("number of fields = %d\n", tableInfo->numField);

    /* フィールド情報を読み取って出力 */
    for (i = 0; i < tableInfo->numField; i++) {
	/* フィールド名の出力 */
	printf("  field %d: name = %s, ", i + 1, tableInfo->fieldInfo[i].name);

	/* データ型の出力 */
	printf("data type = ");
	switch (tableInfo->fieldInfo[i].dataType) {
	case TYPE_INTEGER:
	    printf("integer\n");
	    break;
	case TYPE_STRING:
	    printf("string\n");
	    break;
	default:
	    printf("unknown\n");
	}
    }

    /* データ定義情報を解放する */
    freeTableInfo(tableInfo);

    return;
}

