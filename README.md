# Usapyon2dash
うさぴょん２からの派生。

将棋電王トーナメントに参加したバージョン（＋αになる予定）

2016/10/15　バグ残ってると思われるバージョンをとりあえずＵＰ。

誰かバグ指摘してくれんかなぁ…。
多分ponder絡み。


電王戦トーナメントで使った評価バイナリは下記よりダウンロード可能。
（usapyon2dash.exeと同じディレクトリに配置して下さい）

https://drive.google.com/drive/folders/0B0pVFBgivYmac1BPZHZXaDZtajg


定跡は、

http://usapyon.game.coocan.jp/usapyon2/index.html

から先後共にダウンロード可能。
（同じくusapyon2dash.exeと同じディレクトリに配置して下さい）

# 2016/10/22

Write_Debug_Logがファイル名指定なのに、チェックボックスになっていたのを修正。

エンジン設定を引き継ぐと、falseというファイル名になってしまうので、その際には空にして下さい。

もしかすると、ログを吐きながら将棋所と繋ぐと時々文字化け？してbestmoveが帰らない現象があったのかも知れない…。
（結果としてタイムアウトで負ける）
