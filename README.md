# Usapyon2dash
うさぴょん２からの派生。

将棋電王トーナメントに参加したバージョン（＋αになる予定）

2016/10/15　バグ残ってると思われるバージョンをとりあえずＵＰ。

誰かバグ指摘してくれんかなぁ…。
多分ponder絡み。


電王戦トーナメントで使った評価バイナリは下記よりダウンロード可能。
（usapyon2dash.exe　または usapyon2dashAVX.exe　と同じディレクトリに配置して下さい）

~~https://drive.google.com/drive/folders/0B0pVFBgivYmac1BPZHZXaDZtajg~~
　＜Googleドライブがあふれてしまったのでここでの配布は中止しました

定跡は、

http://usapyon.game.coocan.jp/usapyon2/index.html

から先後共にダウンロード可能です。うさぴょん２の定跡ファイルと共通です。
（同じくusapyon2dash.exe　または usapyon2dashAVX.exe　と同じディレクトリに配置して下さい）

## 評価バイナリ・定跡について(2022/10/08追記)

https://github.com/YasuhiroIke/Usapyon2dash/releases

から、評価バイナリ・定跡共にダウンロード可能です。
（一式全部持って行くと良いかと思います）

## ビルド方法について

VisualStudioが必要です。（直近では、Visual Studio 2019で確認）

 x64 Native Tools Command Prompt
を開き、ソースコードのあるディレクトリにて、

nmake -f Makefile.vs
もしくは
nmake -f MakefileAVX.vs
を実行すると、ソースコードと同じディレクトリに、obj/exeファイルが
（ビルドに成功すれば）出来ます。

(警告がいっぱい出るのですが、致命的なものはないので、無視して下さい。)


## 2016/10/22

Write_Debug_Logがファイル名指定なのに、チェックボックスになっていたのを修正。

エンジン設定を引き継ぐと、falseというファイル名になってしまうので、その際には空にして下さい。

もしかすると、ログを吐きながら将棋所と繋ぐと時々文字化け？してbestmoveが帰らない現象があったのかも知れない…。
（結果としてタイムアウトで負ける）

## 2016/10/29

長時間対局テストを繰り返した結論として、ログファイルを吐きながら対局すると、USIのGUIとの通信に失敗していたようです。

というわけで、これで一応ＦＩＸとさせていただきます。
思考ログをファイルに取りたい方は「ごめんなさい、バグってます…」と謝るしかありません…。


## 2016/11/11

以下の場所にリリースしていたのにここでお知らせをしていませんでした…すみません…。

以下の場所から必要なファイルは全て取得できます。

https://github.com/YasuhiroIke/Usapyon2dash/releases
