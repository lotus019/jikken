/*
 * datamanip.c -- データ操作モジュール
 */

#include "microdb.h"

/*
 * DATA_FILE_EXT -- データファイルの拡張子
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

    for (フィールド数分だけループする) {
        /* i番目のフィールドがINT型かSTRING型か調べる */
        /* INT型ならtotalにsizeof(int)を加算 */
        /* STRING型ならtotalにMAX_STRINGを加算 */
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
    int numPage;
    char *record;
    char *p;

    /* テーブルの情報を取得する */
    if ((tableInfo = getTableInfo(tableName)) == NULL) {
        /* エラー処理 */
        return NG;
    }

    /* 1レコード分のデータをファイルに収めるのに必要なバイト数を計算する */
    recordSize = getRecordSize(tableInfo);

    /* 必要なバイト数分のメモリを確保する */
    if ((record = malloc(recordSize)) == NULL) {
        /* エラー処理 */
    }
    p = record;

    /* 先頭に、「使用中」を意味するフラグを立てる */
    memset(p, 1, 1);
    p += 1;

    /* 確保したメモリ領域に、フィールド数分だけ、順次データを埋め込む */
    for (i = 0; i < tableInfo->numField; i++) {
  switch (tableInfo->fieldInfo[i].dataType) {
  case TYPE_INTEGER:
      memcpy(p, ......);
      p += ...;
      break;
  case TYPE_STRING:
      memcpy(p, ......);
      p += ......;
      break;
  default:
      /* ここにくることはないはず */
            freeTableInfo(tableInfo);
            free(record);
      return NG;
  }
    }

    /* 使用済みのtableInfoデータのメモリを解放する */
    freeTableInfo(tableInfo);

    /*
     * ここまでで、挿入するレコードの情報を埋め込んだバイト列recordができあがる
     */

    /* データファイルをオープンする */
    ......

    /* データファイルのページ数を調べる */
    numPage = ......;

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
      q = ......;
      if (*q == 0) {
    /* 見つけた空き領域に上で用意したバイト列recordを埋め込む */
    memcpy(q, ......);

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

    /*
     * ファイルの最後まで探しても未使用の場所が見つからなかったら
     * ファイルの最後に新しく空のページを用意し、そこに書き込む
     */
    ......

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
}

/*
 * printRecordSet -- レコード集合の表示
 *
 * 引数:
 *  recordSet: 表示するレコード集合
 *
 * 返り値:
 *  なし
 */
void printRecordSet(RecordSet *recordSet)
{
}
