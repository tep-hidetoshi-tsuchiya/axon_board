# CCS プロジェクト ビルド設定手順

## AXON_BOARD プロジェクト用の CCS ビルド設定

### 前提条件
- TI Code Composer Studio 2030 インストール済み
- MSPM0 SDK 2.06.00.05 インストール済み
- サンプルプロジェクト `gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang` を CCS にインポート済み
- AXON プロジェクトのソースファイルをコピー済み（`copy-to-ccs-sample.ps1` 実行済み）

---

## 必須設定: AXON_BOARD マクロの追加

### 手順

1. **CCS を起動**
   ```powershell
   Start-Process "C:\ti\ccs2030\ccs\theia\ccstudio.exe"
   ```

2. **プロジェクトのプロパティを開く**
   - Project Explorer で `gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang` を右クリック
   - `Properties` を選択

3. **コンパイラシンボルを設定**
   - 左メニュー: **Build** → **Arm Compiler** → **Predefined Symbols**
   - 右側: **Pre-define NAME (--define, -D)** セクション
   - 緑の **+** ボタンをクリック
   - 追加するマクロ:
     ```
     AXON_BOARD
     ```
   - **OK** をクリック

4. **設定を保存**
   - **Apply and Close** をクリック

5. **プロジェクトをクリーン**
   - メニュー: **Project** → **Clean...**
   - プロジェクト選択: `gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang`
   - **Clean** をクリック

6. **プロジェクトをビルド**
   - メニュー: **Project** → **Build Project**
   - または、プロジェクトを右クリック → **Build Project**

---

## ビルドから除外すべきファイル

以下のファイルは単体テスト用のため、本番ビルドから除外してください：

### 除外手順

1. Project Explorer で以下のファイルを右クリック:
   ```
   protcol/test/UnitTest01.cpp
   ```

2. **Resource Configurations** → **Exclude from Build...**

3. **Debug** と **Release** の両方にチェックを入れる

4. **OK** をクリック

---

## 期待される結果

ビルド成功時のメッセージ例:
```
**** Build of configuration 'Debug' for project 'gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang' ****
...
Building target: "gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang.out"
Invoking: Arm Linker
...
Finished building target: "gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang.out"

**** Build Finished ****
```

出力ファイル:
```
C:\DATA\DEVELOP\Git\capsule_toy_control_board_CCS\gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang\Debug\gpio_toggle_output_LP_MSPM0G3507_nortos_ticlang.out
```

---

## トラブルシューティング

### エラー: "SOMA_BOARD or AXON_BOARD is not defined"
- **原因**: `AXON_BOARD` マクロが未定義
- **解決策**: 上記「コンパイラシンボルを設定」を実施

### エラー: "use of undeclared identifier 'CONFIG_*'"
- **原因**: `AXON_BOARD` マクロが未定義のため、`msp_peripheral_config.h` の条件コンパイルが正しく機能していない
- **解決策**: `AXON_BOARD` マクロを追加後、Clean & Rebuild

### エラー: "'unistd.h' file not found" (UnitTest01.cpp)
- **原因**: Catch2 テストフレームワークが POSIX ヘッダーを要求（Windows では利用不可）
- **解決策**: `protcol/test/UnitTest01.cpp` をビルドから除外

### インクルードパスエラー
- **確認事項**: 
  - Properties → Build → Arm Compiler → Include Options
  - 以下のパスが含まれていることを確認:
    ```
    ${PROJECT_ROOT}
    ${PROJECT_ROOT}/Debug
    ${COM_TI_MSPM0_SDK_INSTALL_DIR}/source/third_party/CMSIS/Core/Include
    ${COM_TI_MSPM0_SDK_INSTALL_DIR}/source
    ```

---

## デバッグ実行

ビルド成功後、デバッグを開始できます：

1. **デバッグ設定確認**
   - メニュー: **Run** → **Debug Configurations...**
   - 左側: **Code Composer Studio - Device Debugging**
   - プロジェクト名の設定を確認

2. **デバッグ開始**
   - プロジェクトを右クリック → **Debug As** → **Code Composer Studio Device Debugging**
   - または、ツールバーの Debug アイコンをクリック

3. **ブレークポイント設定**
   - `main.c` の `main()` 関数内で左側の行番号をダブルクリック

4. **実行制御**
   - **Resume (F8)**: 実行続行
   - **Suspend**: 実行一時停止
   - **Step Over (F6)**: ステップオーバー
   - **Step Into (F5)**: ステップイン
   - **Step Out (F7)**: ステップアウト

---

## 参考情報

- 元プロジェクト: `C:\DATA\DEVELOP\Git\capsule_toy_control_board`
- CMake ビルド設定: `CMakeLists.txt` の `add_definitions(-DAXON_BOARD)`
- ペリフェラル設定: `src/peripheral/msp_peripheral_config.h`

---

最終更新: 2025年11月12日
