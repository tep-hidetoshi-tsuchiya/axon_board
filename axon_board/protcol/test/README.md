# Unit Test Build with Clang

このプロジェクトでは、Clangコンパイラを使用してユニットテストをビルドおよび実行できます。
C言語で書かれた`src/target.c`とC++のテストコードを組み合わせたテストが実行されます。

## 前提条件

- CMake 3.16以上
- Clang 20.1.0以上（C/C++両方対応）
- MinGW Makefiles generator (Windowsの場合)

## テスト対象

- **src/target.c**: C言語で実装されたTarget構造体とcreate_target関数
- **src/target.h**: 関数プロトタイプとTarget構造体の定義
- **test/UnitTest01.cpp**: C++で書かれたCatch2ベースのユニットテスト

## テスト内容

### Target Creation テストケース
1. **Basic target creation**: 基本的なTarget作成のテスト
2. **Multiple target creation**: 複数のTarget作成時の上書き動作確認
3. **Target with different IDs**: 様々なIDでのTarget作成
4. **Target with empty name**: 空文字列での名前設定
5. **Target with null name**: NULL名前での安全性確認

### Addition テストケース
- 基本的な加算機能のテスト（サンプル）

## ビルド方法

### 方法1: 手動でCMakeを使用

```powershell
# ビルドディレクトリを作成
mkdir build-clang
cd build-clang

# CMakeでプロジェクトを構成 (Clangコンパイラを指定)
cmake -DCMAKE_CXX_COMPILER=clang++ -G "MinGW Makefiles" ..

# プロジェクトをビルド
cmake --build .

# テストを実行
ctest

# または、実行ファイルを直接実行
./UnitTest01.exe
```

### 方法2: PowerShellスクリプトを使用

```powershell
# ビルドとテスト実行
./build-clang.ps1 -Test

# クリーンビルド
./build-clang.ps1 -Clean -Test

# 詳細出力付きビルド
./build-clang.ps1 -Verbose -Test
```

### 方法3: Makefileを使用

```cmd
# 全体のビルドとテスト
make -f Makefile.clang all

# 個別のステップ
make -f Makefile.clang configure
make -f Makefile.clang build
make -f Makefile.clang test

# クリーン
make -f Makefile.clang clean
```

## コンパイラ固有の設定

CMakeLists.txtは以下のコンパイラを自動検出し、適切なフラグを設定します：

- **Clang**: `-Wall -Wextra -Wpedantic -Wno-unused-parameter`
- **MSVC**: `/W4 /permissive-`
- **GCC**: `-Wall -Wextra -Wpedantic`

## ファイル構成

```
test/
├── CMakeLists.txt          # CMake設定ファイル (C/C++混在対応, Clang対応)
├── UnitTest01.cpp          # C++ユニットテストソースファイル
├── catch.hpp               # Catch2 ヘッダーファイル
├── build-clang.ps1         # PowerShellビルドスクリプト
├── Makefile.clang          # Makefileビルドスクリプト
├── README.md               # このファイル
├── build/                  # MSVC用ビルドディレクトリ
└── build-clang/            # Clang用ビルドディレクトリ
../src/
├── target.h                # Target構造体とAPI定義（C言語）
└── target.c                # Target実装（C言語）
```

## 実装の特徴

### C/C++混在コード
- C言語で実装された`target.c`をC++のテストから呼び出し
- `extern "C"`を使用してC++とCの相互運用を実現
- CMakeLists.txtでC17とC++17標準を併用

### 静的変数のテスト
- `target.c`は内部で静的変数`_target`を使用
- 複数回の`create_target`呼び出しで前の値が上書きされることを確認
- メモリアドレスが同一であることの検証

## トラブルシューティング

### Clangが見つからない場合
```powershell
# Clangのインストール確認
clang++ --version

# PATHの確認
where clang++
```

### ビルドエラーが発生した場合
```powershell
# キャッシュをクリアして再実行
Remove-Item build-clang -Recurse -Force
./build-clang.ps1 -Clean -Test -Verbose
```
