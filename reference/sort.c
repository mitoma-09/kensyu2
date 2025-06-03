#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>

/////////////////////////////////
// 関数宣言
/////////////////////////////////
int choice1(void);
int choice2(void);

int top_sort(int person, char *subject, char *text);
int top_sort_sum(int person, char *text);

int top_sort_day(int day, int person, char *subject, char *text);
int top_sort_day_sum(int day, int person, char *text);

double aver_subject(char *subject, char *text);
double aver_sum(char *text);

double aver_day_sum(int day, char *text);

int under_average(int day, char *subject, char *text);
int under_average_sum(int day, char *text);

int under_average_all(char *subject, char *text);
int under_average_sum_all(char *text);

int validate_date(const int date);

int execute_sql(const char *sql, int (*callback)(void *, int, char **, char **), void *callback_data);

/// @brief 構造体
typedef struct data{
    char name[40];
    //int day;
    int exam_day;
    int nLang;
    int math;
    int Eng;
    int JHist;
    int wHist;
    int geo;
    int phys;
    int chem;
    int bio;
    int ID;
} DATA;

/// @brief 平均点を返すコールバック関数
/// @param data
/// @param argc
/// @param argv
/// @param colName
/// @return 平均点
int callback_avg(void *data, int argc, char **argv, char **colName){
    if (argv[0]){
        *(double *)data = atof(argv[0]); // SQLの結果を取得し、double型に変換
    }else{
        *(double *)data = 0; // NULLの場合は0を返す
    }
    return 0;
}

/// @brief 表示用コールバック関数
/// @param NotUsed
/// @param argc
/// @param argv
/// @param colName
/// @return
int callback2(void *NotUsed, int argc, char **argv, char **colName){
    extern int isFirstCall; // 初回かどうかを判定するフラグ

    if (isFirstCall){ // カラム名を表示
        printf("%-25s %-10s %-6s \n", colName[0], colName[1], colName[2]);
        printf("--------------------------------------------\n"); // 区切り線
        isFirstCall = 0;                                          // フラグを更新
    }

    // printf("%-20s %-5s %-10s\n", argv[0], argv[1], argv[2]); // データを表示
    //  データがNULLの場合の処理を追加
    printf("%-25s  %-10s   %-6s \n", argv[0] ? argv[0] : "-",
           argv[1] ? argv[1] : "-",
           argv[2] ? argv[2] : "-");

    /*for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");*/
    // isFirstCall = 1; // フラグをリセット
    return 0;
}

////////////////////////////
// グローバル変数
////////////////////////////
#define MAX_SQL_SIZE 1000
// #define DEBUG

int isFirstCall; // Callbackで初回かどうかを判定するフラグ

#define TOTAL_SCORE "COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + \
                     COALESCE(JHist, 0)+ COALESCE(wHist, 0) + COALESCE(geo, 0) + \
                     COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)"

sqlite3_stmt *stmt_select; // 検索 用　
sqlite3_stmt *stmt_insert; // 挿入 用
sqlite3_stmt *stmt_update; // 更新 用
sqlite3_stmt *stmt_disp;   // 表示 用　

long rowid; // ROWID

sqlite3 *db;      // データベースハンドラ
char *db_name;    // データベースファイル名
char *table_name; // テーブル名

char *table; // テーブル列挙

char text[MAX_SQL_SIZE]; // SQL文用

char *subjects[] = {"nLang", "math", "Eng", "JHist", "wHist", "geo", "phys",
                    "chem", "bio"};

#define NUM_SUBJECT 9

///////////////////////////
// main関数
///////////////////////////
int main(int argc, char *argv[]){
    isFirstCall = 1;

    db_name = "test.sqlite3";
    // table_name = "pointTools";
    table_name = "pT";

    char *error_message; // エラーメッセージ用
    char sql_statement[2000];

    table = "name TEXT, day INTEGER, nLang INTEGER, math INTEGER, "
            "Eng INTEGER, JHist INTEGER, wHist INTEGER, geo INTEGER , "
            "phys INTEGER, chem INTEGER, bio INTEGER, ID INTEGER,"
            "PRIMARY KEY(name, day, ID) ";

    // strcpy(text, "SELECT * FROM pointTools");
    // text = "SELECT * FROM pointTools"; // ベースのSQL

    int rc; // 結果
    int i;
    int b, c, d; // scanf用変数 b:オプション

    // データベースファイルをオープン
    rc = sqlite3_open(db_name, &db);
    fprintf(stdout, "Open/Create status:[%s] [%s]\n", db_name, sqlite3_errmsg(db));
    if (rc){
        sqlite3_close(db);
        printf("データベースを開けません。");
        return 1;
    }

    // テーブル作成
    sprintf(sql_statement, "CREATE TABLE IF NOT EXISTS %s(%s)", table_name, table);

    // SQLステートメントを実行
    rc = sqlite3_exec(db, sql_statement, NULL, NULL, &error_message);

    if (rc == SQLITE_OK){
        fprintf(stdout, "Exec(Ceate Table):%s\n", sqlite3_errmsg(db));
    }else{
        fprintf(stdout, "ERROR(Ceate Table):%s\n", error_message);
        sqlite3_free(error_message);
    }
    // SQLITE stmt 用の設定(仮)
    // char *sql_statement_select ="SELECT * FROM pointTools "
    // "WHERE name = ?2 AND day = ?3";
    // char *sql_statement_insert = "INSERT INTO pointTools "
    //     "  VALUES( ?1 , ?2 , ?3 , ?4 , ?5 , ?6 , ?7 , ?8 , ?9 ,?10 , ?11 , ?12 )";
    // char *sql_statement_update = "UPDATE pointTools "
    //     "               SET at_bat =?5 , hit =?6  WHERE ROWID = :rowid";
    // char *sql_statement_disp   = "SELECT * FROM pointTools ORDER BY day";

    // sqlite3_prepare_v2(db, sql_statement_select, -1, &stmt_select, NULL);
    // sqlite3_prepare_v2(db, sql_statement_insert, -1, &stmt_insert, NULL);
    // sqlite3_prepare_v2(db, sql_statement_update, -1, &stmt_update, NULL);
    // sqlite3_prepare_v2(db, sql_statement_disp, -1, &stmt_disp, NULL);

    printf(" \n");

    // 機能選択
    c = choice1();

    // データベースを閉じる
    rc = sqlite3_close(db);

    if (c == 0){
        printf("参照機能を終了します\n");
    }else{
        // printf("もう一度やり直してください\n");
    }

    return 0;
}

///////////////////////////
// 選択関数
///////////////////////////
int choice1(void){

    int b, c, d, e;
    int day;
    double average;

    printf(" 1)試験実施日毎の各科目トップ5\n");
    printf(" 2)試験実施日毎の全科目トップ5\n");
    printf(" 3)試験実施日毎の全科目平均点数\n");
    printf(" 4)試験実施日毎の各科目平均点数以下の受験者一覧\n");
    printf(" 5)試験実施日毎の全科目平均点数以下の受験者一覧\n");
    printf(" 6)全試験における各科目合計トップ10\n");
    printf(" 7)全試験における全科目合計トップ10\n");
    printf(" 8)全試験における各科目平均点数\n");
    printf(" 9)全試験における全科目平均点数\n");
    printf(" 0)その他の機能\n");
    printf("利用したい機能を半角数字で入力してください:");
    scanf("%d", &b);

    switch (b){
    case 1:
        printf("試験実施日毎の各科目トップ5を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        while(getchar() != '\n') ;

        if (!validate_date(day)){ // 正常な日付の場合は処理を続行
            return 1;
        }
        printf("\n");

        printf("選択した試験日 %d のデータを取得します...\n", day);

        for (int i = 0; i < NUM_SUBJECT; i++){   // subjectの回数ループ
            isFirstCall = 1;                         // ヘッダーの表示リセット
            top_sort_day(day, 5, subjects[i], text); // 別のクラスで処理

            printf("\n");
            if (i != NUM_SUBJECT){
                printf("%sは以上です。\n",subjects[i]);

                while(getchar() != '\n') ; //エンターキー待機
            }
        }

        break;

    case 2:
        printf("試験実施日毎の全科目トップ5を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);

        printf("\n");

        if (!validate_date(day)){
            return 1;
        }

        isFirstCall = 1;
        top_sort_day_sum(day, 5, text); // 別のクラスで処理

        printf("\n");

        break;

    case 3:
        printf("試験実施日毎の全科目平均点数を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        printf("\n");

        if (!validate_date(day)){
            return 1;
        }

        isFirstCall = 1;
        printf("%dの日付について全科目の平均点を表示します。\n", day);
        average = aver_day_sum(day, text); // 別のクラスで計算
        printf("全科目の平均点は%.1fです\n", average);

        printf("\n");

        break;
    case 4:
        printf("試験実施日毎の各科目平均点数以下の受験者一覧を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        while(getchar() != '\n') ;

        printf("\n");

        if (!validate_date(day)){
            return 1;
        }

        for (int i = 0; i < NUM_SUBJECT; i++)        {                                          // subjectの回数ループ
            isFirstCall = 1;                       // ヘッダーのリセット
            under_average(day, subjects[i], text); // 別のクラスで処理

            if (i != NUM_SUBJECT - 1)            {
                //printf("%sは以上です。\n",subjects[i]);

                while(getchar() != '\n') ; //エンターキー待機
            }
            //printf("\n");
        }

        break;
    case 5:
        printf("試験実施日毎の全科目平均点数以下の受験者一覧を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        printf("\n");

        if (!validate_date(day)){
            return 1;
        }

        isFirstCall = 1; // ヘッダーのリセット
        under_average_sum(day, text);
        printf("\n");

        break;
    case 6:
        printf("全試験における各科目合計トップ10を表示します\n");
        while(getchar() != '\n') ;
        for (int i = 0; i < NUM_SUBJECT; i++)        {                                           // subjectの回数ループ
            isFirstCall = 1;                        // ヘッダーの表示リセット
            top_sort(10, subjects[i], text);        // 別のクラスで処理

            while(getchar() != '\n') ;
        }

        break;
    case 7:
        printf("全試験における全科目合計トップ10を表示します\n");
        printf("\n");
        isFirstCall = 1;
        top_sort_sum(10, text); // 別のクラスで処理

        printf("\n");

        break;
    case 8:
        printf("全試験における各科目平均点数を表示します\n");
        while(getchar() != '\n') ;
        for (int i = 0; i < NUM_SUBJECT; i++)        {                                           // subjectの回数ループ
            isFirstCall = 1;                        // ヘッダーの表示リセット
            aver_subject(subjects[i], text);

            while(getchar() != '\n') ;
        }

        break;
    case 9:
        printf("全試験における全科目平均点数を表示します\n");
        average = aver_sum(text); // 別のクラスで計算
        printf("全科目の平均点は%.1fです\n", average);

        printf("\n");

        break;
    case 0:
        /*printf(" 0)その他の機能\n");
        printf("利用したい機能を半角数字で入力してください:");*/

        return choice2();
        break;

    default:
        printf("\n");
        printf("その文字は利用できません\n");
        printf("最初からやり直してください\n");

        return 1;

        break;
    }

    return 0;
}

int choice2(void){

    int b;
    double average;

    printf(" \n");

    // printf(" 1)全試験における全科目平均点数\n");
    printf(" 1)全試験における各科目平均点数以下の受験者一覧\n");
    printf(" 2)全試験における全科目平均点数以下の受験者一覧\n");
    printf(" 0)その他の機能\n");
    printf("利用したい機能を半角数字で入力してください:");
    scanf("%d", &b);

    switch (b){
    case 1:
        printf("全試験における各科目平均点数以下の受験者一覧を表示します\n");
        while(getchar() != '\n') ;

        for (int i = 0; i < NUM_SUBJECT; i++)        {                                         // subjectの回数ループ
            isFirstCall = 1;                      // ヘッダーのリセット
            under_average_all(subjects[i], text); // 別のクラスで処理

            while(getchar() != '\n') ;
        }
        break;
    case 2:
        printf("全試験における全科目平均点数以下の受験者一覧を表示します\n");
        isFirstCall = 1; // ヘッダーのリセット
        under_average_sum_all(text);
        printf("\n");

        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        printf("表示できるものがありません。\n");
        printf("最初からやり直してください\n");
        break;
    case 0:

        return choice1();

        break;

    default:

        printf("\n");
        printf("その文字は利用できません\n");
        printf("最初からやり直してください\n");
        return 1;

        break;
    }

    return 0;
}

///////////////////////////
// 参照機能
///////////////////////////

/// @brief 科目のトップN人を表示
/// @param person トップN人
/// @param subject 科目
/// @param text
/// @return
int top_sort(int person, char *subject, char *text){
    printf("%sのトップ%dを表示します。\n", subject, person);

    if (person > 0){
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT * FROM ( "
                 "SELECT name, day, %s, RANK() OVER (ORDER BY %s DESC) AS ranking "
                 "FROM %s WHERE %s IS NOT NULL "
                 ") AS ranked_data "
                 "WHERE ranking <= %d "
                 "ORDER BY ranking ASC;",
                 subject, subject, table_name, subject, person);
    }else{
        printf("エラー: person の値が不正です。\n");
    }

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 全科目総合得点ののトップN人を表示
/// @param person トップN人
/// @param text
/// @return
int top_sort_sum(int person, char *text){
    printf("全科目総合得点のトップ%dを表示します。\n", person);

    if (person > 0){
        snprintf(text, MAX_SQL_SIZE,
                 //  "SELECT * FROM (  SELECT name, day, "
                 //  "   (COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
                 //  "    COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
                 //  "    COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) AS total_score,  "
                 //  "RANK() OVER (ORDER BY "
                 //  "    (COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
                 //  "     COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
                 //  "     COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) DESC) AS ranking  "
                 //  "FROM %s  "
                 //  ") AS ranked_data  "
                 //  "WHERE ranking <= %d  "
                 //  "ORDER BY ranking ASC;",
                 //  table_name, person);
                 "SELECT * FROM (  SELECT name, day, "
                 "   ( %s ) AS total_score,  "
                 "RANK() OVER (ORDER BY "
                 "    ( %s) DESC) AS ranking  "
                 "FROM %s  "
                 ") AS ranked_data  "
                 "WHERE ranking <= %d  "
                 "ORDER BY ranking ASC;",
                 TOTAL_SCORE,TOTAL_SCORE,table_name, person);
    }else{
        printf("エラー: person の値が不正です。\n");
    }

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 日程ごとの科目トップN人を表示
/// @param day 日程指定
/// @param person トップN人
/// @param subject 科目
/// @param text
/// @return
int top_sort_day(int day, int person, char *subject, char *text){
    printf("%sのトップ%dを表示します。\n", subject, person);

    if (person > 0){
        snprintf(text, MAX_SQL_SIZE,
                 //"SELECT * FROM %s WHERE %s IS NOT NULL AND %s >= (SELECT %s FROM %s "
                 //"ORDER BY %s DESC LIMIT 1 OFFSET %d) ORDER BY %s DESC;"
                 //,table_name,subject,subject,subject,table_name,subject, person-1,subject);

                 "SELECT * FROM ( "
                 "SELECT name, day, %s, RANK() OVER (ORDER BY %s DESC) AS ranking "
                 "FROM %s WHERE %s IS NOT NULL AND day = %d "
                 ") AS ranked_data "
                 "WHERE ranking <= %d "
                 "ORDER BY ranking ASC;",
                 subject, subject, table_name, subject, day, person);
    }else{
        printf("エラー: person の値が不正です。\n");
    }

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 日程ごとの全科目総合点トップN人を表示
/// @param day 日程指定
/// @param person トップN人
/// @param text
/// @return
int top_sort_day_sum(int day, int person, char *text){
    printf("%dの日付についてソートを開始します。\n", day);
    printf("全科目総合得点のトップ%dを表示します。\n", person);

    if (person > 0){
        snprintf(text, MAX_SQL_SIZE,

                 // " SELECT name, day, "
                 // " (COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) +"
                 // " COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
                 // " COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) AS total_score, "
                 // " RANK() OVER (ORDER BY "
                 // "(COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
                 // " COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
                 // " COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) DESC) AS ranking  "
                 // " FROM %s"
                 // " ORDER BY ranking ASC"
                 // " WHERE ranking<= %d;"
                 // ,table_name,person);

                 "SELECT * FROM (  SELECT name, day, "
                 "   ( %s ) AS total_score,  "
                 "RANK() OVER (ORDER BY "
                 "    ( %s ) DESC) AS ranking  "
                 "FROM %s  WHERE day = %d "
                 ") AS ranked_data  "
                 "WHERE ranking <= %d  "
                 "ORDER BY ranking ASC;",
                 TOTAL_SCORE, TOTAL_SCORE,table_name, day, person);
    }else{
        printf("エラー: person の値が不正です。\n");
    }

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 科目平均点検索・表示
/// @param subject 科目
/// @param text
/// @return 平均点(少数第二位四捨五入)
double aver_subject(char *subject, char *text){
    double average = 0;

    snprintf(text, MAX_SQL_SIZE,
             "SELECT AVG( %s ) FROM %s WHERE %s IS NOT NULL;",
             subject, table_name, subject);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback_avg, &average);

    printf("%sの全日通した平均点は %.1f 点です。\n", subject, average);
    return average;
}

/// @brief 全体平均点検索
/// @param text
/// @return 全体平均点(少数第二位四捨五入)
double aver_sum(char *text){
    double average = 0;

    snprintf(text, MAX_SQL_SIZE,
             "SELECT SUM(individual_avg) / COUNT(DISTINCT name) AS overall_avg"
             " FROM ("
             " SELECT name, "
             " CAST(( %s ) AS REAL) / "
             " NULLIF( (nLang IS NOT NULL) + (math IS NOT NULL) + (Eng IS NOT NULL) +"
             " (JHist IS NOT NULL) + (wHist IS NOT NULL) + (geo IS NOT NULL) +"
             " (phys IS NOT NULL) + (chem IS NOT NULL) + (bio IS NOT NULL), 0) AS individual_avg"
             " FROM %s"
             " ) AS avg_per_person;",
             TOTAL_SCORE,table_name);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback_avg, &average);

    return average; // 全員の得点の平均値を戻り値
}

/// @brief 日程ごとの全体平均点検索
/// @param day 日程検索
/// @param text
/// @return 平均点(少数第二位四捨五入)
double aver_day_sum(int day, char *text){
    double average = 0;

    // printf("%dの日付について全科目の平均点を表示します。\n",day);

    // 受験者ごとの平均点を求めるSQL
    /*snprintf(text, MAX_SQL_SIZE,
    "SELECT name, day, "
    "   SUM(COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
    "       COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
    "       COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) / "
    "   (COUNT(nLang) + COUNT(math) + COUNT(Eng) + COUNT(JHist) + "
    "    COUNT(wHist) + COUNT(geo) + COUNT(phys) + COUNT(chem) + COUNT(bio)) "
    "   AS avg_score  "
    "FROM %s WHERE day = %d GROUP BY name, day;"
    ,table_name,day);*/

    snprintf(text, MAX_SQL_SIZE,
             "SELECT SUM(individual_avg) / COUNT(DISTINCT name) AS overall_avg"
             " FROM ("
             " SELECT name, "
             " CAST(( %s ) AS REAL) / "
             " NULLIF( (nLang IS NOT NULL) + (math IS NOT NULL) + (Eng IS NOT NULL) +"
             " (JHist IS NOT NULL) + (wHist IS NOT NULL) + (geo IS NOT NULL) +"
             " (phys IS NOT NULL) + (chem IS NOT NULL) + (bio IS NOT NULL), 0) AS individual_avg"
             " FROM %s"
             " WHERE day = %d"
             " ) AS avg_per_person;",
             TOTAL_SCORE,table_name, day);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback_avg, &average);

    return average; // 全員の得点の平均値を戻り値
}

/// @brief 日程ごとの科目平均点以下生徒検索
/// @param day 日程検索
/// @param subject 科目
/// @param text
/// @return
int under_average(int day, char *subject, char *text){
    double average = 0;

    snprintf(text, MAX_SQL_SIZE,
             "SELECT AVG( %s ) FROM %s WHERE day = %d AND %s IS NOT NULL;",
             subject, table_name, day, subject);

    // SQLを実行して平均点を取得
    int rc = execute_sql(text, callback_avg, &average);

    printf("%sの平均点は%.1f点です。\n", subject, average);
    printf("%sの得点が平均点以下の生徒を表示します。\n", subject);

    snprintf(text, MAX_SQL_SIZE,
             "WITH subject_avg AS ( "
             "    SELECT AVG( %s ) AS avg_score "
             "    FROM %s "
             "    WHERE day = %d  AND %s IS NOT NULL"
             " ) "
             " SELECT p.name, p.day, p.%s, sa.avg_score "
             " FROM %s p "
             " JOIN subject_avg sa "
             " WHERE p.day = %d AND p.%s <= sa.avg_score "
             " ORDER BY p.%s DESC;",
             subject, table_name, day, subject, subject, table_name, day, subject, subject);

    /*"SELECT * FROM ( "
    "SELECT name, day, %s, RANK() OVER (ORDER BY %s DESC) AS ranking "
    "FROM %s WHERE %s IS NOT NULL AND day = %d "
    ") AS ranked_data "
    "WHERE ranking <= %d "
    "ORDER BY ranking ASC;"
    //,subject,subject,table_name,subject,day,person);*/

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 日程ごとの平均点以下生徒検索
/// @param day 日程
/// @param text
/// @return
int under_average_sum(int day, char *text){
    double average = aver_day_sum(day, text);

    // printf("試験実施日毎の全科目平均点数以下の受験者一覧を表示します\n");

    printf("%dにおける全体の平均点は%.1f点です。\n", day, average);
    printf("全体の得点が平均点以下の生徒を表示します。\n");

    snprintf(text, MAX_SQL_SIZE,

             // " SELECT name, day, "
             // " (COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) +"
             // " COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
             // " COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) AS total_score, "
             // " RANK() OVER (ORDER BY "
             // "(COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
             // " COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
             // " COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) DESC) AS ranking  "
             // " FROM %s"
             // " ORDER BY ranking ASC"
             // " WHERE ranking<= %d;"
             // ,table_name,person);

             "SELECT * FROM (  SELECT name, day, "
             "  CAST( SUM( %s )  AS REAL)/ "
             "  NULLIF( (COUNT(nLang) + COUNT(math) + COUNT(Eng) + COUNT(JHist) + "
             "    COUNT(wHist) + COUNT(geo) + COUNT(phys) + COUNT(chem) + COUNT(bio)) ,0)"
             "   AS avg_score  "
             " FROM %s  WHERE day = %d GROUP BY name, day "
             " )   "
             " WHERE avg_score <= %f  "
             " ORDER BY avg_score DESC; ",
             TOTAL_SCORE,table_name, day, average);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 科目平均点以下生徒検索
/// @param subject　科目
/// @param text
/// @return
int under_average_all(char *subject, char *text){
    double average = 0;

    snprintf(text, MAX_SQL_SIZE,
             "SELECT AVG( %s ) FROM %s WHERE %s IS NOT NULL;",
             subject, table_name, subject);

    // SQLを実行して平均点を取得
    int rc = execute_sql(text, callback_avg, &average);

    printf("%sの平均点は%.1f点です。\n", subject, average);
    printf("%sの得点が平均点以下の生徒を表示します。\n", subject);

    snprintf(text, MAX_SQL_SIZE,
             "WITH subject_avg AS ( "
             "    SELECT AVG( %s ) AS avg_score "
             "    FROM %s "
             "    WHERE  %s IS NOT NULL"
             " ) "
             " SELECT p.name, p.day, p.%s, sa.avg_score "
             " FROM %s p "
             " JOIN subject_avg sa "
             " WHERE  p.%s <= sa.avg_score "
             " ORDER BY p.%s DESC;",
             subject, table_name, subject, subject, table_name, subject, subject);

    /*"SELECT * FROM ( "
    "SELECT name, day, %s, RANK() OVER (ORDER BY %s DESC) AS ranking "
    "FROM %s WHERE %s IS NOT NULL AND day = %d "
    ") AS ranked_data "
    "WHERE ranking <= %d "
    "ORDER BY ranking ASC;"
    //,subject,subject,table_name,subject,day,person);*/

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 全体平均点以下生徒検索
/// @param text
/// @return
int under_average_sum_all(char *text){

    double average = aver_sum(text);

    printf("全体の平均点は%.1f点です。\n", average);
    printf("全体の得点が平均点以下の生徒を表示します。\n");

    snprintf(text, MAX_SQL_SIZE,

             // " SELECT name, day, "
             // " (COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) +"
             // " COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
             // " COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) AS total_score, "
             // " RANK() OVER (ORDER BY "
             // "(COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
             // " COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
             // " COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) DESC) AS ranking  "
             // " FROM %s"
             // " ORDER BY ranking ASC"
             // " WHERE ranking<= %d;"
             // ,table_name,person);

             "SELECT * FROM (  SELECT name, day, "
             "  CAST( SUM( %s )  AS REAL)/ "
             "  NULLIF( (COUNT(nLang) + COUNT(math) + COUNT(Eng) + COUNT(JHist) + "
             "    COUNT(wHist) + COUNT(geo) + COUNT(phys) + COUNT(chem) + COUNT(bio)) ,0)"
             "   AS avg_score  "
             " FROM %s   GROUP BY name, day "
             " )   "
             " WHERE avg_score <= %f  "
             " ORDER BY avg_score DESC; ",
             TOTAL_SCORE, table_name, average);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);
    /*int rc; // result codes
    char *error_message;

    rc = sqlite3_exec(db, text, callback2, 0, &error_message);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQLエラー: %s\n", error_message);
        sqlite3_free(error_message);
    }*/

    return 0;
}

///////////////////////////
// 日付入力チェック
///////////////////////////
int validate_date(int date){
    if (date >= 100000000 || date < 10000000){
        printf("エラー: 日付は8桁の数字で入力してください（例: 20250513）。\n");
        // printf("%d\n",date);
        return 0;
    }

    int year, day, month;
    year = date / 10000;
    month = (date % 10000) / 100;
    day = date % 100;

    // year = atoi(date);
    // month = atoi(date + 4);
    // day = atoi(date + 6);

    // printf("%d年%d月%d日\n",year,month,day);

    if (month < 1 || month > 12){
        printf("エラー: 存在しない月です。\n");
        return 0;
    }

    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0){
        days_in_month[1] = 29;
    }

    if (day < 1 || day > days_in_month[month - 1]){
        printf("エラー: 存在しない日です。\n");
        return 0;
    }

    return 1;
}

/// @brief SQL実行とエラー処理
/// @param sql
/// @param callback
/// @param callback_data
/// @return
int execute_sql(const char *sql, int (*callback)(void *, int, char **, char **), void *callback_data){
    int rc;
    char *error_message;
    rc = sqlite3_exec(db, sql, callback, callback_data, &error_message);
    if (rc != SQLITE_OK){
        fprintf(stderr, "SQLエラー: %s\n", error_message);
        sqlite3_free(error_message);
    }
    return rc;
}

//#define TOTAL_SCORE "COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + COALESCE(JHist, 0)
//+ COALESCE(wHist, 0) + COALESCE(geo, 0) + COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)"


