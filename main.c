#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h> // setlocale のため
#include <ctype.h>  // tolower のため

#include "sqlite3.h"
#include "db_operations.h"     // DB操作関連の関数
#include "data_validation.h"   // 入力検証関連の関数
#include "subject_utils.h"     // 科目ユーティリティ関連の関数

// 登録内容確認関数（元のmain関数にあったものをこちらに移動）
void confirm_registration(const char *name, const char *exam_date, char subjects[][20], int scores[], int subject_count) {
    printf("\n--- 登録内容確認 ---\n");
    printf("名前: %s\n", name);
    printf("試験日: %s\n", exam_date);
    printf("科目と点数:\n");
    for (int i = 0; i < subject_count; i++) {
        printf("  %s: %d点\n", subjects[i], scores[i]);
    }
    printf("--------------------\n");
}

// 登録処理のメイン関数（元のmain関数の登録部分を切り出し）
void handle_registration_flow(sqlite3 *db) {
    char name[61]; // 20文字以内ならUTF-8で最大60バイト + ヌル終端
    char exam_date[9];
    int user_id = 0, exam_session_id = 0, score, subject_choice;
    char continue_choice;

    char subjects_input[10][20]; // 登録する科目名を一時的に保持
    int scores_input[10];      // 登録する点数を一時的に保持
    int current_subject_count = 0;

    // social_selectedとscience_selectedはsubject_utils.cで定義されているグローバル変数
    // ここでリセットしておかないと、複数回登録を繰り返した場合に問題になる
    social_selected = 0;
    science_selected = 0;

    // ユーザー選択：新規登録 or 既登録受験者
    char user_option;
    printf("新規登録(n) もしくは 既登録受験者(e) を選択してください: ");
    scanf(" %c", &user_option);
    // scanf("%d", ...) の後に fgets や getchar() を使う場合は、改行文字の消費に注意
    // getchar() でバッファをクリアするのが確実
    while (getchar() != '\n'); 

    if (tolower(user_option) == 'n') {
        // 名前入力（新規登録）
        do {
            printf("名前を入力してください（全角カタカナ20文字以内）: ");
            fgets(name, sizeof(name), stdin);
            name[strcspn(name, "\n")] = '\0'; // 改行文字を削除
        } while (!validate_name(name));

        user_id = register_user(db, name);
        if (user_id == 0) {
            fprintf(stderr, "受験者登録に失敗しました。\n");
            return;
        }
        printf("受験者ID: %d が登録されました。\n", user_id); //IDを表示する
    } else if (tolower(user_option) == 'e') {
        // 既登録受験者ID入力
        printf("既登録受験者のIDを入力してください: ");
        scanf("%d", &user_id);
        while (getchar() != '\n'); // 改行を消費

        // 入力されたIDが存在するかチェック
        sqlite3_stmt *stmt;
        const char *check_sql = "SELECT name FROM users WHERE id = ?";
        if (sqlite3_prepare_v2(db, check_sql, -1, &stmt, NULL) != SQLITE_OK) {
            fprintf(stderr, "データベースエラー: %s\n", sqlite3_errmsg(db));
            return;
        }
        sqlite3_bind_int(stmt, 1, user_id);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            const char *registered_name = (const char *)sqlite3_column_text(stmt, 0);
            // 既存ユーザーの名前をname変数にコピーしておく (確認用)
            strncpy(name, registered_name, sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
            printf("登録済み受験者: %s (ID: %d)\n", name, user_id);
        } else {
            printf("エラー: 受験者ID %d は存在しません。\n", user_id);
            sqlite3_finalize(stmt);
            return;
        }
        sqlite3_finalize(stmt);
    } else {
        printf("無効な選択です。登録処理を中断します。\n");
        return;
    }

    // 試験日入力
    do {
        printf("試験日を入力してください（例: 20250513）: ");
        scanf("%8s", exam_date);
        while (getchar() != '\n'); // 改行を消費
    } while (!validate_date(exam_date));

    // 科目と点数登録
    do {
        display_subjects();
        do {
            printf("科目を選択してください（1〜9）: ");
            if (scanf("%d", &subject_choice) != 1) {
                printf("無効な入力です。数字を入力してください。\n");
                while (getchar() != '\n');
                subject_choice = -1; // 無効な選択
                continue;
            }
            while (getchar() != '\n'); // 改行を消費

            // 社会・理科の排他選択ロジック
            // i=3(日本史), 4(世界史), 5(地理) は社会
            // i=6(物理), 7(化学), 8(生物) は理科
            if (subject_choice >= 4 && subject_choice <= 6) { // 社会科目 (1-indexed: 4,5,6)
                if (social_selected && (subject_choice != 4 && subject_choice != 5 && subject_choice != 6)) {
                    printf("エラー: 社会科目は既に選択されています。他の社会科目は選択できません。\n");
                    subject_choice = -1; // 無効な選択
                } else {
                    social_selected = 1;
                }
            } else if (subject_choice >= 7 && subject_choice <= 9) { // 理科科目 (1-indexed: 7,8,9)
                if (science_selected && (subject_choice != 7 && subject_choice != 8 && subject_choice != 9)) {
                    printf("エラー: 理科科目は既に選択されています。他の理科科目は選択できません。\n");
                    subject_choice = -1; // 無効な選択
                } else {
                    science_selected = 1;
                }
            }

        } while (subject_choice < 1 || subject_choice > 9); // 有効な科目番号が入力されるまで繰り返す

        const char *subject_name_str = get_subject_name(subject_choice);
        if (subject_name_str != NULL) {
            // 既に選択された科目かチェック
            int already_selected = 0;
            for (int i = 0; i < current_subject_count; i++) {
                if (strcmp(subjects_input[i], subject_name_str) == 0) {
                    printf("エラー: その科目は既に登録済みです。別の科目を選択してください。\n");
                    already_selected = 1;
                    break;
                }
            }
            if (already_selected) {
                continue; // 再度科目選択からやり直し
            }

            strcpy(subjects_input[current_subject_count], subject_name_str);
        } else {
            printf("無効な科目選択です。\n");
            continue; // 再度科目選択からやり直し
        }

        do {
            printf("点数を入力してください: ");
            if (scanf("%d", &score) != 1) {
                printf("無効な入力です。数字を入力してください。\n");
                while (getchar() != '\n');
                score = -1; // 無効なスコア
                continue;
            }
            while (getchar() != '\n'); // 改行を消費
        } while (!validate_score(score));

        scores_input[current_subject_count] = score;
        current_subject_count++;

        if (current_subject_count >= 10) { // 最大科目数に達したら強制終了
            printf("登録可能な科目数の上限に達しました。\n");
            break;
        }

        printf("他の科目を登録しますか？（y/n）: ");
        scanf(" %c", &continue_choice);
        while (getchar() != '\n'); // 改行を消費
    } while (tolower(continue_choice) == 'y');

    // 登録内容確認
    confirm_registration(name, exam_date, subjects_input, scores_input, current_subject_count);

    // 試験セッション登録
    exam_session_id = register_exam(db, user_id, exam_date);
    if (exam_session_id == 0) {
        fprintf(stderr, "試験セッション登録に失敗しました。処理を中断します。\n");
        return;
    }

    // 点数の登録
    for (int i = 0; i < current_subject_count; i++) {
        register_score(db, exam_session_id, subjects_input[i], scores_input[i]);
    }

    printf("登録が完了しました。\n");
}


int main(int argc, char **argv) {
    setlocale(LC_ALL, "ja_JP.UTF-8"); // ロケール設定

    sqlite3 *db;
    
    // DB接続
    // "testmanager.db" を使用 (元のコードに合わせた)
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
        printf(">"); // ユーザー入力のプロンプト

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
                handle_registration_flow(db); // 登録処理を呼び出す
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
                //select_data(db); // 仮の参照関数を呼び出し
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