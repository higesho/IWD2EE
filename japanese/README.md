# Icewind Dale II（IWD2EE）日本語化パッチ

GOG.com 版 Icewind Dale II に IWD2EE を入れた状態を、日本語で遊べるようにする非公式のパッチです。

- 紹介ページ・入れ方: https://higesho.github.io/IWD2EE/
- ダウンロード: https://github.com/higesho/IWD2EE/releases

## このフォルダの中身

配布物（`IcewindDale2EE_JP.zip`）に入っているプログラムのソースです。

- `src/iwd2jp.c` — `IWD2JP.exe`。ゲームのフォルダの `dialog.tlk`・`Party.ini`・`Characters\*.RES`／`*.CHR`・`override\IEex_TRA.lua` の英文を、訳の表 `IWD2JP.dat` の日本語に置き換え、`icewind2.ini` に日本語表示の設定を書き、`override\IWD2JP.lua` を組み込みます。訳の表は英文を持たず、英文のハッシュ値（FNV-1a 64 ビット）で引きます。`/restore` で元に戻します。`build.bat` で組み立てます（Visual Studio 2022、x86）。
- `src/IWD2JP.lua.in` — IEex の上で動く手直し（`IWD2JP.lua` の元）。
  - 行頭・行末禁則: 行を折る処理（`CUtil_SplitString`）の中で折る位置を決める箇所に割り込み、`kinsoku.c` の処理で位置を調整します。
  - IWD2EE の lua が `dialog.tlk` の文を文字列検索のパターンとして使うため、Shift_JIS の 2 バイト目の記号（`[` `]` `^`）を普通の文字として扱わせます。
- `src/kinsoku.c` — 禁則の処理。`build_kinsoku.bat` で組み立て、`extract_obj.py` で機械語を取り出し、`make_lua.py` で `IWD2JP.lua` に埋め込みます。

## ライセンス

ソースは自由に使ってかまいません。ゲームの文章・画像の権利は原作の権利者にあります。
