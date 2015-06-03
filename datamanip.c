/*
 * datamanip.c -- データ操作モジュール
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "microdb.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

/*
 * DAATA_FILE_EXT -- データファイルの拡張子
 */
#define DATA_FILE_EXT ".dat"

/*
 * initializeDataManipModule -- データ操作モジュールの初期化
 *
 * 引数:
 *  なし
 *
 * 返り値;
 *  成功ならOK、失敗ならNGを返す
 */
Result initializeDataManipModule()
{
    return OK;
}

/*
 * finalizeDataManipModule -- データ操作モジュールの終了処理
 *
 * 引数:
 *  なし
 *
 * 返り値;
 *  成功ならOK、失敗ならNGを返す
 */
Result finalizeDataManipModule()
{
    return OK;
}

/*
 * getRecordSize -- 1レコード分の保存に必要なバイト数の計算
 *
 * 引数:
 *  tableInfo: データ定義情報を収めた構造体
 *
 * 返り値:
 *  tableInfoのテーブルに収められた1つのレコードを保存するのに
 *  必要なバイト数
 */
static int getRecordSize(TableInfo *tableInfo)
{
    int total = 0;
    int i;

    for (i = 0; i < tableInfo->numField; i++) {
        /* i番目のフィールドがINT型かSTRING型か調べる */
        if (tableInfo->fieldInfo[i].dataType==TYPE_INTEGER){
          /* INT型ならtotalにsizeof(int)を加算 */
          total += sizeof(int);
        }else if (tableInfo->fieldInfo[i].dataType==TYPE_STRING){
          /* STRING型ならtotalにMAX_STRINGを加算 */
          total += MAX_STRING;
        }
      
    }

    /* フラグの分の1を足す */
    total++;

    return total;
}

/*
 * insertRecord -- レコードの挿入
 *
 * 引数:
 *  tableName: レコードを挿入するテーブルの名前
 *  recordData: 挿入するレコードのデータ
 *
 * 返り値:
 *  挿入に成功したらOK、失敗したらNGを返す
 */
Result insertRecord(char *tableName, RecordData *recordData)
{
    TableInfo *tableInfo;
    int numPage,len,recordSize,i,j;
    char *record;
    char *filename;
    File *file;
    char *p;
    char page[PAGE_SIZE]={0};

    /* テーブルの情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
        /* エラー処理 */
        return NG;
    }

    /* 1レコード分のデータをファイルに収めるのに必要なバイト数を計算する */
    recordSize = getRecordSize(tableInfo);
    assert(0<recordSize);

    /* 必要なバイト数分のメモリを確保する */
    if ((record = malloc(recordSize)) == NULL) {
        /* エラー処理 */
      return NG;
    }
    p = record;

    /* 先頭に、「使用中」を意味するフラグを立てる */
    memset(p, 1, 1);
    p += 1;

    /* 確保したメモリ領域に、フィールド数分だけ、順次データを埋め込む */
    for (i = 0; i < tableInfo->numField; i++) {
      switch (tableInfo->fieldInfo[i].dataType) {
        case TYPE_INTEGER:
          memcpy(p, &recordData->fieldData[i].valueSet, sizeof(int));
          p += sizeof(int);
          break;
        case TYPE_STRING:
          memcpy(p, &recordData->fieldData[i].valueSet, MAX_STRING);
          p += MAX_STRING;
        break;
        default:
          /* ここにくることはないはず */
            freeTableInfo(tableInfo);
            free(record);
            return NG;
      }
    }

    assert(record!=NULL);

    /* 使用済みのtableInfoデータのメモリを解放する */
    freeTableInfo(tableInfo);

    /*
     * ここまでで、挿入するレコードの情報を埋め込んだバイト列recordができあがる
     */


    /* [tableName].defという文字列を作る */
    len = strlen(tableName) + strlen(DATA_FILE_EXT) + 1;
    if ((filename = malloc(len)) == NULL) {
      return NG;
    }
    snprintf(filename, len, "%s%s", tableName, DATA_FILE_EXT);

    /* データファイルをオープンする */
    if ((file = openFile(filename)) == NULL) {
      return NG;
    }

    /* データファイルのページ数を調べる */
    numPage = getNumPages(filename);

    /* レコードを挿入できる場所を探す */
    for (i = 0; i < numPage; i++) {
        /* 1ページ分のデータを読み込む */
        if (readPage(file, i, page) != OK) {
          free(record);
          return NG;
        }

      /* pageの先頭からrecordSizeバイトずつ飛びながら、先頭のフラグが「0」(未使用)の場所を探す */
        for (j = 0; j < (PAGE_SIZE / recordSize); j++) {
          char *q;
          q = page + j*recordSize;
          if (*q == 0) {
            /* 見つけた空き領域に上で用意したバイト列recordを埋め込む */
            memcpy(q, record, recordSize);

            /* ファイルに書き戻す */
            if (writePage(file, i, page) != OK) {
              free(record);
              return NG;
            }
            closeFile(file);
            free(record);
            return OK;
          }
        }
    }

    assert(record != NULL);

    /*
     * ファイルの最後まで探しても未使用の場所が見つからなかったら
     * ファイルの最後に新しく空のページを用意し、そこに書き込む
     */

    char page2[PAGE_SIZE]={0};
    memcpy(page2, record, recordSize);
    if (writePage(file, i, page2) != OK) {
      free(record);
      return NG;
    }

    free(record);
    return OK;
}


/* ------ ■■これ以降の関数は、次回以降の実験で説明の予定 ■■ ----- */

/*
 * checkCondition -- レコードが条件を満足するかどうかのチェック
 *
 * 引数:
 *  recordData: チェックするレコード
 *  condition: チェックする条件
 *
 * 返り値:
 *  レコードrecordが条件conditionを満足すればOK、満足しなければNGを返す
 */
static Result checkCondition(RecordData *recordData, Condition *condition)
{ 
  int i;

  for (i = 0; i < recordData->numField; i++){
    if (strcmp(recordData->fieldData[i].name,condition->name)==0){
      if (condition->dataType==TYPE_INTEGER){
        if (condition->operator==OPR_EQUAL){
          if (recordData->fieldData[i].valueSet.intValue == condition->valueSet.intValue){
            return OK;     
          }
        }else if (condition->operator==OPR_NOT_EQUAL){
          if (recordData->fieldData[i].valueSet.intValue != condition->valueSet.intValue){
            return OK;
          }
        }else if (condition->operator==OPR_GREATER_THAN){
          if (recordData->fieldData[i].valueSet.intValue > condition->valueSet.intValue){
            return OK;
          }
        }else if (condition->operator==OPR_LESS_THAN){
          if (recordData->fieldData[i].valueSet.intValue < condition->valueSet.intValue){
            return OK;
          }
        }  
      }else if (condition->dataType==TYPE_STRING){
        if (condition->operator==OPR_EQUAL){
          if (strcmp(recordData->fieldData[i].valueSet.stringValue,condition->valueSet.stringValue)==0){
            return OK;     
          }
        }else if (condition->operator==OPR_NOT_EQUAL){
          if (strcmp(recordData->fieldData[i].valueSet.stringValue,condition->valueSet.stringValue)!=0){
            return OK;
          }
        }else if (condition->operator==OPR_GREATER_THAN){
          if (strcmp(recordData->fieldData[i].valueSet.stringValue,condition->valueSet.stringValue)>0){
            return OK;
          }
        }else if (condition->operator==OPR_LESS_THAN){
          if (strcmp(recordData->fieldData[i].valueSet.stringValue,condition->valueSet.stringValue)<0){
            return OK;
          }
        }  
      }      
    }  
  }

  return NG;

}

/*
 * selectRecord -- レコードの検索
 *
 * 引数:
 *  tableName: レコードを検索するテーブルの名前
 *  condition: 検索するレコードの条件
 *
 * 返り値:
 *  検索に成功したら検索されたレコード(の集合)へのポインタを返し、
 *  検索に失敗したらNULLを返す。
 *  検索した結果、該当するレコードが1つもなかった場合も、レコードの
 *  集合へのポインタを返す。
 *
 * ***注意***
 *  この関数が返すレコードの集合を収めたメモリ領域は、不要になったら
 *  必ずfreeRecordSetで解放すること。
 */
RecordSet *selectRecord(char *tableName, Condition *condition)
{
  int i,j,k,l=0,len,recordSize;
  char *filename;
  File *file;
  RecordData *p;
  char page[PAGE_SIZE]={0};
  RecordSet *recordSet;
  TableInfo *tableInfo;

  /*レコードセットを用意する*/
  if ((recordSet = (RecordSet *)malloc(sizeof(RecordSet))) == NULL) {
      return NULL;
  }
  recordSet->recordData=NULL;

  /* テーブルの定義情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
      /* テーブル情報の取得に失敗したので、処理をやめて返る */
      return NULL;
    }


  /* [tableName].defという文字列を作る */
    len = strlen(tableName) + strlen(DATA_FILE_EXT) + 1;
    if ((filename = malloc(len)) == NULL) {
      return NULL;
    }
    snprintf(filename, len, "%s%s", tableName, DATA_FILE_EXT);

    /* データファイルをオープンする */
    if ((file = openFile(filename)) == NULL) {
      return NULL;
    }


  /*レコードの大きさを得る*/
  recordSize = getRecordSize(tableInfo); 

  /*ページを一つずつ読み込む*/
  for (i = 0; i < getNumPages(filename); i++){
    char *q;
    readPage(file,i,page);
    
    /*レコードサイズごとに調べる*/
    for (j = 0; j < PAGE_SIZE/recordSize; j++){
      q = page + j * recordSize;
      
      /*使用中か調べる*/
      if (*q==0){
        continue;
      }
        
      q += 1;
      RecordData *record;
        
      /*データを収める領域を確保する*/
      if ((record = (RecordData *) malloc(sizeof(RecordData))) == NULL) {
        /* エラー処理 */
        return NULL;
      }

      /*レコードの内容を領域に写す*/
      record->numField = tableInfo->numField;
      for (k = 0; k < tableInfo->numField; k++) {
        strcpy(record->fieldData[k].name,tableInfo->fieldInfo[k].name);
        switch (tableInfo->fieldInfo[k].dataType) {
          case TYPE_INTEGER:
            record->fieldData[k].dataType = TYPE_INTEGER;
            memcpy(&record->fieldData[k].valueSet, q, sizeof(int));
            q += sizeof(int);
            break;
          case TYPE_STRING:
            record->fieldData[k].dataType = TYPE_STRING;
            memcpy(&record->fieldData[k].valueSet, q, MAX_STRING);
            q += MAX_STRING;
            break;
          default:
            /* ここにくることはないはず */
            freeTableInfo(tableInfo);
            free(record);
            return NULL;
        }
      }
        
      
      /*写されたデータが条件に一致したら、レコードの集合に追加する*/
      if (checkCondition(record,condition)==OK){
        if (recordSet->recordData==NULL){
          recordSet->recordData = record;
          record->next = NULL;
        }else{
          p = recordSet->recordData;
          recordSet->recordData = record;
          record->next = p;  
        }
        l++;
        recordSet->numRecord= l ;
      }else{
         free(record);
      }   
    }    
  }

  /*ファイルを閉じてレコードの集合を返す*/
  closeFile(file);
  return recordSet;

}



/*
 * freeRecordSet -- レコード集合の情報を収めたメモリ領域の解放
 *
 * 引数:
 *  recordSet: 解放するメモリ領域
 *
 * 返り値:
 *  なし
 *
 * ***注意***
 *  関数selectRecordが返すレコードの集合を収めたメモリ領域は、
 *  不要になったら必ずこの関数で解放すること。
 */
void freeRecordSet(RecordSet *recordSet)
{
  //char *p,*r;
  RecordData *p;
  //r = recordSet->recordData;
  /*レコードがある限り読み込む*/
  while(recordSet->recordData !=NULL){
    p = recordSet->recordData->next;
    free(recordSet->recordData);
    recordSet->recordData = p;
  }

  free(recordSet);

}

/*
 * deleteRecord -- レコードの削除
 *
 * 引数:
 *  tableName: レコードを削除するテーブルの名前
 *  condition: 削除するレコードの条件
 *
 * 返り値:
 *  削除に成功したらOK、失敗したらNGを返す
 */
Result deleteRecord(char *tableName, Condition *condition)
{
  int i,j,k,flag,len,recordSize;
  char *filename;
  File *file;
  char *p;
  char page[PAGE_SIZE]={0};
  //char *record;
  TableInfo *tableInfo;
  RecordData *record;

  /* テーブルの定義情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
      /* テーブル情報の取得に失敗したので、処理をやめて返る */
      return NG;
    }

  /* [tableName].defという文字列を作る */
    len = strlen(tableName) + strlen(DATA_FILE_EXT) + 1;
    if ((filename = malloc(len)) == NULL) {
      return NG;
    }
    snprintf(filename, len, "%s%s", tableName, DATA_FILE_EXT);

    /* データファイルをオープンする */
    if ((file = openFile(filename)) == NULL) {
      return NG;
    }

  /*レコードの大きさを得る*/
  recordSize = getRecordSize(tableInfo); 
        
    /*データを収める領域を確保する*/
  if ((record = (RecordData *) malloc(sizeof(RecordData))) == NULL) {
    /* エラー処理 */
    return NG;
  }

  /*ページを一つずつ読み込む*/
  for (i = 0; i < getNumPages(filename); i++){
    char *q;
    readPage(file,i,page);
    
    /*レコードサイズごとに調べる*/
    for (j = 0; j < PAGE_SIZE/recordSize; j++){
      q = page + j * recordSize;
      
      /*使用中か調べる*/
      if (*q==1){
        p = q;
        q += 1;
        /*レコードの内容を領域に写す*/
        record->numField = tableInfo->numField;
        for (k = 0; k < tableInfo->numField; k++) {
          strcpy(record->fieldData[k].name,tableInfo->fieldInfo[k].name);
          switch (tableInfo->fieldInfo[k].dataType) {
            case TYPE_INTEGER:
              record->fieldData[k].dataType = TYPE_INTEGER;
              memcpy(&record->fieldData[k].valueSet, q, sizeof(int));
              q += sizeof(int);
              break;
            case TYPE_STRING:
              record->fieldData[k].dataType =TYPE_STRING;
              memcpy(&record->fieldData[k].valueSet, q, MAX_STRING);
              q += MAX_STRING;
              break;
            default:
              /* ここにくることはないはず */
              freeTableInfo(tableInfo);
              free(record);
              return NG;
          }
        }
        
        /*写されたデータが条件に一致したら、フラグを0にする*/
        if (checkCondition(record,condition)==OK){
          *p = 0;
          flag = 1;
        }
      } 
    }

    /* ファイルに書き戻す */
    if (flag==1){
      if (writePage(file, i, page) != OK) {
        return NG;
      }  
    }
    free(record);
  }

  /*ファイルを閉じる*/
  closeFile(file);
  return OK;

}

/*
 * createDataFile -- データファイルの作成
 *
 * 引数:
 *  tableName: 作成するテーブルの名前
 *
 * 返り値:
 *  作成に成功したらOK、失敗したらNGを返す
 */
Result createDataFile(char *tableName)
{
  int len;
  char *filename;

  /* [tableName].dafという文字列を作る */
  len = strlen(tableName) + strlen(DATA_FILE_EXT) + 1;
  if ((filename = malloc(len)) == NULL) {
      return NG;
  }
  snprintf(filename, len, "%s%s", tableName, DATA_FILE_EXT);


  if (createFile(filename) != OK) {
    return NG;
  }

  return OK;

}

/*
 * deleteDataFile -- データファイルの削除
 *
 * 引数:
 *  tableName: 削除するテーブルの名前
 *
 * 返り値:
 *  削除に成功したらOK、失敗したらNGを返す
 */
Result deleteDataFile(char *tableName)
{
  int len;
  char *filename;

  /* [tableName].dafという文字列を作る */
  len = strlen(tableName) + strlen(DATA_FILE_EXT) + 1;
  if ((filename = malloc(len)) == NULL) {
      return NG;
  }
  snprintf(filename, len, "%s%s", tableName, DATA_FILE_EXT);


  if (deleteFile(filename) != OK) {
    return NG;
  }

  return OK;

}

/*
 * printTableData -- すべてのデータの表示(テスト用)
 *
 * 引数:
 *  tableName: データを表示するテーブルの名前
 */
void printTableData(char *tableName)
{
    TableInfo *tableInfo;
    File *file;
    int len;
    int i, j, k;
    int recordSize;
    int numPage;
    char *filename;
    char page[PAGE_SIZE];

    /* テーブルのデータ定義情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
  return;
    }

    /* 1レコード分のデータをファイルに収めるのに必要なバイト数を計算する */
    recordSize = getRecordSize(tableInfo);

    /* データファイルのファイル名を保存するメモリ領域の確保 */
    len = strlen(tableName) + strlen(DATA_FILE_EXT) + 1;
    if ((filename = malloc(len)) == NULL) {
  freeTableInfo(tableInfo);
  return;
    }

    /* ファイル名の作成 */
    snprintf(filename, len, "%s%s", tableName, DATA_FILE_EXT);

    /* データファイルのページ数を求める */
    numPage = getNumPages(filename);

    /* データファイルをオープンする */
    if ((file = openFile(filename)) == NULL) {
  free(filename);
  freeTableInfo(tableInfo);
  return;
    }

    free(filename);

    /* レコードを1つずつ取りだし、表示する */
    for (i = 0; i < numPage; i++) {
        /* 1ページ分のデータを読み込む */
        readPage(file, i, page);

        /* pageの先頭からrecord_sizeバイトずつ切り取って処理する */
        for (j = 0; j < (PAGE_SIZE / recordSize); j++) {
            /* 先頭の「使用中」のフラグが0だったら読み飛ばす */
      char *p = &page[recordSize * j];
      if (*p == 0) {
    continue;
      }

      /* フラグの分だけポインタを進める */
      p++;

            /* 1レコード分のデータを出力する */
      for (k = 0; k < tableInfo->numField; k++) {
    int intValue;
    char stringValue[MAX_STRING];

    printf("Field %s = ", tableInfo->fieldInfo[k].name);

    switch (tableInfo->fieldInfo[k].dataType) {
    case TYPE_INTEGER:
        memcpy(&intValue, p, sizeof(int));
        p += sizeof(int);
        printf("%d\n", intValue);
        break;
    case TYPE_STRING:
        memcpy(stringValue, p, MAX_STRING);
        p += MAX_STRING;
        printf("%s\n", stringValue);
        break;
    default:
        /* ここに来ることはないはず */
        return;
    }
      }

      printf("\n");
  }
    }
}

/*
 * printRecordSet -- レコード集合の表示
 *
 * 引数:
 *  recordSet: 表示するレコード集合
 */
void printRecordSet(RecordSet *recordSet)
{
    RecordData *record;
    int i, j, k;

    /* レコード数の表示 */
    printf("Number of Records: %d\n", recordSet->numRecord);

    /* レコードを1つずつ取りだし、表示する */
    for (record = recordSet->recordData; record != NULL; record = record->next) {
        /* すべてのフィールドのフィールド名とフィールド値を表示する */
      for (i = 0; i < record->numField; i++) {
        printf("Field %s = ", record->fieldData[i].name);

        switch (record->fieldData[i].dataType) {
        case TYPE_INTEGER:
          printf("%d\n", record->fieldData[i].valueSet.intValue);
          break;
        case TYPE_STRING:
          printf("%s\n", record->fieldData[i].valueSet.stringValue);
          break;
        default:
          /* ここに来ることはないはず */
          return;
        }
      }

      printf("\n");
    }
}
