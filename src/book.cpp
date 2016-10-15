/*
  Usapyon2, a USI shogi(japanese-chess) playing engine derived from 
  Stockfish 7 & nanoha-mini 0.2.2.1
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2008 Tord Romstad (Glaurung author)
  Copyright (C) 2008-2015 Marco Costalba, Joona Kiiski, Tord Romstad
  Copyright (C) 2015-2016 Marco Costalba, Joona Kiiski, Gary Linscott, Tord Romstad
  Copyright (C) 2014-2016 Kazuyuki Kawabata
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

#include <cassert>
#include <iostream>

#include "misc.h"
#include "book.h"
#include "movegen.h"
#include "position.h"

#ifdef USAPYON2
Book *book[2];
#else
Book *book;
#endif

Book::Book() : joseki(), prng(now())
{
}
Book::~Book() {}
void Book::open(const std::string& fileName)
{
	FILE *fp = fopen(fileName.c_str(), "rb");
	if (fp == NULL) {
		perror(fileName.c_str());
		return;
	}

	BookKey key;
	BookEntry data;

	for (;;) {
		size_t n;
		n = fread(&key, sizeof(key), 1, fp);
		if (n == 0) break;
		n = fread(&data, sizeof(data), 1, fp);
		if (n == 0) break;
		Joseki_type::iterator p;
		p = joseki.find(key);
		if (p == joseki.end()) {
			joseki.insert(Joseki_value(key, data));
		} else {
			output_info("Error!:Duplicated opening data\n");
		}
	}

	fclose(fp);
}
void Book::close(){}
// ��Ճf�[�^����A���݂̋ǖ�k�̍��@�肪���̋ǖʂłǂꂭ�炢�̕p�x�Ŏw���ꂽ����Ԃ��B
void Book::fromJoseki(Position &pos, int &mNum, ExtMove moves[], BookEntry data[])
{
	BookKey key;
	BookEntry d_null;
	ExtMove *last = generate<LEGAL>(pos, moves);
	mNum = last - moves;
	memset(&d_null, 0, sizeof(d_null));

	int i;
	StateInfo newSt;
	for (i = 0; i < mNum; i++) {
		Move m = moves[i].move;
		pos.do_move(m, newSt);
		int ret = pos.EncodeHuffman(key.data);
		pos.undo_move(m);
		if (ret < 0) {
			pos.print_csa(m);
			output_info("Error!:Huffman encode!\n");
			continue;
		}
		Joseki_type::iterator p;
		p = joseki.find(key);
		if (p == joseki.end()) {
			// �f�[�^�Ȃ�
			data[i] = d_null;
		} else {
			// �f�[�^����
			data[i] = p->second;
		}
	}
}

// ���݂̋ǖʂ��ǂ̂��炢�̕p�x�Ŏw���ꂽ����Ճf�[�^�𒲂ׂ�
int Book::getHindo(const Position &pos)
{
	BookKey key;
	int hindo = 0;

	int ret = pos.EncodeHuffman(key.data);
	if (ret < 0) {
		pos.print_csa();
		output_info("Error!:Huffman encode!\n");
		return 0;
	}
	Joseki_type::iterator p;
	p = joseki.find(key);
	if (p != joseki.end()) {
		// �f�[�^����
		hindo = p->second.hindo;
	}

	return hindo;
}

Move Book::get_move(Position& pos, bool findBestMove)
{
	// ��Ճf�[�^�Ɏ肪����΂�����g��.
	// �V�`����D�悷��
	if (size() > 0) {
		int teNum = 0;
		ExtMove moves[MAX_MOVES];
		BookEntry hindo2[MAX_MOVES];
		BookEntry candidate[4];
		int i;
		memset(candidate, 0, sizeof(candidate));

		fromJoseki(pos, teNum, moves, hindo2);
		// ��ԏ����̍������I�ԁB
		float swin = 0.5f;
		float win_max = 0.0f;
		int max = -1;
		int max_hindo = -1;
		// �ɒ[�ɏ��Ȃ���͑I�����Ȃ� �ˍŕp�o����10����1�ȉ��Ƃ���
		for (i = 0; i < teNum; i++) {
			if (max_hindo < hindo2[i].hindo) {
				max_hindo = hindo2[i].hindo;
			}
		}
		for (i = 0; i < teNum; i++) {
			if (hindo2[i].hindo > 0) {
				// �������̂����ɓ����
				if (candidate[3].hindo < hindo2[i].hindo) {
					for (int j = 0; j < 4; j++) {
						if (candidate[j].hindo < hindo2[i].hindo) {
							for (int k = 3; k > j; k--) {
								candidate[k] = candidate[k-1];
							}
							candidate[j] = hindo2[i];
							candidate[j].eval = i;
							break;
						}
					}
				}

				// �����Z�o
				if (hindo2[i].swin + hindo2[i].gwin > 0) {
					swin = hindo2[i].swin / float(hindo2[i].swin + hindo2[i].gwin);
				} else {
					swin = 0.5f;
				}
				if (pos.side_to_move() != BLACK) swin = 1.0f - swin;

				// �ɒ[�ɏ��Ȃ���͑I�����Ȃ� �ˍŕp�o����10����1�ȉ��Ƃ���
				if (max_hindo / 10 < hindo2[i].hindo && win_max < swin) {
					max = i;
					win_max = swin;
				}
			}
		}

		if (max >= 0) {
			if (!findBestMove) {
				// �����ŕԂ�
				int n = Min(teNum, 4);
				int total = 0;
				for (i = 0; i < n; i++) {
					total += candidate[i].hindo;
				}
				float p = prng.rand<unsigned short>() / 65536.0f;
				max = candidate[0].eval;
				int sum = 0;
				for (i = 0; i < n; i++) {
					sum += candidate[i].hindo;
					if (float(sum) / total >= p) {
						max = candidate[i].eval;
						break;
					}
				}
				if (hindo2[max].swin + hindo2[max].gwin > 0) {
					swin = hindo2[max].swin / float(hindo2[max].swin + hindo2[max].gwin);
					if (pos.side_to_move() != BLACK) swin = 1.0f - swin;
				} else {
					swin = 0.5f;
				}
			}
			// �����Ƃ��悳�����Ȏ��Ԃ�
			output_info("info string %5.1f%%, P(%d, %d)\n", swin*100.0f, hindo2[max].swin, hindo2[max].gwin);
			return moves[max].move;
		} else {
			output_info("info string No book data(plys=%d)\n", pos.gamePly);
		}
	}

	return MOVE_NONE;
}
