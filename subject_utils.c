#include <stdio.h>
#include "subject_utils.h" // 自身のヘッダーファイルをインクルード

// グローバル変数の定義（他のファイルで extern 宣言されている）
int social_selected = 0; // 社会科目が選択済みか
int science_selected = 0; // 理科科目が選択済みか

// 科目名取得
const char *get_subject_name(int choice) {
    const char *subjects[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};
    if (choice >= 1 && choice <= 9) {
        return subjects[choice - 1];
    }
    return NULL;
}

// 科目一覧表示
void display_subjects() {
    const char *subjects[] = {"国語", "数学", "英語", "日本史", "世界史", "地理", "物理", "化学", "生物"};
    printf("科目一覧:\n");
    for (int i = 0; i < 9; i++) {
        // 社会科目の範囲: 3(日本史), 4(世界史), 5(地理) -> インデックス 2, 3, 4
        // 理科科目の範囲: 6(物理), 7(化学), 8(生物) -> インデックス 5, 6, 7
        if ((i >= 2 && i <= 4 && social_selected) || // 社会科目が選択済み
            (i >= 5 && i <= 7 && science_selected)) { // 理科科目が選択済み
            printf("  %d: %s（選択不可）\n", i + 1, subjects[i]);
        } else {
            printf("  %d: %s\n", i + 1, subjects[i]);
        }
    }
}