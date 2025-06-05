#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h> // setlocale のため
#include <ctype.h>  // tolower のため

#include "sqlite3.h"
#include "db_operations.h"     // DB操作関連の関数

int main(int argc, char **argv) {
    setlocale(LC_ALL, "ja_JP.UTF-8"); // ロケール設定

    sqlite3 *db;
    
    // DB接続
    // "testmanager.db" を使用
    db = connect_to_database("testmanager.db");
    
    // テーブル作成（初回起動時のみ実行されると仮定）
    if (create_tables(db) != 0) { // connect_to_databaseから返されたdbポインタを使用
        sqlite3_close(db);
        return(1);
    }

    int firstNum; // ユーザーの選択を保持する変数

    do {
        printf("\n---テスト結果管理ツールへようこそ---\n");
        printf("---利用したい機能を選択してください---\n");
        printf("1.【登録】\n");
        printf("2.【変更】\n");
        printf("3.【削除】\n");
        printf("4.【参照】\n");
        printf("\n");
        printf("終了したい場合は 5 を入力してください\n");
        printf(">"); // 入力待機状態アイコン

        // 機能選択の入力処理
        if (scanf("%d", &firstNum) != 1) {
            printf("無効な入力です。数字を入力してください。\n");
            while (getchar() != '\n'); // 不正な入力をクリア
            firstNum = -1; // ループを継続させるための範囲外の値
            continue;
        }
        while (getchar() != '\n'); // scanfの後の改行を消費

        // 入力値の範囲チェック
        if (firstNum < 1 || firstNum > 5) {
            printf("入力された数字が範囲外です (1～5)。再度入力してください。\n");
            continue;
        }

        // ユーザーが5を選択した場合はループを抜ける（終了）
        if (firstNum == 5) {
            break;
        }

        // 選択された操作に応じた処理
        switch (firstNum) {
            case 1: // 1.登録
                // TODO: 登録処理を呼び出す関数を追加
                break;
            case 2: // 2.変更
                printf("受験者情報の変更ができます。（未実装）\n");
                // TODO: 変更処理を呼び出す関数を追加
                break;
            case 3: // 3.削除
                printf("受験者情報の削除ができます。（未実装）\n");
                // TODO: 削除処理を呼び出す関数を追加
                break;
            case 4: // 4.参照
                printf("結果による様々な情報を参照できます。\n");
                // TODO: 参照処理を呼び出す関数を追加
                break;
            default:
                break;
        }

    } while (firstNum != 5);

    printf("テスト結果管理ツールを終了します。\n");

    // DBクローズ
    sqlite3_close(db);

    return 0;
}