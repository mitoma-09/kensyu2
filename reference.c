#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <sqlite3.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "database.h"
#include "touroku.h"
#include <wchar.h>
#include <locale.h>


/////////////////////////////////
// 関数宣言
/////////////////////////////////
int disp_choice1(void);
int disp_choice2(void);

int disp_ID(char *text);

int top_sort(int person, char *subject, char *text);
int top_sort_sum(int person, char *text);

int top_sort_day(int day, int person, char *subject, char *text);
int top_sort_day_sum(int day, int person, char *text);

double calc_subject_average(const char *subject, int day, char *text);
double calc_average(int day,char *text);

//double aver_sum(char *text);
//double aver_day_sum(int day, char *text);

int under_average(int day, char *subject, char *text);
int under_average_sum(int day, char *text);

int under_average_all(char *subject, char *text);
int under_average_sum_all(char *text);

int validate_date(const int date);

int execute_sql(const char *sql, int (*callback)(void *, int, char **, char **), void *callback_data);

void display_deviation_scores( const char *subject, int day, char *text);
void display_total_deviation_scores(char *text);


    double
    calc_subject_std(const char *subject, int day, char *text);

/// @brief データの構造体
typedef struct data{
    char name[40];
    // int day;
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

/// @brief 関数の構造体
typedef struct {
    int isFirstCall;

    sqlite3 *db;
    char *db_name;
    char *table_name;

} AppContext;

///@brief 偏差値計算用のコンテキスト構造体
typedef struct{
    double avg;
    double std;
} DeviationContext;

/// @brief 合計得点に対する平均と標準偏差を計算するためのコールバック用コンテキスト
typedef struct{
    double avg;
    double std;
} TotalStats;

//構造体の初回呼び出しリセット
#define RESET_FIRST_CALL(ctx) ((ctx)->isFirstCall = 1)

/// @brief 指定されたフィールド幅（表示桁数）に合わせて文字列を出力する関数
/// @param str 
/// @param field_width 
/// @return 
int print_with_padding(const char *str, int field_width){
    // NULLの場合はハイフンを出力
    if (str == NULL){
        str = "-";
    }

    // UTF-8文字列をワイド文字列に変換
    wchar_t wstr[256] = {0};
    mbstowcs(wstr, str, sizeof(wstr) / sizeof(wchar_t));

    // 実際の表示幅（全角文字は2などでカウントされる）
    int disp_width = wcswidth(wstr, wcslen(wstr));
    if (disp_width < 0)
        disp_width = 0;

    // 文字列を出力
    printf("%s", str);

    // 足りない幅分スペースを出力する
    int pad = field_width - disp_width;
    for (int i = 0; i < pad; i++){
        putchar(' ');
    }
    return 0;
}

// 各フィールドの表示幅に合わせて正しくパディングする関数
int print_field(const char *str, int field_width){
    // NULLの場合はハイフンを使用
    if (str == NULL){
        str = "-";
    }

    // UTF-8文字列をワイド文字列に変換（wchar_tは各文字の「表示幅」を正しく扱える）
    wchar_t wstr[256] = {0};
    size_t len = mbstowcs(wstr, str, sizeof(wstr) / sizeof(wchar_t));
    if (len == (size_t)-1){
        // 変換に失敗したら、ただ文字列をそのまま出力し、バイト数でパディング
        printf("%s", str);
        int pad = field_width - strlen(str);
        for (int i = 0; i < (pad > 0 ? pad : 0); i++){
            putchar(' ');
        }
        return 0;
    }

    // wcswidthで実際の表示幅を測定（全角文字は2桁としてカウントされる）
    int disp_width = wcswidth(wstr, len);
    if (disp_width < 0)
        disp_width = 0;

    // 元の文字列を出力
    printf("%s", str);

    // フィールド幅との差分だけスペースを追加してパディング
    int pad = field_width - disp_width;
    for (int i = 0; i < (pad > 0 ? pad : 0); i++){
        putchar(' ');
    }
    return 0;
}

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
int callback(void *NotUsed, int argc, char **argv, char **colName){
    extern int isFirstCall; // 初回かどうかを判定するフラグ

    // ロケールを設定（多言語対応のため）
    setlocale(LC_ALL, "");

    // 各カラムの希望する表示幅
    int field_widths[3] = {6, 15, 6}; // IDは6桁、nameは15桁、exam_dayは6桁

    if (isFirstCall){
        // ヘッダーを出力
        for (int i = 0; i < argc; i++){
            // ヘッダー出力（余白はパディングで調整）
            print_field(colName[i], field_widths[i]);
            if (i < argc - 1)
                putchar(' '); // カラム間の区切り（必要に応じて変更）
        }
        printf("\n");

        // ヘッダーとデータの区切り線を出力（フィールド幅の合計）
        int total_width = 0;
        for (int i = 0; i < argc; i++){
            total_width += field_widths[i] + 1;
        }
        // 最後の余分なスペースを除く
        total_width--;
        for (int i = 0; i < total_width; i++){
            putchar('-');
        }
        printf("\n");
        //printf("--------------------------------------------\n"); // 区切り線
        isFirstCall = 0;
    }

    // 各行のデータを出力
    for (int i = 0; i < argc; i++){
        print_field(argv[i] ? argv[i] : "-", field_widths[i]);
        if (i < argc - 1)
            putchar(' ');
    }
    printf("\n");

    /*if (isFirstCall){ // カラム名を表示
        printf("%-6s %-15s %-6s \n", colName[0], colName[1], colName[2]);
        printf("--------------------------------------------\n"); // 区切り線
        isFirstCall = 0;                                          // フラグを更新
    }

    // printf("%-20s %-5s %-10s\n", argv[0], argv[1], argv[2]); // データを表示
    //  データがNULLの場合の処理を追加
    printf("%-6s  %-15s   %-6s \n",
           argv[0] ? argv[0] : "-",
           argv[1] ? argv[1] : "-",
           argv[2] ? argv[2] : "-");

    /*for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");*/
    // isFirstCall = 1; // フラグをリセット

    /*if (isFirstCall)
    {
        for (int i = 0; i < argc; i++)
            printf("%-15s ", colName[i]);
        printf("\n--------------------------------------------\n");
        isFirstCall = 0;
    }
    for (int i = 0; i < argc; i++)
        printf("%-15s ", (argv[i] ? argv[i] : "-"));
    printf("\n");*/

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

    setlocale(LC_ALL, "");

    int field_widths[3] = {25, 10, 6}; // name25桁、day10桁、その他6桁

    if (isFirstCall){
        for (int i = 0; i < argc; i++){
            print_field(colName[i], field_widths[i]);
            if (i < argc - 1)
                putchar(' ');
        }
        printf("\n");

        int total_width = 0;
        for (int i = 0; i < argc; i++){
            total_width += field_widths[i] + 1;
        }
        total_width--;
        for (int i = 0; i < total_width; i++){
            putchar('-');
        }
        //printf("\n");
        printf("--------------------------------------------\n"); // 区切り線
        isFirstCall = 0;
    }

    for (int i = 0; i < argc; i++){
        print_field(argv[i] ? argv[i] : "-", field_widths[i]);
        if (i < argc - 1)
            putchar(' ');
    }
    printf("\n");

    /*if (isFirstCall){ // カラム名を表示
        printf("%-25s %-10s %-6s \n", colName[0], colName[1], colName[2]);
        printf("--------------------------------------------\n"); // 区切り線
        isFirstCall = 0;                                          // フラグを更新
    }

    // printf("%-20s %-5s %-10s\n", argv[0], argv[1], argv[2]); // データを表示
    //  データがNULLの場合の処理を追加
    printf("%-25s  %-10s   %-6s \n", 
            argv[0] ? argv[0] : "-",
            argv[1] ? argv[1] : "-",
            argv[2] ? argv[2] : "-");

    /*for (int i = 0; i < argc; i++) {
        printf("%s = %s\n", colName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");*/
    // isFirstCall = 1; // フラグをリセット
    return 0;
}

///@brief SQLコールバック関数（各受験者の得点から偏差値を計算し表示）
int deviation_callback(void *data, int argc, char **argv, char **colNames){
    DeviationContext *ctx = (DeviationContext *)data;
    setlocale(LC_ALL, "");
    extern int isFirstCall; // 初回かどうかを判定するフラグ
    int field_widths[4] = {25, 10, 6, 10}; // name25桁、day10桁、その他6桁

    // 期待されるカラム: name, day, 得点
    if (isFirstCall){
        print_field("name", field_widths[0]);
        putchar(' ');
        print_field("exam_day", field_widths[1]);
        putchar(' ');
        print_field("score", field_widths[2]);
        putchar(' ');
        print_field("deviation", field_widths[3]);
        printf("\n");

        // ヘッダーとデータを区切るための線を出力
        int totalWidth = field_widths[0] + field_widths[1] + field_widths[2] + field_widths[3] + 3; // 各間に1つずつスペース
        for (int i = 0; i < totalWidth; i++){
            putchar('-');
        }
        printf("\n");
        isFirstCall = 0;
    }

    double score = 0.0;
    if (argv[2]){
        score = atof(argv[2]);
    }
    // 偏差値の計算
    double deviation = 50 + 10 * (score - ctx->avg) / ctx->std;
    /*printf("%-25s %-10s %-6s %-10.1f\n",
           argv[0] ? argv[0] : "-",
           argv[1] ? argv[1] : "-",
           argv[2] ? argv[2] : "-",
           deviation);*/

    print_field(argv[0] ? argv[0] : "-", 25);
    putchar(' ');
    print_field(argv[1] ? argv[1] : "-", 10);
    putchar(' ');
    print_field(argv[2] ? argv[2] : "-", 6);
    putchar(' ');

    // 数値型の偏差値は文字列に変換して出力
    char deviation_str[32];
    snprintf(deviation_str, sizeof(deviation_str), "%.1f", deviation);
    print_field(deviation_str, 10);

    printf("\n");

    return 0;
}

/// @brief 合計得点の平均・標準偏差を計算して TotalStats 構造体に格納するコールバック関数の例
int callback_total_stats(void *data, int argc, char **argv, char **azColName){
    TotalStats *stats = (TotalStats *)data;
    setlocale(LC_ALL, "");
    extern int isFirstCall; // 初回かどうかを判定するフラグ
    int field_widths[4] = {25, 10, 6, 10}; // name25桁、day10桁、その他6桁

    if (isFirstCall){
        print_field("name", field_widths[0]);
        putchar(' ');
        print_field("exam_day", field_widths[1]);
        putchar(' ');
        print_field("score", field_widths[2]);
        putchar(' ');
        print_field("deviation", field_widths[3]);
        printf("\n");

        // ヘッダーとデータを区切るための線を出力
        int totalWidth = field_widths[0] + field_widths[1] + field_widths[2] + field_widths[3] + 3; // 各間に1つずつスペース
        for (int i = 0; i < totalWidth; i++){
            putchar('-');
        }
        printf("\n");
        isFirstCall = 0;
    }

    if (argc >= 2){
        // SQL の SELECT で順に avg_total, std_total を取得している前提
        stats->avg = argv[0] ? atof(argv[0]) : 0;
        stats->std = argv[1] ? atof(argv[1]) : 0;
    }
    return 0;
}

/*/// @brief リセット関数
/// @param db 
void reset_db_connection(sqlite3 **db){
    // 既存の接続を閉じる
    if (*db != NULL){
        sqlite3_close(*db);
        *db = NULL;
    }

    // 新しい接続を確立
    if (sqlite3_open("examdata.db", db) != SQLITE_OK){
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(*db));
    }
}
*/
////////////////////////////
// グローバル変数・マクロ
////////////////////////////
#define MAX_SQL_SIZE 1000

// #define DEBUG

int isFirstCall; // Callbackで初回かどうかを判定するフラグ

#define TOTAL_SCORE "COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + \
                     COALESCE(JHist, 0)+ COALESCE(wHist, 0) + COALESCE(geo, 0) + \
                     COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)"

#define CLEAR_INPUT_BUFFER() while (getchar() != '\n') //エンターキー待機

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

#define NUM_SUBJECT 9 //教科数

///////////////////////////
// main関数(reference)
///////////////////////////
int reference(sqlite3 *database){
    db=database;
    isFirstCall = 1;

    //db_name = "test.sqlite3";
    db_name=DATABASE_FILENAME;
    // table_name = "pointTools";
    //table_name = "pT";
    table_name= DATABASE_TABLENAME;

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
    sprintf(sql_statement, "CREATE TABLE IF NOT EXISTS %s( %s )", table_name, table);

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
    c = disp_choice1();

    reset_db_connection(&db);

    // データベースを閉じる
    rc = sqlite3_close(db);

    if (c == 0){
        printf("参照機能を終了します\n");
        CLEAR_INPUT_BUFFER();
    }else{
        // printf("もう一度やり直してください\n");
    }

    return 0;
}

///////////////////////////
// 選択関数
///////////////////////////
int disp_choice1(void){

    int b, c, d, e;
    int day;
    double average;

    printf(" 1)IDに対応する試験者と試験実施日\n");
    printf(" 2)試験実施日毎の各科目トップ5\n");
    printf(" 3)試験実施日毎の全科目トップ5\n");
    printf(" 4)試験実施日毎の全科目平均点数\n");
    printf(" 5)試験実施日毎の各科目平均点数以下の受験者一覧\n");
    printf(" 6)試験実施日毎の全科目平均点数以下の受験者一覧\n");
    printf(" 7)全試験における各科目合計トップ10\n");
    printf(" 8)全試験における全科目合計トップ10\n");
    printf(" 9)全試験における各科目平均点数\n");
    printf(" 0)その他の機能\n");
    printf("利用したい機能を半角数字で入力してください:");
    scanf("%d", &b);

    switch (b){
    case 1:
        printf("IDに対応する試験者と試験実施日を表示します\n");
        CLEAR_INPUT_BUFFER();
        disp_ID(text);

        break;
    case 2:
        printf("試験実施日毎の各科目トップ5を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        CLEAR_INPUT_BUFFER();

        if (!validate_date(day)){ // 正常な日付の場合は処理を続行
            return 1;
        }
        printf("\n");

        printf("選択した試験日 %d のデータを取得します...\n", day);

        for (int i = 0; i < NUM_SUBJECT; i++){       // subjectの回数ループ
            isFirstCall = 1;                         // ヘッダーの表示リセット
            top_sort_day(day, 5, subjects[i], text); // 別のクラスで処理

            printf("\n");
                printf("%sは以上です。\n", subjects[i]);

                CLEAR_INPUT_BUFFER();// エンターキー待機
        }

        break;

    case 3:
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

    case 4:
        printf("試験実施日毎の全科目平均点数を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        printf("\n");

        if (!validate_date(day)){
            return 1;
        }

        isFirstCall = 1;
        printf("%dの日付について全科目の平均点を表示します。\n", day);
        average = calc_average(day, text); // 別のクラスで計算
        printf("全科目の平均点は%.1fです\n", average);

        printf("\n");

        break;
    case 5:
        printf("試験実施日毎の各科目平均点数以下の受験者一覧を表示します\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        CLEAR_INPUT_BUFFER();

        printf("\n");

        if (!validate_date(day)){
            return 1;
        }

        for (int i = 0; i < NUM_SUBJECT; i++){     // subjectの回数ループ
            isFirstCall = 1;                       // ヘッダーのリセット
            under_average(day, subjects[i], text); // 別のクラスで処理

            if (i != NUM_SUBJECT - 1){
                // printf("%sは以上です。\n",subjects[i]);

                CLEAR_INPUT_BUFFER(); // エンターキー待機
            }
            // printf("\n");
        }

        break;
    case 6:
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
    case 7:
        printf("全試験における各科目合計トップ10を表示します\n");
        CLEAR_INPUT_BUFFER();
        for (int i = 0; i < NUM_SUBJECT; i++){  // subjectの回数ループ
            isFirstCall = 1;                    // ヘッダーの表示リセット
            top_sort(10, subjects[i], text);    // 別のクラスで処理

            CLEAR_INPUT_BUFFER();
        }

        break;
    case 8:
        printf("全試験における全科目合計トップ10を表示します\n");
        printf("\n");
        isFirstCall = 1;
        top_sort_sum(10, text); // 別のクラスで処理

        printf("\n");

        break;
    case 9:
        printf("全試験における各科目平均点数を表示します\n");
        CLEAR_INPUT_BUFFER();
        for (int i = 0; i < NUM_SUBJECT; i++){ // subjectの回数ループ
            isFirstCall = 1;                   // ヘッダーの表示リセット
            average = calc_subject_average(subjects[i],0,text);
            printf("%sの平均点は%.1fです\n",subjects[i],average);

            CLEAR_INPUT_BUFFER();
        }

        break;
   
    case 0:
        /*printf(" 0)その他の機能\n");
        printf("利用したい機能を半角数字で入力してください:");*/

        return disp_choice2();
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

int disp_choice2(void){

    int b;
    double average;
    int day; //=20200202;

    printf(" \n");

    // printf(" 1)全試験における全科目平均点数\n");
    printf(" 1)全試験における全科目平均点数\n");
    printf(" 2)全試験における各科目平均点数以下の受験者一覧\n");
    printf(" 3)全試験における全科目平均点数以下の受験者一覧\n");
    printf(" 4)全試験における各科目偏差値一覧\n");
    printf(" 5)試験実施日毎の各科目偏差値一覧\n");
    printf(" 0)その他の機能\n");
    printf("利用したい機能を半角数字で入力してください:");
    scanf("%d", &b);

    switch (b){
    case 1:
        printf("全試験における全科目平均点数を表示します\n");
        average = calc_average(0,text); // 別のクラスで計算
        printf("全科目の平均点は%.1fです\n", average);

        printf("\n");

        break;
    case 2:
        printf("全試験における各科目平均点数以下の受験者一覧を表示します\n");
        CLEAR_INPUT_BUFFER();
        for (int i = 0; i < NUM_SUBJECT; i++){    // subjectの回数ループ
            isFirstCall = 1;                      // ヘッダーのリセット
            under_average_all(subjects[i], text); // 別のクラスで処理

            CLEAR_INPUT_BUFFER();
        }
        break;
    case 3:
        printf("全試験における全科目平均点数以下の受験者一覧を表示します\n");
        isFirstCall = 1; // ヘッダーのリセット
        under_average_sum_all(text);
        printf("\n");

        break;
    case 4:
    //char* subject=subjects[0];
    //    display_deviation_scores(subject,day,text);
        printf("隠し機能：偏差値(全日)\n");

        printf("偏差値を表示します。　対象は全ての試験です。\n");
        
        for (int i = 0; i < NUM_SUBJECT; i++){
            isFirstCall = 1; // ヘッダーのリセット
            display_deviation_scores(subjects[i], 0, text);
            CLEAR_INPUT_BUFFER();
        }
            break;

    case 5:
        printf("隠し機能：偏差値(特定日)\n");

        printf("試験実施日を半角数字8桁(例:20200202)で選択してください:");
        scanf("%d", &day);
        CLEAR_INPUT_BUFFER();
        
        for (int i = 0; i < NUM_SUBJECT; i++){
            isFirstCall = 1; // ヘッダーのリセット
            display_deviation_scores(subjects[i], day, text);
            CLEAR_INPUT_BUFFER();
        }

        break;
    case 6:
        printf("隠し機能：全科目偏差値(全日程)\n");

        
        CLEAR_INPUT_BUFFER();
        isFirstCall = 1; // ヘッダーのリセット
        display_total_deviation_scores(text);

        break;

    case 7:
    case 8:
    case 9:
        printf("表示できるものがありません。\n");
        printf("最初からやり直してください\n");
        return 1;
        break;
    case 0:

        return disp_choice1();

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

int disp_ID(char *text){
    printf("IDを表示します。\n" );

    snprintf(text, MAX_SQL_SIZE,
             "SELECT ID, name, exam_day FROM %s ;",
              table_name);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback, 0);

    return 0;
}

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
                 "SELECT name, exam_day, %s, RANK() OVER (ORDER BY %s DESC) AS ranking "
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
                 "SELECT * FROM (  SELECT name, exam_day, "
                 "   ( %s ) AS total_score,  "
                 "RANK() OVER (ORDER BY "
                 "    ( %s) DESC) AS ranking  "
                 "FROM %s  "
                 ") AS ranked_data  "
                 "WHERE ranking <= %d  "
                 "ORDER BY ranking ASC;",
                 TOTAL_SCORE, TOTAL_SCORE, table_name, person);
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
                 "SELECT * FROM ( "
                 "SELECT name, exam_day, %s, RANK() OVER (ORDER BY %s DESC) AS ranking "
                 "FROM %s WHERE %s IS NOT NULL AND exam_day = %d "
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
                 "SELECT * FROM (  SELECT name, exam_day, "
                 "   ( %s ) AS total_score,  "
                 "RANK() OVER (ORDER BY "
                 "    ( %s ) DESC) AS ranking  "
                 "FROM %s  WHERE exam_day = %d "
                 ") AS ranked_data  "
                 "WHERE ranking <= %d  "
                 "ORDER BY ranking ASC;",
                 TOTAL_SCORE, TOTAL_SCORE, table_name, day, person);
    }else{
        printf("エラー: person の値が不正です。\n");
    }

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 特定の科目の平均点を算出するヘルパー関数
/// @param subject    科目名（列名）
/// @param day        日付を指定する場合はその値、全体の平均を求めるなら0を指定
/// @param text     
/// @return 平均点（エラー時にはマイナス値を返すなど、エラー処理は適宜実装）
double calc_subject_average(const char *subject, int day, char *text){
    double average = 0.0;

    if (day > 0){   //日付ごと
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT AVG(%s) FROM %s WHERE exam_day = %d AND %s IS NOT NULL;",
                 subject, table_name, day, subject);
    }else{          //全体
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT AVG(%s) FROM %s WHERE %s IS NOT NULL;",
                 subject, table_name, subject);
    }

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback_avg, &average);
    if (rc != SQLITE_OK){
        fprintf(stderr, "平均点の計算に失敗しました。\n");
        return -1; // エラー時はマイナス値など、適切なエラーコードを返す
    }

    return average;
}

/// @brief 全体の平均点を算出するヘルパー関数
/// @param day 日付(指定しない場合は0)
/// @param text 
/// @return 
double calc_average(int day, char *text){
    double average = 0.0;
    char where_clauses[100] = "";

    if (day > 0){
        snprintf(where_clauses, sizeof(where_clauses), " WHERE exam_day = %d", day);
    }
    snprintf(text, MAX_SQL_SIZE,
            "SELECT SUM(individual_avg) / COUNT(DISTINCT name) AS overall_avg"
            " FROM ("
            " SELECT name, "
            " CAST(( %s ) AS REAL) / "
            " NULLIF( (nLang IS NOT NULL) + (math IS NOT NULL) + (Eng IS NOT NULL) +"
            " (JHist IS NOT NULL) + (wHist IS NOT NULL) + (geo IS NOT NULL) +"
            " (phys IS NOT NULL) + (chem IS NOT NULL) + (bio IS NOT NULL), 0) AS individual_avg"
            " FROM %s"
            " %s "
            " ) AS avg_per_person;",
            TOTAL_SCORE, table_name, where_clauses);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback_avg, &average);
    if (rc != SQLITE_OK){
        fprintf(stderr, "全体平均点の計算に失敗しました。\n");
        return -1;
    }

    return average; // 平均値を戻り値
}

/*/// @brief 全体平均点検索
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
             TOTAL_SCORE, table_name);

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
        // snprintf(text, MAX_SQL_SIZE,
        // "SELECT name, day, "
        // "   SUM(COALESCE(nLang, 0) + COALESCE(math, 0) + COALESCE(Eng, 0) + "
        // "       COALESCE(JHist, 0) + COALESCE(wHist, 0) + COALESCE(geo, 0) + "
        // "       COALESCE(phys, 0) + COALESCE(chem, 0) + COALESCE(bio, 0)) / "
        // "   (COUNT(nLang) + COUNT(math) + COUNT(Eng) + COUNT(JHist) + "
        // "    COUNT(wHist) + COUNT(geo) + COUNT(phys) + COUNT(chem) + COUNT(bio)) "
        // "   AS avg_score  "
        // "FROM %s WHERE day = %d GROUP BY name, day;"
        // ,table_name,day);

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
             TOTAL_SCORE, table_name, day);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback_avg, &average);

    return average; // 全員の得点の平均値を戻り値
}
*/
/// @brief 日程ごとの科目平均点以下生徒検索
/// @param day 日程検索
/// @param subject 科目
/// @param text
/// @return
int under_average(int day, char *subject, char *text){
    double average = calc_subject_average(subject,day,text);

    printf("%sの平均点は%.1f点です。\n", subject, average);
    printf("%sの得点が平均点以下の生徒を表示します。\n", subject);

    snprintf(text, MAX_SQL_SIZE,
             "WITH subject_avg AS ( "
             "    SELECT AVG( %s ) AS avg_score "
             "    FROM %s "
             "    WHERE exam_day = %d  AND %s IS NOT NULL"
             " ) "
             " SELECT p.name, p.exam_day, p.%s, sa.avg_score "
             " FROM %s p "
             " JOIN subject_avg sa "
             " WHERE p.exam_day = %d AND p.%s <= sa.avg_score "
             " ORDER BY p.%s DESC;",
             subject, table_name, day, subject, subject, table_name, day, subject, subject);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 日程ごとの平均点以下生徒検索
/// @param day 日程
/// @param text
/// @return
int under_average_sum(int day, char *text){
    double average = calc_average(day, text);

    // printf("試験実施日毎の全科目平均点数以下の受験者一覧を表示します\n");

    printf("%dにおける全体の平均点は%.1f点です。\n", day, average);
    printf("全体の得点が平均点以下の生徒を表示します。\n");

    snprintf(text, MAX_SQL_SIZE,

             "SELECT * FROM (  SELECT name, exam_day, "
             "  CAST( SUM( %s )  AS REAL)/ "
             "  NULLIF( (COUNT(nLang) + COUNT(math) + COUNT(Eng) + COUNT(JHist) + "
             "    COUNT(wHist) + COUNT(geo) + COUNT(phys) + COUNT(chem) + COUNT(bio)) ,0)"
             "   AS avg_score  "
             " FROM %s  WHERE exam_day = %d GROUP BY name, exam_day "
             " )   "
             " WHERE avg_score <= %f  "
             " ORDER BY avg_score DESC; ",
             TOTAL_SCORE, table_name, day, average);

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
    double average = calc_subject_average(subject,0,text);

    printf("%sの平均点は%.1f点です。\n", subject, average);
    printf("%sの得点が平均点以下の生徒を表示します。\n", subject);

    snprintf(text, MAX_SQL_SIZE,
             "WITH subject_avg AS ( "
             "    SELECT AVG( %s ) AS avg_score "
             "    FROM %s "
             "    WHERE  %s IS NOT NULL"
             " ) "
             " SELECT p.name, p.exam_day, p.%s, sa.avg_score "
             " FROM %s p "
             " JOIN subject_avg sa "
             " WHERE  p.%s <= sa.avg_score "
             " ORDER BY p.%s DESC;",
             subject, table_name, subject, subject, table_name, subject, subject);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

/// @brief 全体平均点以下生徒検索
/// @param text
/// @return
int under_average_sum_all(char *text){

    double average = calc_average(0,text);

    printf("全体の平均点は%.1f点です。\n", average);
    printf("全体の得点が平均点以下の生徒を表示します。\n");

    snprintf(text, MAX_SQL_SIZE,
             "SELECT * FROM (  SELECT name, exam_day, "
             "  CAST( SUM( %s )  AS REAL)/ "
             "  NULLIF( (COUNT(nLang) + COUNT(math) + COUNT(Eng) + COUNT(JHist) + "
             "    COUNT(wHist) + COUNT(geo) + COUNT(phys) + COUNT(chem) + COUNT(bio)) ,0)"
             "   AS avg_score  "
             " FROM %s   GROUP BY name, exam_day "
             " )   "
             " WHERE avg_score <= %f  "
             " ORDER BY avg_score DESC; ",
             TOTAL_SCORE, table_name, average);

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    int rc = execute_sql(text, callback2, 0);

    return 0;
}

double calc_stddev(const char *subject, int day, char *text){
    double stddev = 0;
    // 例：day指定の場合の標準偏差算出クエリ
    snprintf(text, MAX_SQL_SIZE,
             "WITH stats AS ("
             "   SELECT AVG(%s) AS avg_val FROM %s WHERE exam_day = %d AND %s IS NOT NULL"
             "), stddev_calc AS ("
             "   SELECT sqrt(AVG((%s - stats.avg_val) * (%s - stats.avg_val))) AS std_dev "
             "   FROM %s, stats WHERE exam_day = %d AND %s IS NOT NULL"
             ")"
             "SELECT std_dev FROM stddev_calc;",
             subject, table_name, day, subject,
             subject, subject, table_name, day, subject);

    // execute_sql()を利用してクエリを実行しstddev を取得する
    int rc = execute_sql(text, callback_avg, &stddev);
    return stddev;
}

double calc_median(const char *subject, int day, char *text){
    double median = 0;
    // 例：day指定の場合の中央値取得クエリ（ウィンドウ関数利用）
    snprintf(text, MAX_SQL_SIZE,
             "WITH ordered_scores AS ("
             "  SELECT %s AS score, ROW_NUMBER() OVER (ORDER BY %s) AS rn, "
             "         COUNT(*) OVER () AS total_count "
             "  FROM %s WHERE exam_day = %d AND %s IS NOT NULL"
             ") "
             "SELECT AVG(score) AS median FROM ordered_scores "
             "WHERE rn IN ((total_count + 1)/2, (total_count + 2)/2);",
             subject, subject, table_name, day, subject);

    int rc = execute_sql(text, callback_avg, &median);
    return median;
}

/*
void display_deviation_scores(char *subject, int day, char *text){
    double avg =calc_subject_average(subject,0,text); // 既存の平均取得関数
    double stddev = calc_stddev(subject, day, text);

    printf("%sの偏差値一覧：\n", subject);
    snprintf(text, MAX_SQL_SIZE,
             //"WITH stats AS ("
             //" SELECT %s AS score,"
             //" SELECT AVG(%s) FROM %s WHERE day = %d AND %s IS NOT NULL) AS avg_val, "
             //"    (SELECT sqrt(AVG((%s - (SELECT AVG(%s) FROM %s WHERE day = %d AND %s IS NOT NULL)) * "
             //"(%s - (SELECT AVG(%s) FROM %s WHERE day = %d AND %s IS NOT NULL)))) AS std_dev "
             //" FROM %s WHERE day = %d AND %s IS NOT NULL "
             //") "
             //"SELECT name, score, "
             //" CASE WHEN (SELECT std_dev FROM stats) = 0 THEN 50 "
             //"   ELSE 50 + 10 * (score - (SELECT avg_val FROM stats)) / (SELECT std_dev FROM stats) END AS deviation "
             //"FROM %s WHERE day = %d AND %s IS NOT NULL ORDER BY deviation DESC;",

             "WITH stats AS ("
             " SELECT "
             "  AVG( %s ) AS avg_val, "
             "  sqrt(AVG(( %s - AVG( %s )) * ( %s - AVG( %s )))) AS std_dev "
             " FROM %s  WHERE day = %d AND %s IS NOT NULL "
             ") "
             "SELECT"
             " %s.name, "
             " %s.%s AS score,"
             " CASE "
             "  WHEN stats.std_dev = 0 THEN 50"
             "  ELSE 50 + 10 * (%s.%s - stats.avg_val) / stas.std_dev"
             " END AS deviation "
             "FROM %s "
             "CROSS JOIN stats "
             "WHERE %s.day = %d AND %s.%s IS NOT NULL "
             "ORDER BY deviation DESC",

            // " WITH stats AS("
            // "    SELECT"
            // "    AVG(nLang) AS avg_val,"
            // "    sqrt(AVG((nLang - (SELECT AVG(nLang) FROM pT"
            // "     WHERE day = 20200202 AND nLang IS NOT NULL)) *"
            // "   (nLang - (SELECT AVG(nLang) FROM pT"
            // "    WHERE day = 20200202 AND nLang IS NOT NULL)))) AS std_dev"
            // " FROM pT"
            // "  WHERE day = 20200202 AND nLang IS NOT NULL)"
            // "  SELECT"
            // "  pT.name,"
            // "  pT.nLang AS score,"
            // "  CASE"
            // "    WHEN stats.std_dev = 0 THEN 50 ELSE 50 + 10 * (pT.nLang - stats.avg_val) / "
            // "    stats.std_dev END AS deviation FROM pT CROSS JOIN stats WHERE pT.day = 20200202 AND pT.nLang IS NOT NULL"
            // "    ORDER BY deviation DESC;");
             subject,subject,subject,subject,subject,
             table_name,day,subject,
             table_name,table_name,subject,table_name,subject,
             table_name,table_name,day,table_name,subject

             //subject, subject, table_name, day, subject,
             //subject, subject, table_name, day, subject,
             //subject, subject, table_name, day, subject,
             //table_name, day, subject,
             //table_name, day, subject
             );

#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif

    // execute_sql()を用いて、callback2で結果を表示
    int rc = execute_sql(text, callback2, /*context_data NULL);
}
*/

///@brief 指定された科目・日付ごとの標準偏差を算出するヘルパー関数
///@param subject 科目名（列名）
///@param day     日付を指定する場合はその値、全体の計算なら0
///@param text    SQL文バッファ
///@return 標準偏差（エラーの場合は-1を返す）
double calc_subject_std(const char *subject, int day, char *text){
    double avg = calc_subject_average(subject, day, text);
    if (avg < 0){
        // 平均値の計算に失敗している場合はエラー扱い
        return -1;
    }
    double avg_sq = 0.0;

    if (day > 0){
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT AVG((%s) * (%s)) FROM %s WHERE exam_day = %d AND %s IS NOT NULL;",
                 subject, subject, table_name, day, subject);
    }else{
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT AVG((%s) * (%s)) FROM %s WHERE %s IS NOT NULL;",
                 subject, subject, table_name, subject);
    }
#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif
    int rc = execute_sql(text, callback_avg, &avg_sq);
    if (rc != SQLITE_OK){
        fprintf(stderr, "%sの標準偏差の計算に失敗しました。\n",subject);
        return -1;
    }
    // 分散 = AVG(score^2) - (AVG(score))^2, 標準偏差はその平方根
    double variance = avg_sq - (avg * avg);
    if (variance < 0){
        variance = 0; // 浮動小数点の丸め誤差対策
    }
    double std = sqrt(variance);
    return std;
}

///@brief 偏差値を計算して表示する
///@param subject 科目名（列名）
///@param day     日付（指定する場合はその値、全体なら0）
///@param text    SQL文バッファ
void display_deviation_scores(const char *subject, int day, char *text){
    // 平均値と標準偏差を計算
    double avg = calc_subject_average(subject, day, text);
    double std = calc_subject_std(subject, day, text);
    if (avg < 0 || std <= 0){
        fprintf(stderr, "%sの平均または標準偏差の取得に失敗しました。\n",subject);

        if(std==0){
            printf("標準偏差が0なため、全員同得点の可能性があります。\n");
            printf("この際の偏差値は全員50.0です。\n");
        }
        return;
    }
    printf("%sの平均点は %.1f, 標準偏差は %.1f です。\n", subject, avg, std);
    printf("偏差値（50 + 10 * (得点 - 平均)/標準偏差）を計算します。\n");
    // 各学生の得点を取得するためのクエリを作成
    if (day > 0){
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT name, exam_day, %s FROM %s WHERE exam_day = %d AND %s IS NOT NULL ORDER BY %s DESC;",
                 subject, table_name, day, subject, subject);
    }else{
        snprintf(text, MAX_SQL_SIZE,
                 "SELECT name, exam_day, %s FROM %s WHERE %s IS NOT NULL ORDER BY %s DESC;",
                 subject, table_name, subject, subject);
    }
#ifdef DEBUG
    printf("実行するSQL: %s\n", text);
#endif
    // 上記クエリの結果を処理するためのコールバックに、偏差値計算に必要な情報を渡す
    DeviationContext dev_ctx;
    dev_ctx.avg = avg;
    dev_ctx.std = std;
    int rc = execute_sql(text, deviation_callback, &dev_ctx);
    if (rc != SQLITE_OK){
        fprintf(stderr, "偏差値の表示に失敗しました。\n");
    }
}

/// @brief 合計得点に対する偏差値を計算して表示する関数
void display_total_deviation_scores(char *text){
    TotalStats stats = {0, 0};

    /* ① 全生徒の合計得点（例：各科目の得点の総和）を算出し、
          その平均（avg_total）と標準偏差（std_total）を計算するクエリ */
    snprintf(text, MAX_SQL_SIZE,
             "WITH totals AS ("
             "  SELECT name, exam_day, SUM(%s) AS total_score "
             "  FROM %s "
             "  GROUP BY name, exam_day"
             ") "
             "SELECT AVG(total_score) AS avg_total, "
             "       sqrt(AVG(total_score * total_score) - AVG(total_score)*AVG(total_score)) AS std_total "
             "FROM totals;",
             TOTAL_SCORE, table_name);
#ifdef DEBUG
    printf("実行するSQL (統計量計算): %s\n", text);
#endif
    int rc = execute_sql(text, callback_total_stats, &stats);
    if (rc != SQLITE_OK){
        fprintf(stderr, "合計得点の統計量（平均・標準偏差）の計算に失敗しました。\n");
        return;
    }

    if (stats.std <= 0){
        printf("標準偏差が0のため、全員同得点の可能性があります。偏差値は全員50.0です。\n");
        return;
    }
    printf("合計得点の平均は %.1f, 標準偏差は %.1f です。\n", stats.avg, stats.std);
    printf("偏差値（50 + 10 * (得点 - 平均)/標準偏差）を計算します。\n");

    /* ② 各生徒ごとの合計得点とその偏差値を取得するクエリを作成
          CTE内で既に個々の合計得点を算出してから、偏差値の計算を行います */
    snprintf(text, MAX_SQL_SIZE,
             "WITH totals AS ("
             "  SELECT name, exam_day, SUM(%s) AS total_score "
             "  FROM %s "
             "  GROUP BY name, exam_day"
             ") "
             "SELECT name, exam_day, total_score, "
             "       CASE WHEN %f = 0 THEN 50 "
             "            ELSE 50 + 10 * (total_score - %f) / %f END AS deviation "
             "FROM totals "
             "ORDER BY deviation DESC;",
             TOTAL_SCORE, table_name, stats.std, stats.avg, stats.std);
#ifdef DEBUG
    printf("実行するSQL (偏差値計算): %s\n", text);
#endif
    rc = execute_sql(text, deviation_callback, &stats);
    if (rc != SQLITE_OK){
        fprintf(stderr, "合計得点に対する偏差値の表示に失敗しました。\n");
    }
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
