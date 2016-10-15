/*
  Usapyon2, a USI shogi(japanese-chess) playing engine derived from 
  Stockfish 7 & nanoha-mini 0.2.2.1
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad  (Stockfish author)
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad  (Stockfish author)
  Copyright (C) 2014-2016 Kazuyuki Kawabata (nanoha-mini author)
  Copyright (C) 2015-2016 Yasuhiro Ike

  Usapyon2 is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Usapyon2 is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>
#include <cassert>
#include "position.h"
#include "tt.h"
#include "book.h"
#include "uci.h"
#if defined(EVAL_NANO)
#include "param_nano.h"
#elif defined(EVAL_MINI)
#include "param_mini.h"
#elif defined(EVAL_APERY)
#include "param_apery.h"
#endif

///#define DEBUG_GENERATE

// x ���萔�̂Ƃ��̐�Βl(�v���v���Z�b�T���x���Ŋm�肷��̂Łc)
#define ABS(x)	((x) > 0 ? (x) : -(x))

const uint32_t Hand::tbl[HI+1] = {
	0, HAND_FU_INC, HAND_KY_INC, HAND_KE_INC, HAND_GI_INC, HAND_KI_INC, HAND_KA_INC, HAND_HI_INC, 
};
#ifdef USAPYON2
const int Hand::SHIFTS[HI + 1] = {
	0, HAND_FU_SHIFT,HAND_KY_SHIFT,HAND_KE_SHIFT,HAND_GI_SHIFT,HAND_KI_SHIFT,HAND_KA_SHIFT,HAND_HI_SHIFT
};
const int Hand::MASKS[HI + 1] = {
	0,HAND_FU_MASK,HAND_KY_MASK,HAND_KE_MASK,HAND_GI_MASK,HAND_KI_MASK,HAND_KA_MASK,HAND_HI_MASK
};
#endif
#if defined(ENABLE_MYASSERT)
int debug_level;
#endif

#if !defined(NDEBUG)
#define MOVE_TRACE		// �ǖʂɎ������w�����ێ�����
#endif
#if defined(MOVE_TRACE)
Move m_trace[MAX_PLY +2];
void disp_trace(int n)
{
	for (int i = 0; i < n; i++) {
		std::cerr << i << ":" << move_to_csa(m_trace[i]) << " ";
	}
}
#endif

namespace NanohaTbl {
	// �����̒�`
	const int Direction[32] = {
		DIR00, DIR01, DIR02, DIR03, DIR04, DIR05, DIR06, DIR07,
		DIR08, DIR09, DIR10, DIR11, 0,     0,     0,     0,
		DIR00, DIR01, DIR02, DIR03, DIR04, DIR05, DIR06, DIR07,
		DIR00, DIR01, DIR02, DIR03, DIR04, DIR05, DIR06, DIR07,
	};

#if !defined(TSUMESOLVER)
	// ��̉��l
	const int KomaValue[32] = {
		 0,
		 DPawn,
		 DLance,
		 DKnight,
		 DSilver,
		 DGold,
		 DBishop,
		 DRook,
		 DKing,
		 DProPawn,
		 DProLance,
		 DProKnight,
		 DProSilver,
		 0,
		 DHorse,
		 DDragon,
		 0,
		-DPawn,
		-DLance,
		-DKnight,
		-DSilver,
		-DGold,
		-DBishop,
		-DRook,
		-DKing,
		-DProPawn,
		-DProLance,
		-DProKnight,
		-DProSilver,
		-0,
		-DHorse,
		-DDragon,
	};

	// ���ꂽ�Ƃ�(�ߊl���ꂽ�Ƃ�)�̉��l
	const int KomaValueEx[32] = {
		 0 + 0,
		 DPawn + DPawn,
		 DLance + DLance,
		 DKnight + DKnight,
		 DSilver + DSilver,
		 DGold + DGold,
		 DBishop + DBishop,
		 DRook + DRook,
		 DKing + DKing,
		 DProPawn + DPawn,
		 DProLance + DLance,
		 DProKnight + DKnight,
		 DProSilver + DSilver,
		 0 + 0,
		 DHorse + DBishop,
		 DDragon + DRook,
		 0 + 0,
		-DPawn	-DPawn,
		-DLance	-DLance,
		-DKnight	-DKnight,
		-DSilver	-DSilver,
		-DGold	-DGold,
		-DBishop	-DBishop,
		-DRook	-DRook,
		-DKing	-DKing,
		-DProPawn	-DPawn,
		-DProLance	-DLance,
		-DProKnight	-DKnight,
		-DProSilver	-DSilver,
		-0	-0,
		-DHorse	-DBishop,
		-DDragon	-DRook,
	};

	// ���鉿�l
	const int KomaValuePro[32] = {
		 0,
		 DProPawn - DPawn,
		 DProLance - DLance,
		 DProKnight - DKnight,
		 DProSilver - DSilver,
		 0,
		 DHorse - DBishop,
		 DDragon - DRook,
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
		-(DProPawn - DPawn),
		-(DProLance - DLance),
		-(DProKnight - DKnight),
		-(DProSilver - DSilver),
		 0,
		-(DHorse - DBishop),
		-(DDragon - DRook),
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
		 0,
	};
#endif//#if !defined(TSUMESOLVER)

	// History��index�ϊ��p
	const int Piece2Index[32] = {	// ��̎�ނɕϊ�����({�ƁA�ǁA�\�A�S}�����Ɠ��ꎋ)
		EMP, SFU, SKY, SKE, SGI, SKI, SKA, SHI,
		SOU, SKI, SKI, SKI, SKI, EMP, SUM, SRY,
		EMP, GFU, GKY, GKE, GGI, GKI, GKA, GHI,
		GOU, GKI, GKI, GKI, GKI, EMP, GUM, GRY,
	};
};

static FILE *fp_info = stdout;
static FILE *fp_log = NULL;

int output_info(const char *fmt, ...)
{
	int ret = 0;

	va_list argp;
	va_start(argp, fmt);
	if (fp_log) vfprintf(fp_log, fmt, argp);
#if !defined(USE_USI)
	if (fp_info) {
		ret = vfprintf(fp_info, fmt, argp);
	}
#endif
	va_end(argp);

	return ret;
}
int foutput_log(FILE *fp, const char *fmt, ...)
{
	int ret = 0;

	va_list argp;
	va_start(argp, fmt);
	if (fp == stdout) {
		if (fp_log) vfprintf(fp_log, fmt, argp);
	}
	if (fp) ret = vfprintf(fp, fmt, argp);
	va_end(argp);

	return ret;
}

// ���s�t�@�C���N�����ɍs��������.
void init_application_once()
{
	Position::init_evaluate();	// �]���x�N�g���̓ǂݍ���
	Position::initMate1ply();

	// ��Ճt�@�C���̓ǂݍ���
#ifdef USAPYON2
	if (book[0] == NULL) {
		book[0] = new Book();
		book[0]->open(Options["BookFile"]);
	}
	if (book[1] == NULL) {
		book[1] = new Book();
		book[1]->open(Options["BookFileW"]);
	}
#else
	if (book == NULL) {
		book = new Book();
		book->open(Options["BookFile"]);
	}
#endif
	int from;
	int to;
	int i;
	memset(Position::DirTbl, 0, sizeof(Position::DirTbl));
	for (from = 0x11; from <= 0x99; from++) {
		if ((from & 0x0F) == 0 || (from & 0x0F) > 9) continue;
		for (i = 0; i < 8; i++) {
			int dir = NanohaTbl::Direction[i];
			to = from;
			while (1) {
				to += dir;
				if ((to & 0x0F) == 0 || (to & 0x0F) >    9) break;
				if ((to & 0xF0) == 0 || (to & 0xF0) > 0x90) break;
				Position::DirTbl[from][to] = (1 << i);
			}
		}
	}
}

// �������֌W
void Position::init_position(const unsigned char board_ori[9][9], const int Mochigoma_ori[])
{
//	Tesu = 0;

	size_t i;
	unsigned char board[9][9];
	int Mochigoma[GOTE+HI+1] = {0};
	{
		int x, y;
		for (y = 0; y < 9; y++) {
			for (x = 0; x < 9; x++) {
				board[y][x] = board_ori[y][x];
			}
		}
		memcpy(Mochigoma, Mochigoma_ori, sizeof(Mochigoma));

		// ������ݒ�B
		handS.set(&Mochigoma[SENTE]);
		handG.set(&Mochigoma[GOTE]);
	}

	// �Ֆʂ�WALL�i�ǁj�Ŗ��߂Ă����܂��B
	for (i = 0; i < sizeof(banpadding)/sizeof(banpadding[0]); i++) {
		banpadding[i] = WALL;
	}
	for (i = 0; i < sizeof(ban)/sizeof(ban[0]); i++) {
		ban[i] = WALL;
	}

	// ������
	memset(komano, 0, sizeof(komano));
	memset(knkind, 0, sizeof(knkind));
	memset(knpos,  0, sizeof(knpos));

	// board�ŗ^����ꂽ�ǖʂ�ݒ肵�܂��B
	int z;
	int kn;
	for(int dan = 1; dan <= 9; dan++) {
		for(int suji = 0x10; suji <= 0x90; suji += 0x10) {
			// �����̋؂͍�����E�Ȃ̂ŁA�z��̐錾�Ƌt�ɂȂ邽�߁A�؂͂Ђ�����Ԃ��Ȃ��ƂȂ�܂���B
			z = suji + dan;
			ban[z] = Piece(board[dan-1][9 - suji/0x10]);

			// ��ԍ��n�̃f�[�^�ݒ�
#define KNABORT()	fprintf(stderr, "Error!:%s:%d:ban[0x%X] == 0x%X\n", __FILE__, __LINE__, z, ban[z]);exit(-1)
#define KNSET(kind) for (kn = KNS_##kind; kn <= KNE_##kind; kn++) {		\
						if (knkind[kn] == 0) break;		\
					}		\
					if (kn > KNE_##kind) {KNABORT();}		\
					knkind[kn] = ban[z];			\
					knpos[kn] = z;					\
					komano[z] = kn;

			switch (ban[z]) {
			case EMP:
				break;
			case SFU:
			case STO:
			case GFU:
			case GTO:
				KNSET(FU);
				break;
			case SKY:
			case SNY:
			case GKY:
			case GNY:
				KNSET(KY);
				break;
			case SKE:
			case SNK:
			case GKE:
			case GNK:
				KNSET(KE);
				break;
			case SGI:
			case SNG:
			case GGI:
			case GNG:
				KNSET(GI);
				break;
			case SKI:
			case GKI:
				KNSET(KI);
				break;
			case SKA:
			case SUM:
			case GKA:
			case GUM:
				KNSET(KA);
				break;
			case SHI:
			case SRY:
			case GHI:
			case GRY:
				KNSET(HI);
				break;
			case SOU:
				KNSET(SOU);
				break;
			case GOU:
				KNSET(GOU);
				break;
			case WALL:
			case PIECE_NONE:
			default:
				KNABORT();
				break;
			}
#undef KNABORT
#undef KNSET

		}
	}

#define KNABORT(kind)	fprintf(stderr, "Error!:%s:%d:kind=%d\n", __FILE__, __LINE__, kind);exit(-1)
#define KNHANDSET(SorG, kind) \
		for (kn = KNS_##kind; kn <= KNE_##kind; kn++) {		\
			if (knkind[kn] == 0) break;		\
		}		\
		if (kn > KNE_##kind) {KNABORT(kind);}		\
		knkind[kn] = SorG | kind;			\
		knpos[kn] = (SorG == SENTE) ? 1 : 2;

	int n;
	n = Mochigoma[SENTE+FU];	while (n--) {KNHANDSET(SENTE, FU)}
	n = Mochigoma[SENTE+KY];	while (n--) {KNHANDSET(SENTE, KY)}
	n = Mochigoma[SENTE+KE];	while (n--) {KNHANDSET(SENTE, KE)}
	n = Mochigoma[SENTE+GI];	while (n--) {KNHANDSET(SENTE, GI)}
	n = Mochigoma[SENTE+KI];	while (n--) {KNHANDSET(SENTE, KI)}
	n = Mochigoma[SENTE+KA];	while (n--) {KNHANDSET(SENTE, KA)}
	n = Mochigoma[SENTE+HI];	while (n--) {KNHANDSET(SENTE, HI)}

	n = Mochigoma[GOTE+FU];	while (n--) {KNHANDSET(GOTE, FU)}
	n = Mochigoma[GOTE+KY];	while (n--) {KNHANDSET(GOTE, KY)}
	n = Mochigoma[GOTE+KE];	while (n--) {KNHANDSET(GOTE, KE)}
	n = Mochigoma[GOTE+GI];	while (n--) {KNHANDSET(GOTE, GI)}
	n = Mochigoma[GOTE+KI];	while (n--) {KNHANDSET(GOTE, KI)}
	n = Mochigoma[GOTE+KA];	while (n--) {KNHANDSET(GOTE, KA)}
	n = Mochigoma[GOTE+HI];	while (n--) {KNHANDSET(GOTE, HI)}
#undef KNABORT
#undef KNHANDSET

	// effectB/effectW�̏�����
	init_effect();

	// �s�����̏�����
	make_pin_info();
}

// �s���̏�Ԃ�ݒ肷��
void Position::make_pin_info()
{
	memset(pin+0x11, 0, (0x99-0x11+1)*sizeof(pin[0]));
	int p;
#define ADDKING1(SG, dir) \
	do {				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		if (ban[p -= DIR_ ## dir] != EMP) break;				\
		p -= DIR_ ## dir;										\
	} while (0)
#define ADDKING2(SG, dir) 

	if (kingS) {	//���ʂ��Ֆʂɂ��鎞�̂ݗL��
#define SetPinS(dir)	\
		p = kingS;		\
		ADDKING1(S, dir);	\
		if (ban[p] != WALL) {	\
			if ((ban[p] & GOTE) == 0) {		\
				if (effectW[p] & (EFFECT_ ## dir << EFFECT_LONG_SHIFT)) pin[p] = DIR_ ## dir;		\
			}		\
			ADDKING2(S, dir);	\
		}

		SetPinS(UP);
		SetPinS(UL);
		SetPinS(UR);
		SetPinS(LEFT);
		SetPinS(RIGHT);
		SetPinS(DL);
		SetPinS(DR);
		SetPinS(DOWN);
#undef SetPinS
	}
	if (kingG) {	//�G�ʂ��Ֆʂɂ��鎞�̂ݗL��
#define SetPinG(dir)	\
		p = kingG;		\
		ADDKING1(G, dir);	\
		if (ban[p] != WALL) {		\
			if ((ban[p] & GOTE) != 0) {		\
				if (effectB[p] & (EFFECT_ ## dir << EFFECT_LONG_SHIFT)) pin[p] = DIR_ ## dir;		\
			}	\
			ADDKING2(G, dir);	\
		}

		SetPinG(DOWN);
		SetPinG(DL);
		SetPinG(DR);
		SetPinG(RIGHT);
		SetPinG(LEFT);
		SetPinG(UL);
		SetPinG(UR);
		SetPinG(UP);
#undef SetPinG
	}
#undef ADDKING1
#undef ADDKING2
}

// �����֌W
// effectB/effectW�̏�����
void Position::init_effect()
{
	int dan, suji;

	memset(effect, 0, sizeof(effect));

	for (suji = 0x10; suji <= 0x90; suji += 0x10) {
		for (dan = 1 ; dan <= 9 ; dan++) {
			add_effect(suji + dan);
		}
	}
}

void Position::add_effect(const int z)
{
#define ADD_EFFECT(turn,dir) zz = z + DIR_ ## dir; effect[turn][zz] |= EFFECT_ ## dir;

	int zz;

	switch (ban[z]) {

	case EMP:	break;
	case SFU:
		ADD_EFFECT(BLACK, UP);
		break;
	case SKY:
		AddKikiDirS(z, DIR_UP, EFFECT_UP << EFFECT_LONG_SHIFT);
		break;
	case SKE:
		ADD_EFFECT(BLACK, KEUR);
		ADD_EFFECT(BLACK, KEUL);
		break;
	case SGI:
		ADD_EFFECT(BLACK, UP);
		ADD_EFFECT(BLACK, UR);
		ADD_EFFECT(BLACK, UL);
		ADD_EFFECT(BLACK, DR);
		ADD_EFFECT(BLACK, DL);
		break;
	case SKI:
	case STO:
	case SNY:
	case SNK:
	case SNG:
		ADD_EFFECT(BLACK, UP);
		ADD_EFFECT(BLACK, UR);
		ADD_EFFECT(BLACK, UL);
		ADD_EFFECT(BLACK, RIGHT);
		ADD_EFFECT(BLACK, LEFT);
		ADD_EFFECT(BLACK, DOWN);
		break;
	case SUM:
		ADD_EFFECT(BLACK, UP);
		ADD_EFFECT(BLACK, RIGHT);
		ADD_EFFECT(BLACK, LEFT);
		ADD_EFFECT(BLACK, DOWN);
		// �p�Ɠ���������ǉ����邽�� break ���Ȃ�
	case SKA:
		AddKikiDirS(z, DIR_UR, EFFECT_UR << EFFECT_LONG_SHIFT);
		AddKikiDirS(z, DIR_UL, EFFECT_UL << EFFECT_LONG_SHIFT);
		AddKikiDirS(z, DIR_DR, EFFECT_DR << EFFECT_LONG_SHIFT);
		AddKikiDirS(z, DIR_DL, EFFECT_DL << EFFECT_LONG_SHIFT);
		break;
	case SRY:
		ADD_EFFECT(BLACK, UR);
		ADD_EFFECT(BLACK, UL);
		ADD_EFFECT(BLACK, DR);
		ADD_EFFECT(BLACK, DL);
		// ��Ɠ���������ǉ����邽�� break ���Ȃ�
	case SHI:
		AddKikiDirS(z, DIR_UP,    EFFECT_UP    << EFFECT_LONG_SHIFT);
		AddKikiDirS(z, DIR_DOWN,  EFFECT_DOWN  << EFFECT_LONG_SHIFT);
		AddKikiDirS(z, DIR_LEFT,  EFFECT_LEFT  << EFFECT_LONG_SHIFT);
		AddKikiDirS(z, DIR_RIGHT, EFFECT_RIGHT << EFFECT_LONG_SHIFT);
		break;
	case SOU:
		ADD_EFFECT(BLACK, UP);
		ADD_EFFECT(BLACK, UR);
		ADD_EFFECT(BLACK, UL);
		ADD_EFFECT(BLACK, RIGHT);
		ADD_EFFECT(BLACK, LEFT);
		ADD_EFFECT(BLACK, DOWN);
		ADD_EFFECT(BLACK, DR);
		ADD_EFFECT(BLACK, DL);
		break;

	case GFU:
		ADD_EFFECT(WHITE, DOWN);
		break;
	case GKY:
		AddKikiDirG(z, DIR_DOWN, EFFECT_DOWN << EFFECT_LONG_SHIFT);
		break;
	case GKE:
		ADD_EFFECT(WHITE, KEDR);
		ADD_EFFECT(WHITE, KEDL);
		break;
	case GGI:
		ADD_EFFECT(WHITE, DOWN);
		ADD_EFFECT(WHITE, DR);
		ADD_EFFECT(WHITE, DL);
		ADD_EFFECT(WHITE, UR);
		ADD_EFFECT(WHITE, UL);
		break;
	case GKI:
	case GTO:
	case GNY:
	case GNK:
	case GNG:
		ADD_EFFECT(WHITE, DOWN);
		ADD_EFFECT(WHITE, DR);
		ADD_EFFECT(WHITE, DL);
		ADD_EFFECT(WHITE, RIGHT);
		ADD_EFFECT(WHITE, LEFT);
		ADD_EFFECT(WHITE, UP);
		break;
	case GUM:
		ADD_EFFECT(WHITE, DOWN);
		ADD_EFFECT(WHITE, RIGHT);
		ADD_EFFECT(WHITE, LEFT);
		ADD_EFFECT(WHITE, UP);
		// �p�Ɠ���������ǉ����邽�� break ���Ȃ�
	case GKA:
		AddKikiDirG(z, DIR_DR, EFFECT_DR << EFFECT_LONG_SHIFT);
		AddKikiDirG(z, DIR_DL, EFFECT_DL << EFFECT_LONG_SHIFT);
		AddKikiDirG(z, DIR_UR, EFFECT_UR << EFFECT_LONG_SHIFT);
		AddKikiDirG(z, DIR_UL, EFFECT_UL << EFFECT_LONG_SHIFT);
		break;
	case GRY:
		ADD_EFFECT(WHITE, DR);
		ADD_EFFECT(WHITE, DL);
		ADD_EFFECT(WHITE, UR);
		ADD_EFFECT(WHITE, UL);
		// ��Ɠ���������ǉ����邽�� break ���Ȃ�
	case GHI:
		AddKikiDirG(z, DIR_DOWN,  EFFECT_DOWN  << EFFECT_LONG_SHIFT);
		AddKikiDirG(z, DIR_UP,    EFFECT_UP    << EFFECT_LONG_SHIFT);
		AddKikiDirG(z, DIR_RIGHT, EFFECT_RIGHT << EFFECT_LONG_SHIFT);
		AddKikiDirG(z, DIR_LEFT,  EFFECT_LEFT  << EFFECT_LONG_SHIFT);
		break;
	case GOU:
		ADD_EFFECT(WHITE, DOWN);
		ADD_EFFECT(WHITE, DR);
		ADD_EFFECT(WHITE, DL);
		ADD_EFFECT(WHITE, RIGHT);
		ADD_EFFECT(WHITE, LEFT);
		ADD_EFFECT(WHITE, UP);
		ADD_EFFECT(WHITE, UR);
		ADD_EFFECT(WHITE, UL);
		break;

	case WALL:
	case PIECE_NONE:
	default:__assume(0);
		break;
	}
}

void Position::del_effect(const int z, const Piece kind)
{
#define DEL_EFFECT(turn,dir) zz = z + DIR_ ## dir; effect[turn][zz] &= ~(EFFECT_ ## dir);

	int zz;
	switch (kind) {

	case EMP: break;

	case SFU:
		DEL_EFFECT(BLACK, UP);
		break;
	case SKY:
		DelKikiDirS(z, DIR_UP, ~(EFFECT_UP << EFFECT_LONG_SHIFT));
		break;
	case SKE:
		DEL_EFFECT(BLACK, KEUR);
		DEL_EFFECT(BLACK, KEUL);
		break;
	case SGI:
		DEL_EFFECT(BLACK, UP);
		DEL_EFFECT(BLACK, UR);
		DEL_EFFECT(BLACK, UL);
		DEL_EFFECT(BLACK, DR);
		DEL_EFFECT(BLACK, DL);
		break;
	case SKI:
	case STO:
	case SNY:
	case SNK:
	case SNG:
		DEL_EFFECT(BLACK, UP);
		DEL_EFFECT(BLACK, UR);
		DEL_EFFECT(BLACK, UL);
		DEL_EFFECT(BLACK, RIGHT);
		DEL_EFFECT(BLACK, LEFT);
		DEL_EFFECT(BLACK, DOWN);
		break;
	case SUM:
		DEL_EFFECT(BLACK, UP);
		DEL_EFFECT(BLACK, RIGHT);
		DEL_EFFECT(BLACK, LEFT);
		DEL_EFFECT(BLACK, DOWN);
		// �p�Ɠ����������폜���邽�� break ���Ȃ�
	case SKA:
		DelKikiDirS(z, DIR_UR, ~(EFFECT_UR << EFFECT_LONG_SHIFT));
		DelKikiDirS(z, DIR_UL, ~(EFFECT_UL << EFFECT_LONG_SHIFT));
		DelKikiDirS(z, DIR_DR, ~(EFFECT_DR << EFFECT_LONG_SHIFT));
		DelKikiDirS(z, DIR_DL, ~(EFFECT_DL << EFFECT_LONG_SHIFT));
		break;
	case SRY:
		DEL_EFFECT(BLACK, UR);
		DEL_EFFECT(BLACK, UL);
		DEL_EFFECT(BLACK, DR);
		DEL_EFFECT(BLACK, DL);
		// ��Ɠ����������폜���邽�� break ���Ȃ�
	case SHI:
		DelKikiDirS(z, DIR_UP,    ~(EFFECT_UP    << EFFECT_LONG_SHIFT));
		DelKikiDirS(z, DIR_DOWN,  ~(EFFECT_DOWN  << EFFECT_LONG_SHIFT));
		DelKikiDirS(z, DIR_LEFT,  ~(EFFECT_LEFT  << EFFECT_LONG_SHIFT));
		DelKikiDirS(z, DIR_RIGHT, ~(EFFECT_RIGHT << EFFECT_LONG_SHIFT));
		break;
	case SOU:
		DEL_EFFECT(BLACK, UP);
		DEL_EFFECT(BLACK, UR);
		DEL_EFFECT(BLACK, UL);
		DEL_EFFECT(BLACK, RIGHT);
		DEL_EFFECT(BLACK, LEFT);
		DEL_EFFECT(BLACK, DOWN);
		DEL_EFFECT(BLACK, DR);
		DEL_EFFECT(BLACK, DL);
		break;

	case GFU:
		DEL_EFFECT(WHITE, DOWN);
		break;
	case GKY:
		DelKikiDirG(z, DIR_DOWN, ~(EFFECT_DOWN << EFFECT_LONG_SHIFT));
		break;
	case GKE:
		DEL_EFFECT(WHITE, KEDR);
		DEL_EFFECT(WHITE, KEDL);
		break;
	case GGI:
		DEL_EFFECT(WHITE, DOWN);
		DEL_EFFECT(WHITE, DR);
		DEL_EFFECT(WHITE, DL);
		DEL_EFFECT(WHITE, UR);
		DEL_EFFECT(WHITE, UL);
		break;
	case GKI:
	case GTO:
	case GNY:
	case GNK:
	case GNG:
		DEL_EFFECT(WHITE, DOWN);
		DEL_EFFECT(WHITE, DR);
		DEL_EFFECT(WHITE, DL);
		DEL_EFFECT(WHITE, RIGHT);
		DEL_EFFECT(WHITE, LEFT);
		DEL_EFFECT(WHITE, UP);
		break;
	case GUM:
		DEL_EFFECT(WHITE, UP);
		DEL_EFFECT(WHITE, RIGHT);
		DEL_EFFECT(WHITE, LEFT);
		DEL_EFFECT(WHITE, DOWN);
		// �p�Ɠ����������폜���邽�� break ���Ȃ�
	case GKA:
		DelKikiDirG(z, DIR_UR, ~(EFFECT_UR << EFFECT_LONG_SHIFT));
		DelKikiDirG(z, DIR_UL, ~(EFFECT_UL << EFFECT_LONG_SHIFT));
		DelKikiDirG(z, DIR_DR, ~(EFFECT_DR << EFFECT_LONG_SHIFT));
		DelKikiDirG(z, DIR_DL, ~(EFFECT_DL << EFFECT_LONG_SHIFT));
		break;
	case GRY:
		DEL_EFFECT(WHITE, UR);
		DEL_EFFECT(WHITE, UL);
		DEL_EFFECT(WHITE, DR);
		DEL_EFFECT(WHITE, DL);
		// ��Ɠ����������폜���邽�� break ���Ȃ�
	case GHI:
		DelKikiDirG(z, DIR_UP,    ~(EFFECT_UP    << EFFECT_LONG_SHIFT));
		DelKikiDirG(z, DIR_DOWN,  ~(EFFECT_DOWN  << EFFECT_LONG_SHIFT));
		DelKikiDirG(z, DIR_LEFT,  ~(EFFECT_LEFT  << EFFECT_LONG_SHIFT));
		DelKikiDirG(z, DIR_RIGHT, ~(EFFECT_RIGHT << EFFECT_LONG_SHIFT));
		break;
	case GOU:
		DEL_EFFECT(WHITE, DOWN);
		DEL_EFFECT(WHITE, DR);
		DEL_EFFECT(WHITE, DL);
		DEL_EFFECT(WHITE, RIGHT);
		DEL_EFFECT(WHITE, LEFT);
		DEL_EFFECT(WHITE, UP);
		DEL_EFFECT(WHITE, UR);
		DEL_EFFECT(WHITE, UL);
		break;

	case WALL:
	case PIECE_NONE:
	default:__assume(0);
		break;
	}
}


/// Position::do_move() �͎��i�߂�B�����ĕK�v�Ȃ��ׂĂ̏��� StateInfo �I�u�W�F�N�g�ɕۑ�����B
/// ��͍��@�ł��邱�Ƃ�O��Ƃ��Ă���BPseudo-legal�Ȏ�͂��̊֐����ĂԑO�Ɏ�菜���K�v������B

/// Position::do_move() makes a move, and saves all information necessary
/// to a StateInfo object. The move is assumed to be legal. Pseudo-legal
/// moves should be filtered out before this function is called.

void Position::do_move(Move m, StateInfo& newSt,int count)
{
#ifdef NANOHA
	assert(pos_is_ok());
#else
	assert(is_ok());
#endif
	assert(&newSt != st);
	assert(!at_checking());
#if defined(MOVE_TRACE)
	m_trace[st->gamePly] = m;
	assert(m != MOVE_NULL);	// NullMove��do_null_move()�ŏ�������
#endif

	nodes+=count;
	Key key = st->key;

	// Copy some fields of old state to our new StateInfo object except the
	// ones which are recalculated from scratch anyway, then switch our state
	// pointer to point to the new, ready to be updated, state.
	/// �� position.cpp �� struct StateInfo �ƒ�`�����킹��
	struct ReducedStateInfo {
		int gamePly;
		int pliesFromNull;
		Piece captured;
		uint32_t hand;
		uint32_t effect;
		Key key;
	};

	memcpy(&newSt, st, sizeof(ReducedStateInfo));

	newSt.previous = st;
	st = &newSt;

	// Update side to move
	key ^= zobSideToMove;

	// Increment the 50 moves rule draw counter. Resetting it to zero in the
	// case of non-reversible moves is taken care of later.
	st->pliesFromNull++;

	const Color us = side_to_move();
	if (move_is_drop(m))
	{
		st->key = key;
		do_drop(m);
		st->hand = hand[us].h;
		st->effect = (us == BLACK) ? effectB[kingG] : effectW[kingS];
		assert(!at_checking());
		assert(Position::key() == compute_key());
		return;
	}

	const Square from = move_from(m);
	const Square to = move_to(m);
	bool pm = is_promotion(m);

	Piece piece = piece_on(from);
	Piece capture = piece_on(to);
	int kn;
	unsigned long id;
	unsigned long tkiki;

	assert(color_of(piece_on(from)) == us);
	assert(empty(to) || color_of(piece_on(to)) == flip(us));

	// �s�����̃N���A
	if (piece == SOU) {
		// ���ʂ𓮂���
		DelPinInfS(DIR_UP);
		DelPinInfS(DIR_DOWN);
		DelPinInfS(DIR_RIGHT);
		DelPinInfS(DIR_LEFT);
		DelPinInfS(DIR_UR);
		DelPinInfS(DIR_UL);
		DelPinInfS(DIR_DR);
		DelPinInfS(DIR_DL);
		if (EFFECT_KING_G(to) /*&& EFFECT_KING_G(to) == ((effectB[to] & EFFECT_LONG_MASK) >> EFFECT_LONG_SHIFT)*/) {
			_BitScanForward(&id, EFFECT_KING_G(to));
			DelPinInfG(NanohaTbl::Direction[id]);
		}
	} else if (piece == GOU) {
		// ���ʂ𓮂���
		DelPinInfG(DIR_UP);
		DelPinInfG(DIR_DOWN);
		DelPinInfG(DIR_RIGHT);
		DelPinInfG(DIR_LEFT);
		DelPinInfG(DIR_UR);
		DelPinInfG(DIR_UL);
		DelPinInfG(DIR_DR);
		DelPinInfG(DIR_DL);
		if (EFFECT_KING_S(to) /*&& EFFECT_KING_S(to) == ((effectW[to] & EFFECT_LONG_MASK) >> EFFECT_LONG_SHIFT)*/) {
			_BitScanForward(&id, EFFECT_KING_S(to));
			DelPinInfS(NanohaTbl::Direction[id]);
		}
	} else {
		if (us == BLACK) {
			// ����
			if (EFFECT_KING_S(from)) {
///				_BitScanForward(&id, EFFECT_KING_S(from));
///				DelPinInfS(NanohaTbl::Direction[id]);
				pin[from] = 0;
			}
			if (EFFECT_KING_S(to)/* && (effectW[to] & EFFECT_LONG_MASK)*/) {
				_BitScanForward(&id, EFFECT_KING_S(to));
				DelPinInfS(NanohaTbl::Direction[id]);
			}
///			if (DirTbl[kingG][from] == DirTbl[kingG][to]) {
///				pin[to] = 0;
///			} else 
			{
				if (EFFECT_KING_G(from)) {
					_BitScanForward(&id, EFFECT_KING_G(from));
					DelPinInfG(NanohaTbl::Direction[id]);
				}
				if (EFFECT_KING_G(to)) {
					_BitScanForward(&id, EFFECT_KING_G(to));
					DelPinInfG(NanohaTbl::Direction[id]);
				}
			}
		} else {
			// ����
///			if (DirTbl[kingS][from] == DirTbl[kingS][to]) {
///				pin[to] = 0;
///			} else 
			{
				if (EFFECT_KING_S(from)) {
					_BitScanForward(&id, EFFECT_KING_S(from));
					DelPinInfS(NanohaTbl::Direction[id]);
				}
				if (EFFECT_KING_S(to)) {
					_BitScanForward(&id, EFFECT_KING_S(to));
					DelPinInfS(NanohaTbl::Direction[id]);
				}
			}
			if (EFFECT_KING_G(from)) {
//				_BitScanForward(&id, EFFECT_KING_G(from));
//				DelPinInfG(NanohaTbl::Direction[id]);
				pin[from] = 0;
			}
			if (EFFECT_KING_G(to)/* && (effectB[to] & EFFECT_LONG_MASK)*/) {
				_BitScanForward(&id, EFFECT_KING_G(to));
				DelPinInfG(NanohaTbl::Direction[id]);
			}
		}
	}

	del_effect(from, piece);					// ��������̗���������
	st->capturedPiece = capture;				// StockFish10��Ver�ł͕K�{���ۂ�
	if (capture) {
		del_effect(to, capture);	// ����̗���������
		kn = komano[to];
		knkind[kn] = (capture ^ GOTE) & ~(PROMOTED);
		knpos[kn] = (us == BLACK) ? 1 : 2;
		if (us == BLACK) {
			int kind = capture & ~(GOTE | PROMOTED);
			handS.inc(kind);
#ifdef USAPYON2
			key ^= zobHand[kind][handS.getFromKind(kind)];
#endif
		} else {
			int kind = capture & ~(GOTE | PROMOTED);
			handG.inc(kind);
#ifdef USAPYON2
			key ^= zobHand[kind|GOTE][handG.getFromKind(kind)];
#endif
		}

#if !defined(TSUMESOLVER)
		// material �X�V
		material -= NanohaTbl::KomaValueEx[capture];
#endif//#if !defined(TSUMESOLVER)

		// �n�b�V���X�V
		key ^= zobrist[capture][to];
	} else {
		// �ړ���͋󁨈ړ���̒�������������
		// ���̗���������
		if ((tkiki = effectW[to] & EFFECT_LONG_MASK) != 0) {
			while (tkiki) {
				_BitScanForward(&id, tkiki);
				tkiki &= tkiki-1;
				DelKikiDirG(to, NanohaTbl::Direction[id], ~(1u << id));
			}
		}
		// ���̗���������
		if ((tkiki = effectB[to] & EFFECT_LONG_MASK) != 0) {
			while (tkiki) {
				_BitScanForward(&id, tkiki);
				tkiki &= tkiki-1;
				DelKikiDirS(to, NanohaTbl::Direction[id], ~(1u << id));
			}
		}
	}

	kn = komano[from];
	if (pm) {
#if !defined(TSUMESOLVER)
		// material �X�V
		material += NanohaTbl::KomaValuePro[piece];
#endif//#if !defined(TSUMESOLVER)

		piece = Piece(int(piece)|PROMOTED);
	}
	knkind[kn] = piece;
	knpos[kn] = to;

	// �n�b�V���X�V
	key ^= zobrist[ban[from]][from] ^ zobrist[piece][to];

	// Prefetch TT access as soon as we know key is updated
	prefetch(reinterpret_cast<char*>(TT.first_entry(key)));

	// Move the piece

	ban[to]   = piece;
	ban[from] = EMP;
	komano[to] = kn;
	komano[from] = 0;

	// �������X�V
	add_effect(to);

	// �ړ����̒���������L�΂�
	// ���̗���������
	if ((tkiki = effectW[from] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			AddKikiDirG(from, NanohaTbl::Direction[id], 1u << id);
		}
	}
	// ���̗���������
	if ((tkiki = effectB[from] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			AddKikiDirS(from, NanohaTbl::Direction[id], 1u << id);
		}
	}

	// �s�����̕t��
	if (piece == SOU) {
		AddPinInfS(DIR_UP);
		AddPinInfS(DIR_DOWN);
		AddPinInfS(DIR_RIGHT);
		AddPinInfS(DIR_LEFT);
		AddPinInfS(DIR_UR);
		AddPinInfS(DIR_UL);
		AddPinInfS(DIR_DR);
		AddPinInfS(DIR_DL);
		if (EFFECT_KING_G(from) /*&& (effectB[from] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_G(from));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(to) /*&& (effectB[from] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_G(to));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
	} else if (piece == GOU) {
		AddPinInfG(DIR_UP);
		AddPinInfG(DIR_DOWN);
		AddPinInfG(DIR_RIGHT);
		AddPinInfG(DIR_LEFT);
		AddPinInfG(DIR_UR);
		AddPinInfG(DIR_UL);
		AddPinInfG(DIR_DR);
		AddPinInfG(DIR_DL);
		if (EFFECT_KING_S(from) /*&& (effectW[from] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_S(from));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_S(to) /*&& (effectW[from] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_S(to));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
	} else {
		if (EFFECT_KING_S(from)) {
			_BitScanForward(&id, EFFECT_KING_S(from));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_S(to)) {
			_BitScanForward(&id, EFFECT_KING_S(to));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(from)) {
			_BitScanForward(&id, EFFECT_KING_G(from));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(to)) {
			_BitScanForward(&id, EFFECT_KING_G(to));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
	}

	// Set capture piece
	st->captured = capture;

	// Update the key with the final value
	st->key = key;
	st->hand = hand[us].h;
	st->effect = (us == BLACK) ? effectB[kingG] : effectW[kingS];

#if !defined(NDEBUG)
	// ����w�������ƂɁA����ɂȂ��Ă���ˎ��E��ɂȂ��Ă���
	if (in_check()) {
		print_csa(m);
		disp_trace(st->gamePly + 1);
		MYABORT();
	}
#endif

	// Finish
	sideToMove = flip(sideToMove);

#if defined(MOVE_TRACE)
	int fail;
	if (pos_is_ok(&fail) == false) {
		std::cerr << "Error!:is_ok() is false. Reason code = " << fail << std::endl;
		print_csa(m);
	}
#else
	assert(is_ok());
#endif
	assert(Position::key() == compute_key());
}

void Position::do_drop(Move m)
{
	const Color us = side_to_move();
	const Square to = move_to(m);

	assert(empty(to));

	Piece piece = move_piece(m);
	int kn = 0x80;
	int kne = 0;
	unsigned long id;
	unsigned long tkiki;

	// �s�����̃N���A
	if (EFFECT_KING_S(to)/* && EFFECT_KING_S(to) == ((effectW[to] & EFFECT_LONG_MASK) >> EFFECT_LONG_SHIFT)*/) {
		_BitScanForward(&id, EFFECT_KING_S(to));
		DelPinInfS(NanohaTbl::Direction[id]);
	}
	if (EFFECT_KING_G(to)/* && EFFECT_KING_G(to) == ((effectB[to] & EFFECT_LONG_MASK) >> EFFECT_LONG_SHIFT)*/) {
		_BitScanForward(&id, EFFECT_KING_G(to));
		DelPinInfG(NanohaTbl::Direction[id]);
	}

	// �ړ���͋󁨈ړ���̒�������������
	// ���̗���������
	if ((tkiki = effectW[to] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			DelKikiDirG(to, NanohaTbl::Direction[id], ~(1u << id));
		}
	}
	// ���̗���������
	if ((tkiki = effectB[to] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			DelKikiDirS(to, NanohaTbl::Direction[id], ~(1u << id));
		}
	}

	unsigned int diff = 0;
	switch (piece & ~GOTE) {
	case EMP:
		break;
	case FU:
		kn  = KNS_FU;
		kne = KNE_FU;
		diff = HAND_FU_INC;
		break;
	case KY:
		kn  = KNS_KY;
		kne = KNE_KY;
		diff = HAND_KY_INC;
		break;
	case KE:
		kn  = KNS_KE;
		kne = KNE_KE;
		diff = HAND_KE_INC;
		break;
	case GI:
		kn  = KNS_GI;
		kne = KNE_GI;
		diff = HAND_GI_INC;
		break;
	case KI:
		kn  = KNS_KI;
		kne = KNE_KI;
		diff = HAND_KI_INC;
		break;
	case KA:
		kn  = KNS_KA;
		kne = KNE_KA;
		diff = HAND_KA_INC;
		break;
	case HI:
		kn  = KNS_HI;
		kne = KNE_HI;
		diff = HAND_HI_INC;
		break;
	default:
		break;
	}

	if (us == BLACK) {
#ifdef USAPYON2
		st->key ^= zobHand[piece][handS.getFromKind(piece & ~ GOTE)];
#endif
		handS.h -= diff;
		while (kn <= kne) {
			if (knpos[kn] == 1) break;
			kn++;
		}
	} else {
#ifdef USAPYON2
		st->key ^= zobHand[piece][handG.getFromKind(piece & ~GOTE)];
#endif
		handG.h -= diff;
		while (kn <= kne) {
			if (knpos[kn] == 2) break;
			kn++;
		}
	}

#if !defined(NDEBUG)
	// �G���[�̂Ƃ��� Die!
	if (kn > kne) {
		print_csa(m);
		MYABORT();
	}
#endif

	assert(color_of(piece) == us);

	knkind[kn] = piece;
	knpos[kn] = to;
	ban[to] = piece;
	komano[to] = kn;

	// �������X�V
	add_effect(to);

	// �ړ����A�ړ��悪�ʂ̉�������ɂ������Ƃ��ɂ����̃s������ǉ�����
	if (EFFECT_KING_S(to)) {
		_BitScanForward(&id, EFFECT_KING_S(to));
		AddPinInfS(NanohaTbl::Direction[id]);
	}
	if (EFFECT_KING_G(to)) {
		_BitScanForward(&id, EFFECT_KING_G(to));
		AddPinInfG(NanohaTbl::Direction[id]);
	}

	// Set capture piece
	st->captured = EMP;

	// Update the key with the final value
	st->key ^= zobrist[piece][to];

	// Prefetch TT access as soon as we know key is updated
	prefetch(reinterpret_cast<char*>(TT.first_entry(st->key)));

	// Finish
	sideToMove = flip(sideToMove);

	assert(pos_is_ok());
	assert(Position::key() == compute_key());
}

/// Position::undo_move() unmakes a move. When it returns, the position should
/// be restored to exactly the same state as before the move was made.

void Position::undo_move(Move m) {

#if defined(MOVE_TRACE)
	assert(m != MOVE_NULL);	// NullMove��undo_null_move()�ŏ�������
	int fail;
	if (pos_is_ok(&fail) == false) {
		disp_trace(st->gamePly+1);
		MYABORT();
	}
#else
	assert(is_ok());
#endif
	assert(is_ok(m));

	sideToMove = flip(sideToMove);

	if (move_is_drop(m))
	{
		undo_drop(m);
		return;
	}

	Color us = side_to_move();
	Square from = move_from(m);
	Square to = move_to(m);
	bool pm = is_promotion(m);
	Piece piece = move_piece(m);
	Piece captured = st->captured;
	int kn;
	unsigned long id;
	unsigned long tkiki;

	assert(empty(from));
	assert(color_of(piece_on(to)) == us);

	// �s�����̃N���A
	if (piece == SOU) {
		DelPinInfS(DIR_UP);
		DelPinInfS(DIR_DOWN);
		DelPinInfS(DIR_RIGHT);
		DelPinInfS(DIR_LEFT);
		DelPinInfS(DIR_UR);
		DelPinInfS(DIR_UL);
		DelPinInfS(DIR_DR);
		DelPinInfS(DIR_DL);
		if (EFFECT_KING_G(from) /*&& (effectB[from] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_G(from));
			DelPinInfG(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(to)) {
			_BitScanForward(&id, EFFECT_KING_G(to));
			DelPinInfG(NanohaTbl::Direction[id]);
		}
	} else if (piece == GOU) {
		DelPinInfG(DIR_UP);
		DelPinInfG(DIR_DOWN);
		DelPinInfG(DIR_RIGHT);
		DelPinInfG(DIR_LEFT);
		DelPinInfG(DIR_UR);
		DelPinInfG(DIR_UL);
		DelPinInfG(DIR_DR);
		DelPinInfG(DIR_DL);
		if (EFFECT_KING_S(from) /*&& (effectW[from] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_S(from));
			DelPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_S(to)) {
			_BitScanForward(&id, EFFECT_KING_S(to));
			DelPinInfS(NanohaTbl::Direction[id]);
		}
	} else {
		if (us == BLACK) {
			if (EFFECT_KING_S(from)) {
				_BitScanForward(&id, EFFECT_KING_S(from));
				DelPinInfS(NanohaTbl::Direction[id]);
			}
			if (EFFECT_KING_S(to)) {
				_BitScanForward(&id, EFFECT_KING_S(to));
				DelPinInfS(NanohaTbl::Direction[id]);
///				pin[to] = 0;
			}
			if (EFFECT_KING_G(from)) {
				_BitScanForward(&id, EFFECT_KING_G(from));
				DelPinInfG(NanohaTbl::Direction[id]);
			}
			if (EFFECT_KING_G(to)) {
				_BitScanForward(&id, EFFECT_KING_G(to));
				DelPinInfG(NanohaTbl::Direction[id]);
			}
		} else {
			if (EFFECT_KING_S(from)) {
				_BitScanForward(&id, EFFECT_KING_S(from));
				DelPinInfS(NanohaTbl::Direction[id]);
			}
			if (EFFECT_KING_S(to)) {
				_BitScanForward(&id, EFFECT_KING_S(to));
				DelPinInfS(NanohaTbl::Direction[id]);
			}
			if (EFFECT_KING_G(from)) {
				_BitScanForward(&id, EFFECT_KING_G(from));
				DelPinInfG(NanohaTbl::Direction[id]);
			}
			if (EFFECT_KING_G(to)) {
				_BitScanForward(&id, EFFECT_KING_G(to));
				DelPinInfG(NanohaTbl::Direction[id]);
///				pin[to] = 0;
			}
		}
	}

	del_effect(to, ban[to]);					// ����������̗���������

	kn = komano[to];
	if (pm) {
///		piece &= ~PROMOTED;

#if !defined(TSUMESOLVER)
		// material �X�V
		material -= NanohaTbl::KomaValuePro[piece];
#endif//#if !defined(TSUMESOLVER)
	}
	knkind[kn] = piece;
	knpos[kn] = from;

	ban[to] = captured;
	komano[from] = komano[to];
	ban[from] = piece;

	if (captured) {
#if !defined(TSUMESOLVER)
		// material �X�V
		material += NanohaTbl::KomaValueEx[captured];
#endif//#if !defined(TSUMESOLVER)

		int kne = 0;
		switch (captured & ~(GOTE|PROMOTED)) {
		case EMP:
			break;
		case FU:
			kn  = KNS_FU;
			kne = KNE_FU;
			break;
		case KY:
			kn  = KNS_KY;
			kne = KNE_KY;
			break;
		case KE:
			kn  = KNS_KE;
			kne = KNE_KE;
			break;
		case GI:
			kn  = KNS_GI;
			kne = KNE_GI;
			break;
		case KI:
			kn  = KNS_KI;
			kne = KNE_KI;
			break;
		case KA:
			kn  = KNS_KA;
			kne = KNE_KA;
			break;
		case HI:
			kn  = KNS_HI;
			kne = KNE_HI;
			break;
		default:
			break;
		}
	
		while (kn <= kne) {
			if (us == BLACK) {
				if (knpos[kn] == 1) break;
			} else {
				if (knpos[kn] == 2) break;
			}
			kn++;
		}
#if 0
		// �G���[�̂Ƃ��� Die!
		if (kn > kne) {
			Print();
			move_print(m);
			output_info(":kn=%d, kne=%d, capture=0x%X\n", kn, kne, captured);
			MYABORT();
		}
#endif
		knkind[kn] = captured;
		knpos[kn] = to;
		ban[to] = captured;
		komano[to] = kn;
		add_effect(to);	// �������̗�����ǉ�

		if (us == BLACK) {
			handS.dec(captured & ~(GOTE | PROMOTED));
		} else {
			handG.dec(captured & ~(GOTE | PROMOTED));
		}
	} else {
		// �ړ���͋󁨈ړ���̒���������ʂ�
		// ���̗���������
		if ((tkiki = effectW[to] & EFFECT_LONG_MASK) != 0) {
			while (tkiki) {
				_BitScanForward(&id, tkiki);
				tkiki &= tkiki-1;
				AddKikiDirG(to, NanohaTbl::Direction[id], 1u << id);
			}
		}
		// ���̗���������
		if ((tkiki = effectB[to] & EFFECT_LONG_MASK) != 0) {
			while (tkiki) {
				_BitScanForward(&id, tkiki);
				tkiki &= tkiki-1;
				AddKikiDirS(to, NanohaTbl::Direction[id], 1u << id);
			}
		}
		ban[to] = EMP;
		komano[to] = 0;
	}

	// �ړ����̒�����������������
	// ���̗���������
	if ((tkiki = effectW[from] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			DelKikiDirG(from, NanohaTbl::Direction[id], ~(1u << id));
			if (piece == SOU) {
				// ���������͋ʂ�������т�
				if (ban[from + NanohaTbl::Direction[id]] != WALL) effectW[from + NanohaTbl::Direction[id]] |= (1u << id);
			}
		}
	}
	// ���̗���������
	if ((tkiki = effectB[from] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			DelKikiDirS(from, NanohaTbl::Direction[id], ~(1u << id));
			if (piece == GOU) {
				// ���������͋ʂ�������т�
				if (ban[from + NanohaTbl::Direction[id]] != WALL) effectB[from + NanohaTbl::Direction[id]] |= (1u << id);
			}
		}
	}

	// �������X�V
	add_effect(from);

	// �s�����t��
	if (piece == SOU) {
		AddPinInfS(DIR_UP);
		AddPinInfS(DIR_DOWN);
		AddPinInfS(DIR_RIGHT);
		AddPinInfS(DIR_LEFT);
		AddPinInfS(DIR_UR);
		AddPinInfS(DIR_UL);
		AddPinInfS(DIR_DR);
		AddPinInfS(DIR_DL);
		if (EFFECT_KING_G(from)) {
			_BitScanForward(&id, EFFECT_KING_G(from));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(to) /*&& (effectB[to] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_G(to));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
	} else if (piece == GOU) {
		AddPinInfG(DIR_UP);
		AddPinInfG(DIR_DOWN);
		AddPinInfG(DIR_RIGHT);
		AddPinInfG(DIR_LEFT);
		AddPinInfG(DIR_UR);
		AddPinInfG(DIR_UL);
		AddPinInfG(DIR_DR);
		AddPinInfG(DIR_DL);
		if (EFFECT_KING_S(from)) {
			_BitScanForward(&id, EFFECT_KING_S(from));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_S(to) /*&& (effectW[to] & EFFECT_LONG_MASK)*/) {
			_BitScanForward(&id, EFFECT_KING_S(to));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
	} else {
		if (EFFECT_KING_S(from)) {
			_BitScanForward(&id, EFFECT_KING_S(from));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_S(to)) {
			_BitScanForward(&id, EFFECT_KING_S(to));
			AddPinInfS(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(from)) {
			_BitScanForward(&id, EFFECT_KING_G(from));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
		if (EFFECT_KING_G(to)) {
			_BitScanForward(&id, EFFECT_KING_G(to));
			AddPinInfG(NanohaTbl::Direction[id]);
		}
	}

	// Finally point our state pointer back to the previous state
	st = st->previous;

	assert(pos_is_ok());
	assert(Position::key() == compute_key());
}

void Position::undo_drop(Move m)
{
	Color us = side_to_move();
	Square to = move_to(m);
	Piece piece = move_piece(m);
	int kn = 0x80;
	int kne = 0;
	unsigned long id;
	unsigned long tkiki;

	assert(color_of(piece_on(to)) == us);

	// �ړ����A�ړ��悪�ʂ̉�������ɂ������Ƃ��ɂ����̃s�������폜����
	if (EFFECT_KING_S(to)) {
		_BitScanForward(&id, EFFECT_KING_S(to));
		DelPinInfS(NanohaTbl::Direction[id]);
	}
	if (EFFECT_KING_G(to)) {
		_BitScanForward(&id, EFFECT_KING_G(to));
		DelPinInfG(NanohaTbl::Direction[id]);
	}

	unsigned int diff = 0;
	switch (piece & ~GOTE) {
	case EMP:
		break;
	case FU:
		kn  = KNS_FU;
		kne = KNE_FU;
		diff = HAND_FU_INC;
		break;
	case KY:
		kn  = KNS_KY;
		kne = KNE_KY;
		diff = HAND_KY_INC;
		break;
	case KE:
		kn  = KNS_KE;
		kne = KNE_KE;
		diff = HAND_KE_INC;
		break;
	case GI:
		kn  = KNS_GI;
		kne = KNE_GI;
		diff = HAND_GI_INC;
		break;
	case KI:
		kn  = KNS_KI;
		kne = KNE_KI;
		diff = HAND_KI_INC;
		break;
	case KA:
		kn  = KNS_KA;
		kne = KNE_KA;
		diff = HAND_KA_INC;
		break;
	case HI:
		kn  = KNS_HI;
		kne = KNE_HI;
		diff = HAND_HI_INC;
		break;
	default:
		break;
	}

	while (kn <= kne) {
		if (knpos[kn] == to) break;
		kn++;
	}
#if 0
	// �G���[�̂Ƃ��� Die!
	if (kn > kne) {
		Print();
		move_print(m);
		output_info("\n");
		MYABORT();
	}
#endif

	knkind[kn] = piece;
	knpos[kn] = (us == BLACK) ? 1 : 2;
	ban[to] = EMP;
	komano[to] = 0;

	del_effect(to, piece);					// ����������̗���������

	// �ł����ʒu�̒���������ʂ�
	// ���̗�����ǉ�
	if ((tkiki = effectW[to] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			AddKikiDirG(to, NanohaTbl::Direction[id], 1u << id);
		}
	}
	// ���̗�����ǉ�
	if ((tkiki = effectB[to] & EFFECT_LONG_MASK) != 0) {
		while (tkiki) {
			_BitScanForward(&id, tkiki);
			tkiki &= tkiki-1;
			AddKikiDirS(to, NanohaTbl::Direction[id], 1u << id);
		}
	}

	// �ړ����A�ړ��悪�ʂ̉�������ɂ������Ƃ��ɂ����̃s������ǉ�����
	if (EFFECT_KING_S(to) /*&& EFFECT_KING_S(to) == ((effectW[to] & EFFECT_LONG_MASK) >> EFFECT_LONG_SHIFT)*/) {
		_BitScanForward(&id, EFFECT_KING_S(to));
		AddPinInfS(NanohaTbl::Direction[id]);
	}
	if (EFFECT_KING_G(to) /*&& EFFECT_KING_G(to) == ((effectB[to] & EFFECT_LONG_MASK) >> EFFECT_LONG_SHIFT)*/) {
		_BitScanForward(&id, EFFECT_KING_G(to));
		AddPinInfG(NanohaTbl::Direction[id]);
	}

	if (us == BLACK) handS.h += diff;
	else             handG.h += diff;

	// Finally point our state pointer back to the previous state
	st = st->previous;

	assert(pos_is_ok());
	assert(Position::key() == compute_key());
}


//�@�Ăяo����Ă��Ȃ��H
#ifndef USAPYON2
// ���i�߂��Ƀn�b�V���v�Z�̂ݍs��
uint64_t Position::calc_hash_no_move(const Move m) const
{
	uint64_t new_key = key();

	new_key ^= zobSideToMove;	// ��Ԕ��]

	// from �̌v�Z
	int from = move_from(m);
	int to = move_to(m);
	int piece = move_piece(m);
	if (!move_is_drop(m)) {
		// �󔒂ɂȂ������Ƃŕς��n�b�V���l
		new_key ^= zobrist[piece][from];
	} else {
		// ������̕ω��ɂ��n�b�V��
		// �{���K�v�ȃn�Y�����A�Ăяo����Ă��Ȃ��̂Ȃ�֌W�Ȃ��c�B
	}

	// to �̏���
	// ban[to]�ɂ��������̂��g�������������
	Piece capture = move_captured(m);
	if (capture) {
		new_key ^= zobrist[ban[to]][to];
		// ������̕ω��ɂ��n�b�V��
		// �{���K�v�ȃn�Y�����A�Ăяo����Ă��Ȃ��̂Ȃ�֌W�Ȃ��c�B
	}

	// �V��������g�������ɉ�����
	if (is_promotion(m)) piece |= PROMOTED;
	new_key ^= zobrist[piece][to];

	return new_key;
}
#endif

// �w����`�F�b�N�n
// �w���肪���肩�ǂ����`�F�b�N����
bool Position::is_check_move(const Color us, Move m) const
{
	const Square kPos = (us == BLACK) ? Square(kingG) : Square(kingS);	// ���葤�̋ʂ̈ʒu

	return move_attacks_square(m, kPos);
}

bool Position::move_attacks_square(Move m, Square kPos) const
{
	const Color us = side_to_move();
	const effect_t *akiki = (us == BLACK) ? effectB : effectW;	// �����̗���
	const Piece piece = is_promotion(m) ? Piece(move_piece(m) | PROMOTED) : move_piece(m);
	const Square to = move_to(m);

	switch (piece) {
	case EMP:break;
	case SFU:
		if (to + DIR_UP == kPos) return true;
		break;
	case SKY:
		if (DirTbl[to][kPos] == EFFECT_UP) {
			if (SkipOverEMP(to, DIR_UP) == kPos) return true;
		}
		break;
	case SKE:
		if (to + DIR_KEUR == kPos) return true;
		if (to + DIR_KEUL == kPos) return true;
		break;
	case SGI:
		if (to + DIR_UP == kPos) return true;
		if (to + DIR_UR == kPos) return true;
		if (to + DIR_UL == kPos) return true;
		if (to + DIR_DR == kPos) return true;
		if (to + DIR_DL == kPos) return true;
		break;
	case SKI:
	case STO:
	case SNY:
	case SNK:
	case SNG:
		if (to + DIR_UP    == kPos) return true;
		if (to + DIR_UR    == kPos) return true;
		if (to + DIR_UL    == kPos) return true;
		if (to + DIR_RIGHT == kPos) return true;
		if (to + DIR_LEFT  == kPos) return true;
		if (to + DIR_DOWN  == kPos) return true;
		break;

	case GFU:
		if (to + DIR_DOWN == kPos) return true;
		break;
	case GKY:
		if (DirTbl[to][kPos] == EFFECT_DOWN) {
			if (SkipOverEMP(to, DIR_DOWN) == kPos) return true;
		}
		break;
	case GKE:
		if (to + DIR_KEDR == kPos) return true;
		if (to + DIR_KEDL == kPos) return true;
		break;
	case GGI:
		if (to + DIR_DOWN == kPos) return true;
		if (to + DIR_DR   == kPos) return true;
		if (to + DIR_DL   == kPos) return true;
		if (to + DIR_UR   == kPos) return true;
		if (to + DIR_UL   == kPos) return true;
		break;
	case GKI:
	case GTO:
	case GNY:
	case GNK:
	case GNG:
		if (to + DIR_DOWN  == kPos) return true;
		if (to + DIR_DR    == kPos) return true;
		if (to + DIR_DL    == kPos) return true;
		if (to + DIR_RIGHT == kPos) return true;
		if (to + DIR_LEFT  == kPos) return true;
		if (to + DIR_UP    == kPos) return true;
		break;

	case SUM:
	case GUM:
		if (to + DIR_UP    == kPos) return true;
		if (to + DIR_RIGHT == kPos) return true;
		if (to + DIR_LEFT  == kPos) return true;
		if (to + DIR_DOWN  == kPos) return true;
		// Through
	case SKA:
	case GKA:
		if ((DirTbl[to][kPos] & (EFFECT_UR | EFFECT_UL | EFFECT_DR | EFFECT_DL)) != 0) {
			if ((DirTbl[to][kPos] & EFFECT_UR) != 0) {
				if (SkipOverEMP(to, DIR_UR) == kPos) return true;
			}
			if ((DirTbl[to][kPos] & EFFECT_UL) != 0) {
				if (SkipOverEMP(to, DIR_UL) == kPos) return true;
			}
			if ((DirTbl[to][kPos] & EFFECT_DR) != 0) {
				if (SkipOverEMP(to, DIR_DR) == kPos) return true;
			}
			if ((DirTbl[to][kPos] & EFFECT_DL) != 0) {
				if (SkipOverEMP(to, DIR_DL) == kPos) return true;
			}
		}
		break;
	case SRY:
	case GRY:
		if (to + DIR_UR == kPos) return true;
		if (to + DIR_UL == kPos) return true;
		if (to + DIR_DR == kPos) return true;
		if (to + DIR_DL == kPos) return true;
		// Through
	case SHI:
	case GHI:
		if ((DirTbl[to][kPos] & (EFFECT_UP | EFFECT_RIGHT | EFFECT_LEFT | EFFECT_DOWN)) != 0) {
			if ((DirTbl[to][kPos] & EFFECT_UP) != 0) {
				if (SkipOverEMP(to, DIR_UP) == kPos) return true;
			}
			if ((DirTbl[to][kPos] & EFFECT_DOWN) != 0) {
				if (SkipOverEMP(to, DIR_DOWN) == kPos) return true;
			}
			if ((DirTbl[to][kPos] & EFFECT_RIGHT) != 0) {
				if (SkipOverEMP(to, DIR_RIGHT) == kPos) return true;
			}
			if ((DirTbl[to][kPos] & EFFECT_LEFT) != 0) {
				if (SkipOverEMP(to, DIR_LEFT) == kPos) return true;
			}
		}
		break;
	case SOU:
	case GOU:
	case WALL:
	case PIECE_NONE:
	default:
		break;
	}

	// ��ړ����邱�Ƃɂ�鉤��.
	const int from = move_from(m);
	if (from < 0x11) return false;
	if ((DirTbl[from][kPos]) & (akiki[from] >> EFFECT_LONG_SHIFT)) {
		unsigned long id;
		_BitScanForward(&id, DirTbl[from][kPos]);
 		if (DirTbl[from][kPos] == DirTbl[to][kPos]) return false; 
 		//if (from - to == NanohaTbl::Direction[id] || to - from == NanohaTbl::Direction[id]) return false;
		if (SkipOverEMP(from, NanohaTbl::Direction[id]) == kPos) {
			return true;
		}
	}

	return false;
}
bool Position::move_gives_check(Move m) const
{
	const Color us = side_to_move();
	return is_check_move(us, m);
}


// ���@�肩�m�F����
bool Position::pl_move_is_legal(const Move m) const
{
	const Piece piece = move_piece(m);
	const Color us = side_to_move();

	// �����̋�łȂ���𓮂����Ă��邩�H
	if (us != color_of(piece)) return false;

	const PieceType pt = type_of(piece);
	const int to = move_to(m);
	const int from = move_from(m);
	if (from == to) return false;

	if (move_is_drop(m)) {
		// �ł�������Ă��邩�H
		const Hand &h = (us == BLACK) ? handS : handG;
		if ( !h.exist(piece)) {
			return false;
		}
		if (ban[to] != EMP) {
			return false;
		}
		if (pt == FU) {
			// ����Ƒł����l�߂̃`�F�b�N
			if (is_double_pawn(us, to)) return false;
			if (is_pawn_drop_mate(us, to)) return false;
			// �s�����̂Ȃ���ł��͐������Ȃ��͂��Ȃ̂ŁA�`�F�b�N���ȗ����H
			if (us == BLACK) return is_drop_pawn<BLACK>(to);
			if (us == WHITE) return is_drop_pawn<WHITE>(to);
		} else if (pt == KY) {
			// �s�����̂Ȃ���ł��͐������Ȃ��͂��Ȃ̂ŁA�`�F�b�N���ȗ����H
			if (us == BLACK) return is_drop_pawn<BLACK>(to);
			if (us == WHITE) return is_drop_pawn<WHITE>(to);
		} else if (pt == KE) {
			// �s�����̂Ȃ���ł��͐������Ȃ��͂��Ȃ̂ŁA�`�F�b�N���ȗ����H
			if (us == BLACK) return is_drop_knight<BLACK>(to);
			if (us == WHITE) return is_drop_knight<WHITE>(to);
		}
	} else {
		// ����������݂��邩�H
#if !defined(NDEBUG)
		if (DEBUG_LEVEL > 0) {
			std::cerr << "Color=" << int(us) << ", sideToMove=" << int(sideToMove) << std::endl;
			std::cerr << "Move : from=0x" << std::hex << from << ", to=0x" << to << std::endl;
			std::cerr << "   piece=" << int(piece) << ", cap=" << int(move_captured(m)) << std::endl;
			std::cerr << "   ban[from]=" << int(ban[from]) << ", ban[to]=" << int(ban[to]) << std::endl;
		}
#endif
		if (ban[from] != piece) {
			return false;
		}
		if (ban[to] == WALL) {
			return false;
		}
		if (ban[to] != EMP && color_of(ban[to]) == us) {
			// �����̋������Ă���
			return false;
		}
		// �ʂ̏ꍇ�A���E�͂ł��Ȃ�
		if (move_ptype(m) == OU) {
			Color them = flip(sideToMove);
			if (effect[them][to]) return false;
		}
		// �s���̏ꍇ�A�s���̕����ɂ��������Ȃ��B
		if (pin[from]) {
			int kPos = (us == BLACK) ? kingS : kingG;
			if (DirTbl[kPos][to] != DirTbl[kPos][from]) return false;
		}
		// TODO:����щz���Ȃ����H
		int d = Max(abs((from >> 4)-(to >> 4)), abs((from & 0x0F)-(to&0x0F)));
		if (pt == KE) {
			if (d != 2) return false;
		} else if (d > 1) {
			// ���A�p�A��A�n�A�������Ȃ�.
			// �ړ��̓r�����`�F�b�N����
			int dir = (to - from) / d;
			if (((to - from) % d) != 0) return false;
			for (int i = 1, z = from + dir; i < d; i++, z += dir) {
				if (ban[z] != EMP) return false;
			}
		}
	}

#if 0
	// TODO:�s�����̂Ȃ���A����A�ł����l�߁A����щz���Ȃ����H���̃`�F�b�N
	// capture��Տ�̋�ɐݒ�
	if (IsCorrectMove(m)) {
		if (piece == SOU || piece == GOU) return 1;

		// ���ʂɉ���������Ă��Ȃ����A���ۂɓ������Ē��ׂ�
		Position kk(*this);
		StateInfo newSt;
		kk.do_move(m, newSt);
		if (us == BLACK && kingS && EXIST_EFFECT(kk.effectW[kingS])) {
			return false;
		}
		if (us != BLACK &&  kingG && EXIST_EFFECT(kk.effectB[kingG])) {
			return false;
		}
		return true;
	}
	return false;
#else
	return true;
#endif
}

// �w��ꏊ(to)���ł����l�߂ɂȂ邩�m�F����
bool Position::is_pawn_drop_mate(const Color us, int to) const
{
	// �܂��A�ʂ̓��ɕ���ł肶��Ȃ���Αł����l�߂̐S�z�͂Ȃ��B
	if (us == BLACK) {
		if (kingG + DIR_DOWN != to) {
			return 0;
		}
	} else {
		if (kingS + DIR_UP != to) {
			return 0;
		}
	}

	Piece piece;

	// �������邩�H
	if (us == BLACK) {
		// �����̗������Ȃ��Ȃ�ʂŎ���
		if (! EXIST_EFFECT(effectB[to])) return 0;

		// ��铮����񋓂��Ă݂���ʂŎ��肵���Ȃ�
		if ((EXIST_EFFECT(effectW[to]) & ~EFFECT_DOWN) != 0) {
			// �ʈȊO�Ŏ���肪����y�ۑ�zpin�̍l��
			effect_t kiki = effectW[to] & (EFFECT_SHORT_MASK & ~EFFECT_DOWN);
			unsigned long id;
			while (kiki) {
				_BitScanForward(&id, kiki);
				kiki &= (kiki - 1);
				if (pin[to - NanohaTbl::Direction[id]] == 0) return 0;
			}
			kiki = effectW[to] & EFFECT_LONG_MASK;
			while (kiki) {
				_BitScanForward(&id, kiki);
				kiki &= (kiki - 1);
				if (pin[SkipOverEMP(to, -NanohaTbl::Direction[id])] == 0) return 0;
			}
		}
		// �ʂɓ����������邩�ǂ������`�F�b�N
		if (effectB[to] & ((EFFECT_LEFT|EFFECT_RIGHT|EFFECT_UR|EFFECT_UL) << EFFECT_LONG_SHIFT)) {
			if ((effectB[to] & (EFFECT_LEFT  << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_LEFT ] != WALL && (ban[to+DIR_LEFT ] & GOTE) == 0)
			  && ((effectB[to+DIR_LEFT ] & ~(EFFECT_LEFT  << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
			if ((effectB[to] & (EFFECT_RIGHT << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_RIGHT] != WALL && (ban[to+DIR_RIGHT] & GOTE) == 0)
			  && ((effectB[to+DIR_RIGHT] & ~(EFFECT_RIGHT << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
			if ((effectB[to] & (EFFECT_UR    << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_UR   ] != WALL && (ban[to+DIR_UR   ] & GOTE) == 0)
			  && ((effectB[to+DIR_UR   ] & ~(EFFECT_UR    << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
			if ((effectB[to] & (EFFECT_UL    << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_UL   ] != WALL && (ban[to+DIR_UL   ] & GOTE) == 0)
			  && ((effectB[to+DIR_UL   ] & ~(EFFECT_UL    << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
		}
#define EscapeG(dir)	piece = ban[kingG + DIR_##dir];	\
						if (piece != WALL && !(piece & GOTE) && !EXIST_EFFECT(effectB[kingG + DIR_##dir])) return 0
		EscapeG(UP);
		EscapeG(UR);
		EscapeG(UL);
		EscapeG(RIGHT);
		EscapeG(LEFT);
		EscapeG(DR);
		EscapeG(DL);
#undef EscapeG

		// �ʂ̓��������Ȃ��̂Ȃ�A�ł����l�߁B
		return 1;
	} else {
		// �����̗������Ȃ��Ȃ�ʂŎ���
		if (! EXIST_EFFECT(effectW[to])) return 0;

		// ��铮����񋓂��Ă݂���ʂŎ��肵���Ȃ�
		if ((EXIST_EFFECT(effectB[to]) & ~EFFECT_UP) != 0) {
			// �ʈȊO�Ŏ���肪����y�ۑ�zpin�̍l��
			effect_t kiki = effectB[to] & (EFFECT_SHORT_MASK & ~EFFECT_UP);
			unsigned long id;
			while (kiki) {
				_BitScanForward(&id, kiki);
				kiki &= (kiki - 1);
				if (pin[to - NanohaTbl::Direction[id]] == 0) return 0;
			}
			kiki = effectB[to] & EFFECT_LONG_MASK;
			while (kiki) {
				_BitScanForward(&id, kiki);
				kiki &= (kiki - 1);
				if (pin[SkipOverEMP(to, -NanohaTbl::Direction[id])] == 0) return 0;
			}
		}
		// �ʂɓ����������邩�ǂ������`�F�b�N
		if (effectW[to] & ((EFFECT_LEFT|EFFECT_RIGHT|EFFECT_DR|EFFECT_DL) << EFFECT_LONG_SHIFT)) {
			if ((effectW[to] & (EFFECT_LEFT  << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_LEFT ] == EMP || (ban[to+DIR_LEFT ] & GOTE))
			  && ((effectW[to+DIR_LEFT ] & ~(EFFECT_LEFT  << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
			if ((effectW[to] & (EFFECT_RIGHT << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_RIGHT] == EMP || (ban[to+DIR_RIGHT] & GOTE))
			  && ((effectW[to+DIR_RIGHT] & ~(EFFECT_RIGHT << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
			if ((effectW[to] & (EFFECT_DR    << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_DR   ] == EMP || (ban[to+DIR_DR   ] & GOTE))
			  && ((effectW[to+DIR_DR   ] & ~(EFFECT_DR    << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
			if ((effectW[to] & (EFFECT_DL    << EFFECT_LONG_SHIFT))
			  && (ban[to+DIR_DL   ] == EMP || (ban[to+DIR_DL   ] & GOTE))
			  && ((effectW[to+DIR_DL   ] & ~(EFFECT_DL    << EFFECT_LONG_SHIFT)) == 0)) {
				return 0;
			}
		}
#define EscapeS(dir)	piece = ban[kingS + DIR_##dir];	\
						if ((piece == EMP || (piece & GOTE)) && !EXIST_EFFECT(effectW[kingS + DIR_##dir])) return 0
		EscapeS(DOWN);
		EscapeS(DR);
		EscapeS(DL);
		EscapeS(RIGHT);
		EscapeS(LEFT);
		EscapeS(UR);
		EscapeS(UL);
#undef EscapeG

		// �ʂ̓��������Ȃ��̂Ȃ�A�ł����l�߁B
		return 1;
	}
}

// Move Generator�n

// �萶��
template<Color us>
ExtMove* Position::add_straight(ExtMove* mlist, const int from, const int dir) const
{
	int z_pin = this->pin[from];
	if (z_pin == 0 || abs(z_pin) == abs(dir)) {
		// �󔒂̊ԁA������𐶐�����
		int to;
		int dan;
		int fromDan = from & 0x0f;
		bool promote = can_promotion<us>(fromDan);
		const Piece piece = ban[from];
		unsigned int tmp = From2Move(from) | Piece2Move(piece);
		for (to = from + dir; ban[to] == EMP; to += dir) {
			dan = to & 0x0f;
			promote |= can_promotion<us>(dan);
			tmp &= ~TO_MASK;
			tmp |= To2Move(to);
			if (promote && (piece & PROMOTED) == 0) {
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				if (us == BLACK && piece == SKY) {
					if (dan > 1) {
						(mlist++)->move = Move(tmp);
					}
				} else if (us == WHITE && piece == GKY) {
					if (dan < 9) {
						(mlist++)->move = Move(tmp);
					}
				} else {
					// �p�E���
					// ����Ȃ������������B
					(mlist++)->move = Move(tmp | MOVE_CHECK_NARAZU);
				}
			} else {
				// ����Ȃ��Ƃ��Ɣn�E��
				(mlist++)->move = Move(tmp);
			}
		}
		// �����̋�łȂ��Ȃ�A�����֓���
		if ((us == BLACK && (ban[to] != WALL) && (ban[to] & GOTE))
		 || (us == WHITE && (ban[to] != WALL) && (ban[to] & GOTE) == 0)) {
			dan = to & 0x0f;
			promote |= can_promotion<us>(dan);
			tmp &= ~TO_MASK;
			tmp |= To2Move(to) | Cap2Move(ban[to]);
			if (promote && (piece & PROMOTED) == 0) {
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				if (piece == SKY) {
					if (dan > 1) {
						(mlist++)->move = Move(tmp);
					}
				} else if (piece == GKY) {
					if (dan < 9) {
						(mlist++)->move = Move(tmp);
					}
				} else {
					// �p�E���
					// ����Ȃ������������B
					(mlist++)->move = Move(tmp | MOVE_CHECK_NARAZU);
				}
			} else {
				// ����Ȃ��Ƃ��Ɣn�E��
				(mlist++)->move = Move(tmp);
			}
		}
	}
	return mlist;
}

template<Color us>
ExtMove* Position::add_move(ExtMove* mlist, const int from, const int dir) const
{
	const int to = from + dir;
	const Piece capture = ban[to];
	if ((capture == EMP) 
		 || (us == BLACK &&  (capture & GOTE))
		 || (us == WHITE && ((capture & GOTE) == 0 && capture != WALL))
	) {
		const int piece = ban[from];
		int dan = to & 0x0f;
		int fromDan = from & 0x0f;
		bool promote = can_promotion<us>(dan) || can_promotion<us>(fromDan);
		unsigned int tmp = From2Move(from) | To2Move(to) | Piece2Move(piece) | Cap2Move(capture);
		if (promote) {
			const int kind = piece & ~GOTE;
			switch (kind) {
			case SFU:
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				if (is_drop_pawn<us>(dan)) {
					// ����Ȃ������������B
					(mlist++)->move = Move(tmp | MOVE_CHECK_NARAZU);
				}
				break;
			case SKY:
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				if (is_drop_pawn<us>(dan)) {
					// ����Ȃ������������B
					(mlist++)->move = Move(tmp);
				}
				break;
			case SKE:
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				if (is_drop_knight<us>(dan)) {
					// ����Ȃ������������B
					(mlist++)->move = Move(tmp);
				}
				break;
			case SGI:
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				(mlist++)->move = Move(tmp);
				break;
			case SKA:
			case SHI:
				(mlist++)->move = Move(tmp | FLAG_PROMO);
				// ����Ȃ������������B
				(mlist++)->move = Move(tmp | MOVE_CHECK_NARAZU);
				break;
			default:
				(mlist++)->move = Move(tmp);
				break;
			}
		} else {
			// ����Ȃ�
			(mlist++)->move = Move(tmp);
		}
	}
	return mlist;
}

// �w��ꏊ(to)�ɓ�����̐����i�ʈȊO�j
ExtMove* Position::gen_move_to(const Color us, ExtMove* mlist, int to) const
{
	effect_t efft = (us == BLACK) ? this->effectB[to] : this->effectW[to];

	// �w��ꏊ�ɗ����Ă����Ȃ�
	if ((efft & (EFFECT_SHORT_MASK | EFFECT_LONG_MASK)) == 0) return mlist;

	int z;
	int pn;

	// ��т̗���
	effect_t long_effect = efft & EFFECT_LONG_MASK;
	while (long_effect) {
		unsigned long id;
		_BitScanForward(&id, long_effect);
		id -= EFFECT_LONG_SHIFT;
		long_effect &= long_effect - 1;

		z = SkipOverEMP(to, -NanohaTbl::Direction[id]);
		pn = pin[z];
		if (pn == 0 || abs(pn) == abs(NanohaTbl::Direction[id])) {
			mlist = (us == BLACK) ? add_moveB(mlist, z, to - z) : add_moveW(mlist, z, to - z);
		}
	}

	// �Z������
	efft &= EFFECT_SHORT_MASK;
	while (efft) {
		unsigned long id;
		_BitScanForward(&id, efft);
		efft &= efft - 1;

		z = to - NanohaTbl::Direction[id];
		pn = pin[z];
		if (pn == 0 || abs(pn) == abs(NanohaTbl::Direction[id])) {
			if (us == BLACK) {
				if (ban[z] != SOU) mlist = add_moveB(mlist, z, to - z);
			} else {
				if (ban[z] != GOU) mlist = add_moveW(mlist, z, to - z);
			}
		}
	}
	return mlist;
}

// �w��ꏊ(to)�ɋ��ł�̐���
ExtMove* Position::gen_drop_to(const Color us, ExtMove* mlist, int to) const
{
	int dan = to & 0x0f;
	if (us != BLACK) {
		dan = 10 - dan;
	}
	const Hand &h = (us == BLACK) ? handS : handG;
	const int SorG = (us == BLACK) ? SENTE : GOTE;
#define SetTe(koma)	\
	if (h.get ## koma() > 0) {		\
		(mlist++)->move = Move(To2Move(to) | Piece2Move(SorG|koma));		\
	}
	
	if (h.getFU() > 0 && dan > 1) {
		// ����ł�𐶐�
		// ����`�F�b�N
		int nifu = is_double_pawn(us, to & 0xF0);
		// �ł����l�߂��`�F�b�N
		if (!nifu && !is_pawn_drop_mate(us, to)) {
			(mlist++)->move = Move(To2Move(to) | Piece2Move(SorG | FU));
		}
	}
	if (h.getKY() > 0 && dan > 1) {
		// ����ł�𐶐�
		(mlist++)->move = Move(To2Move(to) | Piece2Move(SorG|KY));
	}
	if (h.getKE() > 0 && dan > 2) {
		(mlist++)->move = Move(To2Move(to) | Piece2Move(SorG|KE));
	}
	SetTe(GI)
	SetTe(KI)
	SetTe(KA)
	SetTe(HI)
#undef SetTe
	return mlist;
}

// ���ł�̐���
template <Color us>
ExtMove* Position::gen_drop(ExtMove* mlist) const
{
	int z;
	int suji;
	unsigned int tmp;
	int StartDan;

#if defined(DEBUG_GENERATE)
	ExtMove* top = mlist;
#endif
///	int teNum = teNumM;	// �A�h���X�����Ȃ�
	// ����ł�
	uint32_t exists;
	exists = (us == BLACK) ? handS.existFU() : handG.existFU();
	if (exists > 0) {
		tmp  = (us == BLACK) ? Piece2Move(SFU) : Piece2Move(GFU);	// From = 0;
		//(���Ȃ�Q�i�ڂ�艺�ɁA���Ȃ�W�i�ڂ���ɑłj
		StartDan = (us == BLACK) ? 2 : 1;
		for (suji = 0x10; suji <= 0x90; suji += 0x10) {
			// ����`�F�b�N
			if (is_double_pawn(us, suji)) continue;
			z = suji + StartDan;
			// �ł����l�߂��`�F�b�N
#define FU_FUNC(z)	\
	if (ban[z] == EMP && !is_pawn_drop_mate(us, z)) {	\
		(mlist++)->move = Move(tmp | To2Move(z));	\
	}
			FU_FUNC(z  )
			FU_FUNC(z+1)
			FU_FUNC(z+2)
			FU_FUNC(z+3)
			FU_FUNC(z+4)
			FU_FUNC(z+5)
			FU_FUNC(z+6)
			FU_FUNC(z+7)
#undef FU_FUNC
		}
	}

	// ����ł�
	exists = (us == BLACK) ? handS.existKY() : handG.existKY();
	if (exists > 0) {
		tmp  = (us == BLACK) ? Piece2Move(SKY) : Piece2Move(GKY); // From = 0
		//(���Ȃ�Q�i�ڂ�艺�ɁA���Ȃ�W�i�ڂ���ɑłj
		z = (us == BLACK) ? 0x12 : 0x11;
		for(; z <= 0x99; z += 0x10) {
#define KY_FUNC(z)	\
			if (ban[z] == EMP) {	\
				(mlist++)->move = Move(tmp | To2Move(z));	\
			}
			KY_FUNC(z  )
			KY_FUNC(z+1)
			KY_FUNC(z+2)
			KY_FUNC(z+3)
			KY_FUNC(z+4)
			KY_FUNC(z+5)
			KY_FUNC(z+6)
			KY_FUNC(z+7)
#undef KY_FUNC
		}
	}

	//�j��ł�
	exists = (us == BLACK) ? handS.existKE() : handG.existKE();
	if (exists > 0) {
		//(���Ȃ�R�i�ڂ�艺�ɁA���Ȃ�V�i�ڂ���ɑłj
		tmp  = (us == BLACK) ? Piece2Move(SKE) : Piece2Move(GKE); // From = 0
		z = (us == BLACK) ? 0x13 : 0x11;
		for ( ; z <= 0x99; z += 0x10) {
#define KE_FUNC(z)	\
			if (ban[z] == EMP) {	\
				(mlist++)->move = Move(tmp | To2Move(z));	\
			}
			KE_FUNC(z)
			KE_FUNC(z+1)
			KE_FUNC(z+2)
			KE_FUNC(z+3)
			KE_FUNC(z+4)
			KE_FUNC(z+5)
			KE_FUNC(z+6)
#undef KE_FUNC
		}
	}

	// ��`��Ԃ́A�ǂ��ɂł��łĂ�
	const uint32_t koma_start = (us == BLACK) ? SGI : GGI;
	const uint32_t koma_end = (us == BLACK) ? SHI : GHI;
	uint32_t a[4];
	a[0] = (us == BLACK) ? handS.existGI() : handG.existGI();
	a[1] = (us == BLACK) ? handS.existKI() : handG.existKI();
	a[2] = (us == BLACK) ? handS.existKA() : handG.existKA();
	a[3] = (us == BLACK) ? handS.existHI() : handG.existHI();
	for (uint32_t koma = koma_start, i = 0; koma <= koma_end; koma++, i++) {
		if (a[i] > 0) {
			tmp  = Piece2Move(koma); // From = 0
			for (z = 0x11; z <= 0x99; z += 0x10) {
#define GI_FUNC(z)	\
				if (ban[z] == EMP) {	\
					(mlist++)->move = Move(tmp | To2Move(z));	\
				}
				GI_FUNC(z)
				GI_FUNC(z+1)
				GI_FUNC(z+2)
				GI_FUNC(z+3)
				GI_FUNC(z+4)
				GI_FUNC(z+5)
				GI_FUNC(z+6)
				GI_FUNC(z+7)
				GI_FUNC(z+8)
#undef GI_FUNC
			}
		}
	}

#if defined(DEBUG_GENERATE)
	while (top != mlist) {
		Move m = top->move;
		if (!move_is_drop(m)) {
			if (piece_on(Square(move_from(m))) == EMP) {
				assert(false);
			}
		}
		top++;
	}
#endif
	return mlist;
}

//�ʂ̓�����̐���
ExtMove* Position::gen_move_king(const Color us, ExtMove* mlist, int pindir) const
{
	int to;
	Piece koma;
	unsigned int tmp = (us == BLACK) ? From2Move(kingS) | Piece2Move(SOU) :  From2Move(kingG) | Piece2Move(GOU);

#define MoveKB(dir) to = kingS - DIR_##dir;	\
					if (EXIST_EFFECT(effectW[to]) == 0) {	\
						koma = ban[to];		\
						if (koma == EMP || (koma & GOTE)) {		\
							(mlist++)->move = Move(tmp | To2Move(to) | Cap2Move(ban[to]));	\
						}		\
					}
#define MoveKW(dir) to = kingG - DIR_##dir;	\
					if (EXIST_EFFECT(effectB[to]) == 0) {	\
						koma = ban[to];		\
						if (koma != WALL && !(koma & GOTE)) {		\
							(mlist++)->move = Move(tmp | To2Move(to) | Cap2Move(ban[to]));	\
						}		\
					}

	if (us == BLACK) {
		if (pindir == 0) {
			MoveKB(UP)
			MoveKB(UR)
			MoveKB(UL)
			MoveKB(RIGHT)
			MoveKB(LEFT)
			MoveKB(DR)
			MoveKB(DL)
			MoveKB(DOWN)
		} else {
			if (pindir != ABS(DIR_UP)   ) { MoveKB(UP)    }
			if (pindir != ABS(DIR_UR)   ) { MoveKB(UR)    }
			if (pindir != ABS(DIR_UL)   ) { MoveKB(UL)    }
			if (pindir != ABS(DIR_RIGHT)) { MoveKB(RIGHT) }
			if (pindir != ABS(DIR_LEFT) ) { MoveKB(LEFT)  }
			if (pindir != ABS(DIR_DR)   ) { MoveKB(DR)    }
			if (pindir != ABS(DIR_DL)   ) { MoveKB(DL)    }
			if (pindir != ABS(DIR_DOWN) ) { MoveKB(DOWN)  }
		}
	} else {
		if (pindir == 0) {
			MoveKW(UP)
			MoveKW(UR)
			MoveKW(UL)
			MoveKW(RIGHT)
			MoveKW(LEFT)
			MoveKW(DR)
			MoveKW(DL)
			MoveKW(DOWN)
		} else {
			if (pindir != ABS(DIR_UP)   ) { MoveKW(UP)    }
			if (pindir != ABS(DIR_UR)   ) { MoveKW(UR)    }
			if (pindir != ABS(DIR_UL)   ) { MoveKW(UL)    }
			if (pindir != ABS(DIR_RIGHT)) { MoveKW(RIGHT) }
			if (pindir != ABS(DIR_LEFT) ) { MoveKW(LEFT)  }
			if (pindir != ABS(DIR_DR)   ) { MoveKW(DR)    }
			if (pindir != ABS(DIR_DL)   ) { MoveKW(DL)    }
			if (pindir != ABS(DIR_DOWN) ) { MoveKW(DOWN)  }
		}
	}
#undef MoveKB
#undef MoveKW
	return mlist;
}


//from���瓮����̐���
// �Ֆʂ�from�ɂ����𓮂�����𐶐�����B
// pindir		�����Ȃ�����(pin����Ă���)
ExtMove* Position::gen_move_from(const Color us, ExtMove* mlist, int from, int pindir) const
{
	int z_pin = abs(this->pin[from]);
	pindir = abs(pindir);
#define AddMoveM1(teban,dir)     if (pindir != ABS(DIR_ ## dir)) if (z_pin == ABS(DIR_ ## dir)) mlist = add_move##teban(mlist, from, DIR_ ## dir)
#define AddStraightM1(teban,dir) if (pindir != ABS(DIR_ ## dir)) if (z_pin == ABS(DIR_ ## dir)) mlist = add_straight##teban(mlist, from, DIR_ ## dir)
#define AddMoveM2(teban,dir)     if (pindir != ABS(DIR_ ## dir)) mlist = add_move##teban(mlist, from, DIR_ ## dir)
#define AddStraightM2(teban,dir) if (pindir != ABS(DIR_ ## dir)) mlist = add_straight##teban(mlist, from, DIR_ ## dir)
	switch(ban[from]) {
	case SFU:
		if (z_pin) {
			AddMoveM1(B, UP);
		} else if (pindir) {
			AddMoveM2(B, UP);
		} else {
			mlist = add_moveB(mlist, from, DIR_UP);
		}
		break;
	case SKY:
		if (z_pin) {
			AddStraightM1(B, UP);
		} else if (pindir) {
			AddStraightM2(B, UP);
		} else {
			mlist = add_straightB(mlist, from, DIR_UP);
		}
		break;
	case SKE:
		// �j�n��pin�ƂȂ�������Ȃ�
		if (z_pin == 0) {
			mlist = add_moveB(mlist, from, DIR_KEUR);
			mlist = add_moveB(mlist, from, DIR_KEUL);
		}
		break;
	case SGI:
		if (z_pin) {
			AddMoveM1(B, UP);
			AddMoveM1(B, UR);
			AddMoveM1(B, UL);
			AddMoveM1(B, DR);
			AddMoveM1(B, DL);
		} else if (pindir) {
			AddMoveM2(B, UP);
			AddMoveM2(B, UR);
			AddMoveM2(B, UL);
			AddMoveM2(B, DR);
			AddMoveM2(B, DL);
		} else {
			mlist = add_moveB(mlist, from, DIR_UP);
			mlist = add_moveB(mlist, from, DIR_UR);
			mlist = add_moveB(mlist, from, DIR_UL);
			mlist = add_moveB(mlist, from, DIR_DR);
			mlist = add_moveB(mlist, from, DIR_DL);
		}
		break;
	case SKI:case STO:case SNY:case SNK:case SNG:
		if (z_pin) {
			AddMoveM1(B, UP);
			AddMoveM1(B, UR);
			AddMoveM1(B, UL);
			AddMoveM1(B, DOWN);
			AddMoveM1(B, RIGHT);
			AddMoveM1(B, LEFT);
		} else if (pindir) {
			AddMoveM2(B, UP);
			AddMoveM2(B, UR);
			AddMoveM2(B, UL);
			AddMoveM2(B, DOWN);
			AddMoveM2(B, RIGHT);
			AddMoveM2(B, LEFT);
		} else {
			mlist = add_moveB(mlist, from, DIR_UP);
			mlist = add_moveB(mlist, from, DIR_UR);
			mlist = add_moveB(mlist, from, DIR_UL);
			mlist = add_moveB(mlist, from, DIR_DOWN);
			mlist = add_moveB(mlist, from, DIR_RIGHT);
			mlist = add_moveB(mlist, from, DIR_LEFT);
		}
		break;
	case SUM:
		if (z_pin) {
			AddMoveM1(B, UP);
			AddMoveM1(B, RIGHT);
			AddMoveM1(B, LEFT);
			AddMoveM1(B, DOWN);
			AddStraightM1(B, UR);
			AddStraightM1(B, UL);
			AddStraightM1(B, DR);
			AddStraightM1(B, DL);
		} else if (pindir) {
			AddMoveM2(B, UP);
			AddMoveM2(B, RIGHT);
			AddMoveM2(B, LEFT);
			AddMoveM2(B, DOWN);
			AddStraightM2(B, UR);
			AddStraightM2(B, UL);
			AddStraightM2(B, DR);
			AddStraightM2(B, DL);
		} else {
			mlist = add_moveB(mlist, from, DIR_UP);
			mlist = add_moveB(mlist, from, DIR_RIGHT);
			mlist = add_moveB(mlist, from, DIR_LEFT);
			mlist = add_moveB(mlist, from, DIR_DOWN);
			mlist = add_straightB(mlist, from, DIR_UR);
			mlist = add_straightB(mlist, from, DIR_UL);
			mlist = add_straightB(mlist, from, DIR_DR);
			mlist = add_straightB(mlist, from, DIR_DL);
		}
		break;
	case SKA:
		if (z_pin) {
			AddStraightM1(B, UR);
			AddStraightM1(B, UL);
			AddStraightM1(B, DR);
			AddStraightM1(B, DL);
		} else if (pindir) {
			AddStraightM2(B, UR);
			AddStraightM2(B, UL);
			AddStraightM2(B, DR);
			AddStraightM2(B, DL);
		} else {
			mlist = add_straightB(mlist, from, DIR_UR);
			mlist = add_straightB(mlist, from, DIR_UL);
			mlist = add_straightB(mlist, from, DIR_DR);
			mlist = add_straightB(mlist, from, DIR_DL);
		}
		break;
	case SRY:
		if (z_pin) {
			AddMoveM1(B, UR);
			AddMoveM1(B, UL);
			AddMoveM1(B, DR);
			AddMoveM1(B, DL);
			AddStraightM1(B, UP);
			AddStraightM1(B, RIGHT);
			AddStraightM1(B, LEFT);
			AddStraightM1(B, DOWN);
		} else if (pindir) {
			AddMoveM2(B, UR);
			AddMoveM2(B, UL);
			AddMoveM2(B, DR);
			AddMoveM2(B, DL);
			AddStraightM2(B, UP);
			AddStraightM2(B, RIGHT);
			AddStraightM2(B, LEFT);
			AddStraightM2(B, DOWN);
		} else {
			mlist = add_moveB(mlist, from, DIR_UR);
			mlist = add_moveB(mlist, from, DIR_UL);
			mlist = add_moveB(mlist, from, DIR_DR);
			mlist = add_moveB(mlist, from, DIR_DL);
			mlist = add_straightB(mlist, from, DIR_UP);
			mlist = add_straightB(mlist, from, DIR_RIGHT);
			mlist = add_straightB(mlist, from, DIR_LEFT);
			mlist = add_straightB(mlist, from, DIR_DOWN);
		}
		break;
	case SHI:
		if (z_pin) {
			AddStraightM1(B, UP);
			AddStraightM1(B, RIGHT);
			AddStraightM1(B, LEFT);
			AddStraightM1(B, DOWN);
		} else if (pindir) {
			AddStraightM2(B, UP);
			AddStraightM2(B, RIGHT);
			AddStraightM2(B, LEFT);
			AddStraightM2(B, DOWN);
		} else {
			mlist = add_straightB(mlist, from, DIR_UP);
			mlist = add_straightB(mlist, from, DIR_RIGHT);
			mlist = add_straightB(mlist, from, DIR_LEFT);
			mlist = add_straightB(mlist, from, DIR_DOWN);
		}
		break;
	case SOU:
		mlist = gen_move_king(us, mlist, pindir);
		break;

	case GFU:
		if (z_pin) {
			AddMoveM1(W, DOWN);
		} else if (pindir) {
			AddMoveM2(W, DOWN);
		} else {
			mlist = add_moveW(mlist, from, DIR_DOWN);
		}
		break;
	case GKY:
		if (z_pin) {
			AddStraightM1(W, DOWN);
		} else if (pindir) {
			AddStraightM2(W, DOWN);
		} else {
			mlist = add_straightW(mlist, from, DIR_DOWN);
		}
		break;
	case GKE:
		// �j�n��pin�ƂȂ�������Ȃ�
		if (z_pin == 0) {
			mlist = add_moveW(mlist, from, DIR_KEDR);
			mlist = add_moveW(mlist, from, DIR_KEDL);
		}
		break;
	case GGI:
		if (z_pin) {
			AddMoveM1(W, DOWN);
			AddMoveM1(W, DR);
			AddMoveM1(W, DL);
			AddMoveM1(W, UR);
			AddMoveM1(W, UL);
		} else if (pindir) {
			AddMoveM2(W, DOWN);
			AddMoveM2(W, DR);
			AddMoveM2(W, DL);
			AddMoveM2(W, UR);
			AddMoveM2(W, UL);
		} else {
			mlist = add_moveW(mlist, from, DIR_DOWN);
			mlist = add_moveW(mlist, from, DIR_DR);
			mlist = add_moveW(mlist, from, DIR_DL);
			mlist = add_moveW(mlist, from, DIR_UR);
			mlist = add_moveW(mlist, from, DIR_UL);
		}
		break;
	case GKI:case GTO:case GNY:case GNK:case GNG:
		if (z_pin) {
			AddMoveM1(W, DOWN);
			AddMoveM1(W, DR);
			AddMoveM1(W, DL);
			AddMoveM1(W, UP);
			AddMoveM1(W, RIGHT);
			AddMoveM1(W, LEFT);
		} else if (pindir) {
			AddMoveM2(W, DOWN);
			AddMoveM2(W, DR);
			AddMoveM2(W, DL);
			AddMoveM2(W, UP);
			AddMoveM2(W, RIGHT);
			AddMoveM2(W, LEFT);
		} else {
			mlist = add_moveW(mlist, from, DIR_DOWN);
			mlist = add_moveW(mlist, from, DIR_DR);
			mlist = add_moveW(mlist, from, DIR_DL);
			mlist = add_moveW(mlist, from, DIR_UP);
			mlist = add_moveW(mlist, from, DIR_RIGHT);
			mlist = add_moveW(mlist, from, DIR_LEFT);
		}
		break;
	case GRY:
		if (z_pin) {
			AddMoveM1(W, UR);
			AddMoveM1(W, UL);
			AddMoveM1(W, DR);
			AddMoveM1(W, DL);
			AddStraightM1(W, UP);
			AddStraightM1(W, RIGHT);
			AddStraightM1(W, LEFT);
			AddStraightM1(W, DOWN);
		} else if (pindir) {
			AddMoveM2(W, UR);
			AddMoveM2(W, UL);
			AddMoveM2(W, DR);
			AddMoveM2(W, DL);
			AddStraightM2(W, UP);
			AddStraightM2(W, RIGHT);
			AddStraightM2(W, LEFT);
			AddStraightM2(W, DOWN);
		} else {
			mlist = add_moveW(mlist, from, DIR_UR);
			mlist = add_moveW(mlist, from, DIR_UL);
			mlist = add_moveW(mlist, from, DIR_DR);
			mlist = add_moveW(mlist, from, DIR_DL);
			mlist = add_straightW(mlist, from, DIR_UP);
			mlist = add_straightW(mlist, from, DIR_RIGHT);
			mlist = add_straightW(mlist, from, DIR_LEFT);
			mlist = add_straightW(mlist, from, DIR_DOWN);
		}
		break;
	case GHI:
		if (z_pin) {
			AddStraightM1(W, UP);
			AddStraightM1(W, RIGHT);
			AddStraightM1(W, LEFT);
			AddStraightM1(W, DOWN);
		} else if (pindir) {
			AddStraightM2(W, UP);
			AddStraightM2(W, RIGHT);
			AddStraightM2(W, LEFT);
			AddStraightM2(W, DOWN);
		} else {
			mlist = add_straightW(mlist, from, DIR_UP);
			mlist = add_straightW(mlist, from, DIR_RIGHT);
			mlist = add_straightW(mlist, from, DIR_LEFT);
			mlist = add_straightW(mlist, from, DIR_DOWN);
		}
		break;
	case GUM:
		if (z_pin) {
			AddMoveM1(W, UP);
			AddMoveM1(W, RIGHT);
			AddMoveM1(W, LEFT);
			AddMoveM1(W, DOWN);
			AddStraightM1(W, UR);
			AddStraightM1(W, UL);
			AddStraightM1(W, DR);
			AddStraightM1(W, DL);
		} else if (pindir) {
			AddMoveM2(W, UP);
			AddMoveM2(W, RIGHT);
			AddMoveM2(W, LEFT);
			AddMoveM2(W, DOWN);
			AddStraightM2(W, UR);
			AddStraightM2(W, UL);
			AddStraightM2(W, DR);
			AddStraightM2(W, DL);
		} else {
			mlist = add_moveW(mlist, from, DIR_UP);
			mlist = add_moveW(mlist, from, DIR_RIGHT);
			mlist = add_moveW(mlist, from, DIR_LEFT);
			mlist = add_moveW(mlist, from, DIR_DOWN);
			mlist = add_straightW(mlist, from, DIR_UR);
			mlist = add_straightW(mlist, from, DIR_UL);
			mlist = add_straightW(mlist, from, DIR_DR);
			mlist = add_straightW(mlist, from, DIR_DL);
		}
		break;
	case GKA:
		if (z_pin) {
			AddStraightM1(W, UR);
			AddStraightM1(W, UL);
			AddStraightM1(W, DR);
			AddStraightM1(W, DL);
		} else if (pindir) {
			AddStraightM2(W, UR);
			AddStraightM2(W, UL);
			AddStraightM2(W, DR);
			AddStraightM2(W, DL);
		} else {
			mlist = add_straightW(mlist, from, DIR_UR);
			mlist = add_straightW(mlist, from, DIR_UL);
			mlist = add_straightW(mlist, from, DIR_DR);
			mlist = add_straightW(mlist, from, DIR_DL);
		}
		break;
	case GOU:
		mlist = gen_move_king(us, mlist, pindir);
		break;
	case EMP: case WALL: case PIECE_NONE:
	default:
		break;
	}
#undef AddMoveM
	return mlist;
}

// ����(�{���𐬂��)�𐶐�
template<Color us>
ExtMove* Position::generate_capture(ExtMove* mlist) const
{
	int to;
	int from;
	const Color them = (us == BLACK) ? WHITE : BLACK;
	const effect_t *our_effect = (us == BLACK) ? effectB : effectW;
	const effect_t *their_effect = (us == BLACK) ? effectW : effectB;

	int kno;	// ��ԍ�
	PieceType type;
	effect_t k;
	unsigned long id;
#if defined(DEBUG_GENERATE)
	ExtMove* top = mlist;
#endif

	for (kno = 1; kno <= MAX_KOMANO; kno++) {
		to = knpos[kno];
		if (OnBoard(to)) {
			if (color_of(Piece(knkind[kno])) == them && EXIST_EFFECT(our_effect[to])) {
				// ����̋�Ɏ����̗���������Ύ���H(�vpin���̍l��)
				k = our_effect[to] & EFFECT_SHORT_MASK;
				while (k) {
					_BitScanForward(&id, k);
					k &= k-1;
					from = to - NanohaTbl::Direction[id];
					if (pin[from] && abs(pin[from]) != abs(NanohaTbl::Direction[id])) continue;
					type = type_of(ban[from]);
					if (type == OU) {
						// �ʂ͑���̗��������������Ȃ�
						if (EXIST_EFFECT(their_effect[to]) == 0) {
							mlist->move = cons_move(from, to, ban[from], ban[to], 0);
							mlist++;
						}
					} else if (can_promotion<us>(to) || can_promotion<us>(from)) {
						// �����ʒu�H
						if (type == GI) {
							// ��͐��ƕs���𐶐�����
							mlist->move = cons_move(from, to, ban[from], ban[to], 1);
							mlist++;
							mlist->move = cons_move(from, to, ban[from], ban[to], 0);
							mlist++;
						} else if (type == FU) {
							// ���͐��̂ݐ�������
							mlist->move = cons_move(from, to, ban[from], ban[to], 1);
							mlist++;
						} else if (type == KE) {
							// �j�͐��𐶐���3�i�ڂ̂ݕs���𐶐�����
							mlist->move = cons_move(from, to, ban[from], ban[to], 1);
							mlist++;
							if (is_drop_knight<us>(to)) {
								mlist->move = cons_move(from, to, ban[from], ban[to], 0);
								mlist++;
							}
						} else {
							// ���j��ȊO�̋�(���A����)
							mlist->move = cons_move(from, to, ban[from], ban[to], 0);
							mlist++;
						}
					} else {
						// ����Ȃ��ʒu
						mlist->move = cons_move(from, to, ban[from], ban[to]);
						mlist++;
					}
				}
				k = our_effect[to] & EFFECT_LONG_MASK;
				while (k) {
					_BitScanForward(&id, k);
					k &= k-1;
					from = SkipOverEMP(to, -NanohaTbl::Direction[id]);
					if (pin[from] && abs(pin[from]) != abs(NanohaTbl::Direction[id])) continue;
					type = type_of(ban[from]);
					if (type == KA || type == HI) {
						// �p��͐����Ƃ��͐��̂ݐ�������
						if (can_promotion<us>(to) || can_promotion<us>(from)) {
							mlist->move = cons_move(from, to, ban[from], ban[to], 1);
							mlist++;
						} else {
							mlist->move = cons_move(from, to, ban[from], ban[to], 0);
							mlist++;
						}
					} else if (type == KY) {
						if (can_promotion<us>(to)) {
							// �����ʒu
							mlist->move = cons_move(from, to, ban[from], ban[to], 1);
							mlist++;
							// ����3�i��or7�i�ڂ̂Ƃ��̂ݕs���𐶐�����
							if (is_drop_knight<us>(to)) {
								mlist->move = cons_move(from, to, ban[from], ban[to], 0);
								mlist++;
							}
						} else {
							// ����Ȃ��ʒu
							mlist->move = cons_move(from, to, ban[from], ban[to], 0);
							mlist++;
						}
					} else {
						// �p�򍁈ȊO�̋�(�n�A��)
						mlist->move = cons_move(from, to, ban[from], ban[to]);
						mlist++;
					}
				}
			}
		}
	}
	// �G�w�ɔ�𐬂荞�ގ�𐶐�����
	for (kno = KNS_HI; kno <= KNE_HI; kno++) {
		if (knkind[kno] == make_piece(us, HI)) {
			from = knpos[kno];
			if (OnBoard(from) && can_promotion<us>(from) == false && (pin[from] == 0 || pin[from] == DIR_UP || pin[from] == DIR_DOWN)) {
				const int dir = (us == BLACK) ? DIR_UP : DIR_DOWN;
				to = SkipOverEMP(from, dir);
				while (can_promotion<us>(to -= dir)) {
					mlist->move = cons_move(from, to, ban[from], ban[to], 1);
					mlist++;
				}
			}
		}
	}
	// ���̐����𐶐�
	for (kno = KNS_FU; kno <= KNE_FU; kno++) {
		if (knkind[kno] == make_piece(us, FU)) {
			from = knpos[kno];
			to = (us == BLACK) ? from + DIR_UP : from + DIR_DOWN;
			if (OnBoard(from) && can_promotion<us>(to) && ban[to] == EMP) {
				if (pin[from] == 0 || abs(pin[from]) == 1) {
					mlist->move = cons_move(from, to, ban[from], ban[to], 1);
					mlist++;
				}
			}
		}
	}
#if defined(DEBUG_GENERATE)
	while (top != mlist) {
		Move m = top->move;
		if (!move_is_drop(m)) {
			if (piece_on(Square(move_from(m))) == EMP) {
				assert(false);
			}
		}
		top++;
	}
#endif

	return mlist;
}

// �ʂ𓮂�����̐���(������Ȃ�)
// ���ʂ̋�ƈႢ�A����̗����̂���Ƃ���ɂ͓����Ȃ�
// pindir		�����Ȃ�����
ExtMove* Position::gen_king_noncapture(const Color us, ExtMove* mlist, const int pindir) const
{
	int to;
	Piece koma;
	unsigned int tmp = (us == BLACK) ? From2Move(kingS) | Piece2Move(SOU) :  From2Move(kingG) | Piece2Move(GOU);

#define MoveKS(dir) to = kingS - DIR_##dir;	\
					if (EXIST_EFFECT(effectW[to]) == 0) {	\
						koma = ban[to];		\
						if (koma == EMP) {		\
							(mlist++)->move = Move(tmp | To2Move(to) | Cap2Move(ban[to]));	\
						}		\
					}
#define MoveKG(dir) to = kingG - DIR_##dir;	\
					if (EXIST_EFFECT(effectB[to]) == 0) {	\
						koma = ban[to];		\
						if (koma == EMP) {		\
							(mlist++)->move = Move(tmp | To2Move(to) | Cap2Move(ban[to]));	\
						}		\
					}

	if (us == BLACK) {
		if (pindir == 0) {
			MoveKS(UP)
			MoveKS(UR)
			MoveKS(UL)
			MoveKS(RIGHT)
			MoveKS(LEFT)
			MoveKS(DR)
			MoveKS(DL)
			MoveKS(DOWN)
		} else {
			if (pindir != ABS(DIR_UP)   ) { MoveKS(UP)    }
			if (pindir != ABS(DIR_UR)   ) { MoveKS(UR)    }
			if (pindir != ABS(DIR_UL)   ) { MoveKS(UL)    }
			if (pindir != ABS(DIR_RIGHT)) { MoveKS(RIGHT) }
			if (pindir != ABS(DIR_LEFT) ) { MoveKS(LEFT)  }
			if (pindir != ABS(DIR_DR)   ) { MoveKS(DR)    }
			if (pindir != ABS(DIR_DL)   ) { MoveKS(DL)    }
			if (pindir != ABS(DIR_DOWN) ) { MoveKS(DOWN)  }
		}
	} else {
		if (pindir == 0) {
			MoveKG(UP)
			MoveKG(UR)
			MoveKG(UL)
			MoveKG(RIGHT)
			MoveKG(LEFT)
			MoveKG(DR)
			MoveKG(DL)
			MoveKG(DOWN)
		} else {
			if (pindir != ABS(DIR_UP)   ) { MoveKG(UP)    }
			if (pindir != ABS(DIR_UR)   ) { MoveKG(UR)    }
			if (pindir != ABS(DIR_UL)   ) { MoveKG(UL)    }
			if (pindir != ABS(DIR_RIGHT)) { MoveKG(RIGHT) }
			if (pindir != ABS(DIR_LEFT) ) { MoveKG(LEFT)  }
			if (pindir != ABS(DIR_DR)   ) { MoveKG(DR)    }
			if (pindir != ABS(DIR_DL)   ) { MoveKG(DL)    }
			if (pindir != ABS(DIR_DOWN) ) { MoveKG(DOWN)  }
		}
	}
#undef MoveKS
#undef MoveKG
	return mlist;
}

// �Տ�̋�𓮂�����̂��� generate_capture() �Ő��������������Đ�������(��������Ŏ��Ȃ���(�|���𐬂��)�𐶐�)
template <Color us>
ExtMove* Position::generate_non_capture(ExtMove* mlist) const
{
	int kn;
	int from;
	ExtMove* p = mlist;
#if defined(DEBUG_GENERATE)
	ExtMove* top = mlist;
#endif

	from = sq_king<us>();	// ��
	if (from) mlist = gen_king_noncapture(us, mlist);	// �l�����Ȃǋʂ��Ȃ������ڗ�
	for (kn = KNS_HI; kn <= KNE_FU; kn++) {
		from = knpos[kn];
		if (OnBoard(from)) {
			if (color_of(Piece(knkind[kn])) == us) {
				// ��Ԃ̂Ƃ�
				mlist = gen_move_from(us, mlist, from);
			}
		}
	}

	// generate_capture()�Ő��������������
	ExtMove *last = mlist;
	for (mlist = p; mlist < last; mlist++) {
		Move &tmp = mlist->move;
		// ����͂قڐ����ς�
		//   ����Ő����̂ɐ���Ȃ���͐������Ă��Ȃ�
		if (move_captured(tmp) != EMP) {
			if (is_promotion(tmp)) continue;
			// �����ŁA����&&����Ȃ���ɂȂ��Ă���B
			PieceType pt = move_ptype(tmp);
			switch (pt) {
			case FU:
				if (can_promotion<us>(move_to(tmp))) break;
				continue;
			case KE:
			case GI:
			case KI:
			case OU:
			case TO:
			case NY:
			case NK:
			case NG:
			case UM:
			case RY:
				continue;
			case KY:
				//   ���Ԃ�3�i��(7�i��)�͐������Ă���̂ŁA���O����(�Ώۂ�2�i��(8�i��)�̂�)
				if (((us == BLACK && (move_to(tmp) & 0x0F) != 2) || (us == WHITE && (move_to(tmp) & 0x0F) != 8))) continue;
				break;
			case KA:
			case HI:
				// �p��͐���Ȃ���𐶐����Ă��Ȃ��̂ŁA�����̂ɐ���Ȃ���͂��ׂđΏۂɂ���
				if (!can_promotion<us>(move_to(tmp)) && !can_promotion<us>(move_from(tmp))) continue;
				break;
			case PIECE_TYPE_NONE:
			default:
				print_csa(tmp);
				MYABORT();
				break;
			}
		}
		// ��Ԃ��G�w�ɐ��荞�ގ�͐����ς�
		if (move_ptype(tmp) == HI && is_promotion(tmp) && !can_promotion<us>(move_from(tmp)) && can_promotion<us>(move_to(tmp))) continue;
		// ���̐����͐����ς�
		if (move_ptype(tmp) == FU && is_promotion(tmp)) continue;
		if (mlist != p) (p++)->move = tmp;
		else p++;
	}

#if defined(DEBUG_GENERATE)
	mlist = p;
	while (top != mlist) {
		Move m = top->move;
		if (!move_is_drop(m)) {
			if (piece_on(Square(move_from(m))) == EMP) {
				assert(false);
			}
		}
		top++;
	}
#endif

	return gen_drop<us>(p);
}

// ��������̐���
template<Color us>
ExtMove* Position::generate_evasion(ExtMove* mlist) const
{
	const effect_t efft = (us == BLACK) ? effectW[kingS] & (EFFECT_LONG_MASK | EFFECT_SHORT_MASK) : effectB[kingG] & (EFFECT_LONG_MASK | EFFECT_SHORT_MASK);
#if defined(DEBUG_GENERATE)
	ExtMove* top = mlist;
#endif

	if ((efft & (efft - 1)) != 0) {
		// ������(������2�ȏ�)�̏ꍇ�͋ʂ𓮂��������Ȃ�
		return gen_move_king(us, mlist);
	} else {
		Square ksq = (us == BLACK) ? Square(kingS) : Square(kingG);
		unsigned long id = 0;	// �������s�v���� warning ���o�邽��0������
		int check;	// ����������Ă����̍��W
		if (efft & EFFECT_SHORT_MASK) {
			// ���т̂Ȃ������ɂ�鉤�� �� �����i�F���肵�Ă��������A�ʂ𓮂���
			_BitScanForward(&id, efft);
			check = ksq - NanohaTbl::Direction[id];
			//���������
			mlist = gen_move_to(us, mlist, check);
			//�ʂ𓮂���
			mlist = gen_move_king(us, mlist);
		} else {
			// ���ї����ɂ�鉤�� �� �����i�F���肵�Ă��������A�ʂ𓮂����A����
			_BitScanForward(&id, efft);
			id -= EFFECT_LONG_SHIFT;
			check = SkipOverEMP(ksq, -NanohaTbl::Direction[id]);
			//���������
			mlist = gen_move_to(us, mlist, check);
			//�ʂ𓮂���
			mlist = gen_move_king(us, mlist);
			//����������𐶐�����
			int sq;
			for (sq = ksq - NanohaTbl::Direction[id]; ban[sq] == EMP; sq -= NanohaTbl::Direction[id]) {
				mlist = gen_move_to(us, mlist, sq);
			}
			for (sq = ksq - NanohaTbl::Direction[id]; ban[sq] == EMP; sq -= NanohaTbl::Direction[id]) {
				mlist = gen_drop_to(us, mlist, sq);  //���ł�;
			}
		}
	}
#if defined(DEBUG_GENERATE)
	while (top != mlist) {
		Move m = top->move;
		if (!move_is_drop(m)) {
			if (piece_on(Square(move_from(m))) == EMP) {
				assert(false);
			}
		}
		top++;
	}
#endif

	return mlist;
}

template<Color us>
ExtMove* Position::generate_non_evasion(ExtMove* mlist) const
{
	int z;

	// �Տ�̋�𓮂���
	int kn;
	z = (us == BLACK) ? knpos[1] : knpos[2];
	if (z) mlist = gen_move_king(us, mlist);
	for (kn = KNS_HI; kn <= KNE_FU; kn++) {
		z = knpos[kn];
		if (OnBoard(z)) {
			Piece kind = ban[z];
			if (color_of(kind) == us) {
				mlist = gen_move_from(us, mlist, z);
			}
		}
	}
	mlist = gen_drop<us>(mlist);
	return mlist;
}

// �@�\�F�����錾�ł��邩�ǂ������肷��
//
// �����F���
//
// �߂�l
//   true�F�����錾�ł���
//   false�F�����錾�ł��Ȃ�
//
bool Position::IsKachi(const Color us) const
{
	// ���ʐ錾�����̏����ɂ��ĉ��Ɏ����B
	// --
	// (a) �錾���̎�Ԃł���B
	// (b) �錾���̋ʂ��G�w�O�i�ڈȓ��ɓ����Ă���B
	// (c) �錾����(���5�_����1�_�̌v�Z��)
	//   ���̏ꍇ28�_�ȏ�̎��_������B
	//   ���̏ꍇ27�_�ȏ�̎��_������B
	//   �_���̑ΏۂƂȂ�̂́A�錾���̎���ƓG�w�O�i�ڈȓ��ɑ��݂���ʂ������錾���̋�݂̂ł���
	// (d) �錾���̓G�w�O�i�ڈȓ��̋�́A�ʂ�������10���ȏ㑶�݂���B
	// (e) �錾���̋ʂɉ��肪�������Ă��Ȃ��B(�l�߂��K���ł��邱�Ƃ͊֌W�Ȃ�)
	// (f) �錾���̎������Ԃ��c���Ă���B(�؂ꕉ���̏ꍇ)

	int suji;
	int dan;
	int maisuu = 0;
	unsigned int point = 0;
	// ����(a)
	if (us == BLACK) {
		// ����(b) ����
		if ((kingS & 0x0F) > 3) return false;
		// ����(e)
		if (EXIST_EFFECT(effectW[kingS])) return false;
		// ����(c)(d) ����
		for (suji = 0x10; suji <= 0x90; suji += 0x10) {
			for (dan = 1; dan <= 3; dan++) {
				Piece piece = Piece(ban[suji+dan] & ~PROMOTED);		// �ʂ�0�ɂȂ�̂ŃJ�E���g�O
				if (piece != EMP && !(piece & GOTE)) {
					if (piece == SHI || piece == SKA) point += 5;
					else point++;
					maisuu++;
				}
			}
		}
		// ����(d) ����
		if (maisuu < 10) return false;
		point += handS.getFU() + handS.getKY() + handS.getKE() + handS.getGI() + handS.getKI();
		point += 5 * handS.getKA();
		point += 5 * handS.getHI();
		// ����(c) ���� (���28�_�ȏ�A���27�_�ȏ�)
		if (point < 28) return false;
	} else {
		// ����(b) ����
		if ((kingG & 0x0F) < 7) return false;
		// ����(e)
		if (EXIST_EFFECT(effectB[kingG])) return false;
		// ����(c)(d) ����
		for (suji = 0x10; suji <= 0x90; suji += 0x10) {
			for (dan = 7; dan <= 9; dan++) {
				Piece piece = Piece(ban[suji+dan] & ~PROMOTED);
				if (piece == (GOU & ~PROMOTED)) continue;
				if (piece & GOTE) {
					if (piece == GHI || piece == GKA) point += 5;
					else point++;
					maisuu++;
				}
			}
		}
		// ����(d) ����
		if (maisuu < 10) return false;
		point += handG.getFU() + handG.getKY() + handG.getKE() + handG.getGI() + handG.getKI();
		point += 5 * handG.getKA();
		point += 5 * handG.getHI();
		// ����(c) ���� (���28�_�ȏ�A���27�_�ȏ�)
		if (point < 27) return false;
	}

	return true;
}

namespace {
//
//  �n�t�}����
//           �Տ�(6 + ��)  ����(5 + ��)
//            ��(S/G + Promoted)�A��(S/G)
//    ��     xxxxx0 + 0    (none)
//    ��     xxxx01 + 2    xxxx0 + 1
//    ��     xx0011 + 2    xx001 + 1
//    �j     xx1011 + 2    xx101 + 1
//    ��     xx0111 + 2    xx011 + 1
//    ��     x01111 + 1    x0111 + 1
//    �p     011111 + 2    01111 + 1
//    ��     111111 + 2    11111 + 1
//
static const struct HuffmanBoardTBL {
	int code;
	int bits;
} HB_tbl[] = {
	{0x00, 1},	// EMP
	{0x01, 4},	// SFU
	{0x03, 6},	// SKY
	{0x0B, 6},	// SKE
	{0x07, 6},	// SGI
	{0x0F, 6},	// SKI
	{0x1F, 8},	// SKA
	{0x3F, 8},	// SHI
	{0x00, 0},	// SOU
	{0x05, 4},	// STO
	{0x13, 6},	// SNY
	{0x1B, 6},	// SNK
	{0x17, 6},	// SNG
	{0x00,-1},	// ---
	{0x5F, 8},	// SUM
	{0x7F, 8},	// SRY
	{0x00,-1},	// ---
	{0x09, 4},	// GFU
	{0x23, 6},	// GKY
	{0x2B, 6},	// GKE
	{0x27, 6},	// GGI
	{0x2F, 6},	// GKI
	{0x9F, 8},	// GKA
	{0xBF, 8},	// GHI
	{0x00, 0},	// GOU
	{0x0D, 4},	// GTO	// �ԈႢ
	{0x33, 6},	// GNY
	{0x3B, 6},	// GNK
	{0x37, 6},	// GNG
	{0x00,-1},	// ---
	{0xDF, 8},	// GUM
	{0xFF, 8},	// GRY
	{0x00,-1},	// ---
};

static const struct HuffmanHandTBL {
	int code;
	int bits;
} HH_tbl[] = {
	{0x00,-1},	// EMP
	{0x00, 2},	// SFU
	{0x01, 4},	// SKY
	{0x05, 4},	// SKE
	{0x03, 4},	// SGI
	{0x07, 5},	// SKI
	{0x0F, 6},	// SKA
	{0x1F, 6},	// SHI
	{0x00,-1},	// SOU
	{0x00,-1},	// STO
	{0x00,-1},	// SNY
	{0x00,-1},	// SNK
	{0x00,-1},	// SNG
	{0x00,-1},	// ---
	{0x00,-1},	// SUM
	{0x00,-1},	// SRY
	{0x00,-1},	// ---
	{0x02, 2},	// GFU
	{0x09, 4},	// GKY
	{0x0D, 4},	// GKE
	{0x0B, 4},	// GGI
	{0x17, 5},	// GKI
	{0x2F, 6},	// GKA
	{0x3F, 6},	// GHI
	{0x00,-1},	// GOU
	{0x00,-1},	// GTO
	{0x00,-1},	// GNY
	{0x00,-1},	// GNK
	{0x00,-1},	// GNG
	{0x00,-1},	// ---
	{0x00,-1},	// GUM
	{0x00,-1},	// GRY
	{0x00,-1},	// ---
};

//
// ����
//   const int start_bit;	// �L�^�J�nbit�ʒu
//   const int bits;		// �r�b�g��(��)
//   const int data;		// �L�^����f�[�^
//   unsigned char buf[];	// �����������f�[�^���L�^����o�b�t�@
//   const int size;		// �o�b�t�@�T�C�Y
//
// �߂�l
//   ���o�����f�[�^
//
int set_bit(const int start_bit, const int bits, const int data, unsigned char buf[], const int size)
{
	static const int mask_tbl[] = {
		0x0000, 0x0001, 0x0003, 0x0007, 0x000F, 0x001F, 0x003F, 0x007F, 0x00FF
	};
	if (start_bit < 0) return -1;
	if (bits <= 0 || bits > 8) return -1;
	if (start_bit + bits > 8*size) return -2;
	if ((data & mask_tbl[bits]) != data) return -3;

	const int n = start_bit / 8;
	const int shift = start_bit % 8;
	buf[n] |= (data << shift);
	if (shift + bits > 8) {
		buf[n+1] = (data >> (8 - shift));
	}
	return start_bit + bits;
}
};

// �@�\�F�ǖʂ��n�t�}������������(��Ճ��[�`���p)
//
// ����
//   const int SorG;		// ���
//   unsigned char buf[];	// �����������f�[�^���L�^����o�b�t�@
//
// �߂�l
//   �}�C�i�X�F�G���[
//   ���̒l�F�G���R�[�h�����Ƃ��̃r�b�g��
//
int Position::EncodeHuffman(unsigned char buf[32]) const
{
	const int KingS = (((kingS >> 4)-1) & 0x0F)*9+(kingS & 0x0F);
	const int KingG = (((kingG >> 4)-1) & 0x0F)*9+(kingG & 0x0F);
	const int size = 32;	// buf[] �̃T�C�Y

	int start_bit = 0;

	if (kingS == 0 || kingG == 0) {
		// Error!
		return -1;
	}
///	printf("KingS=%d\n", KingS);
///	printf("KingG=%d\n", KingG);

	memset(buf, 0, size);

	// ��Ԃ𕄍���
	start_bit = set_bit(start_bit, 1, side_to_move(), buf, size);
	// �ʂ̈ʒu�𕄍���
	start_bit = set_bit(start_bit, 7, KingS,               buf, size);
	start_bit = set_bit(start_bit, 7, KingG,               buf, size);

	// �Տ�̃f�[�^�𕄍���
	int suji, dan;
	int piece;
	for (suji = 0x10; suji <= 0x90; suji += 0x10) {
		for (dan = 1; dan <= 9; dan++) {
			piece = ban[suji + dan];
			if (piece < EMP || piece > GRY) {
				// Error!
				exit(1);
			}
			if (HB_tbl[piece].bits < 0) {
				// Error!
				exit(1);
			}
			if (HB_tbl[piece].bits == 0) {
				// �ʂ͕ʓr
				continue;
			}
			start_bit = set_bit(start_bit, HB_tbl[piece].bits, HB_tbl[piece].code, buf, size);
		}
	}

	// ����𕄍���
	unsigned int i, n;
#define EncodeHand(SG,KOMA)			\
		piece = SG ## KOMA;			\
		n = hand ## SG.get ## KOMA();	\
		for (i = 0; i < n; i++) {	\
			start_bit = set_bit(start_bit, HH_tbl[piece].bits, HH_tbl[piece].code, buf, size);	\
		}

	EncodeHand(G,HI)
	EncodeHand(G,KA)
	EncodeHand(G,KI)
	EncodeHand(G,GI)
	EncodeHand(G,KE)
	EncodeHand(G,KY)
	EncodeHand(G,FU)

	EncodeHand(S,HI)
	EncodeHand(S,KA)
	EncodeHand(S,KI)
	EncodeHand(S,GI)
	EncodeHand(S,KE)
	EncodeHand(S,KY)
	EncodeHand(S,FU)

#undef EncodeHand

	return start_bit;
}

// �C���X�^���X��.
template ExtMove* Position::generate_capture<BLACK>(ExtMove* mlist) const;
template ExtMove* Position::generate_capture<WHITE>(ExtMove* mlist) const;
template ExtMove* Position::generate_non_capture<BLACK>(ExtMove* mlist) const;
template ExtMove* Position::generate_non_capture<WHITE>(ExtMove* mlist) const;
template ExtMove* Position::generate_evasion<BLACK>(ExtMove* mlist) const;
template ExtMove* Position::generate_evasion<WHITE>(ExtMove* mlist) const;
template ExtMove* Position::generate_non_evasion<BLACK>(ExtMove* mlist) const;
template ExtMove* Position::generate_non_evasion<WHITE>(ExtMove* mlist) const;

