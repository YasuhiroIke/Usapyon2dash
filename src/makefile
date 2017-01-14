#
# なのはminiにするときはNANOHAMINI=1の前の#を取り、
# なのはnanoにするときはNANOHAMINI=1の前に#を付け、NANOHANANO=1の前の#を取る
# nanoperyにするときはNANOPERY=1の前の#を取る
#
#NANOHAMINI=1
#NANOHANANO=1

EVAL_TYPE=EVAL_APERY
EVAL_OBJ=evaluate_apery.o
EXE = usapyon2dashAVX
PGD = usapyon2dashAVX.pgd
PGOLOG = usapyon2dashAVX_prof.txt

OBJS = mate1ply.o misc.o timeman.o $(EVAL_OBJ) position.o \
	 tt.o main.o move.o \
	 movegen.o search.o uci.o movepick.o thread.o ucioption.o \
	 benchmark.o book.o \
	 shogi.o mate.o problem.o

CC=g++
LD=link



# Compile Options
#
# -DEVAL_MINI      なのはmini(2駒関係(KP+PP)の評価関数)
# -DEVAL_NANO      なのはnano(2駒関係(KPのみ)の評価関数)
# -DEVAL_APERY     nanopery(Aperyの評価関数)
#
# -DTWIG           Apery(大樹の枝)の評価関数を使う
# -DUSE_AVX2_EVAL  評価関数の計算時にAVX命令を使う
#
# Visual C++オプション
#
# /D_CRT_SECURE_NO_WARNINGS
#                   secureな関数を使っていないときの警告を出さない
# /Zc:forScope      スコープ ループに標準 C++ を適用する
# /Wall             警告をすべて有効にする
# /GS[-]            セキュリティ チェックを有効にする
# /favor:<blend|AMD64|EM64T> 最適化するプロセッサ
# /GL[-]            リンク時のコード生成を行う
# /RTCs             スタック フレーム ランタイム チェック
# /RTCu             初期化されていないローカル変数のチェック

FLAGS = -DNDEBUG -D$(EVAL_TYPE) -DUSAPYON2 -DNANOHA -DCHK_PERFORM -DTWIG -DUSE_AVX2_EVAL\
	-DOLD_LOCKS \
	 
#CXXFLAGS=$(FLAGS) /MT /W4 /Wall /nologo /Od /GS /RTCsu
CXXFLAGS=$(FLAGS) -O3 -DIS_64BIT -msse -std=c++11 -mavx2 -Wa,-q -c
LDFLAGS=-lpthread -o$(EXE)
PGOLDFLAGS1=/NOLOGO /STACK:16777216,32768 /out:$(EXE) /LTCG:PGI
PGOLDFLAGS2=/NOLOGO /STACK:16777216,32768 /out:$(EXE) /LTCG:PGO


all: $(EXE)

$(EXE) : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)

.cpp.o :
	$(CC) $(CXXFLAGS) $*.cpp

clean :
	rm -f *.o
	rm -f    $(EXE)

