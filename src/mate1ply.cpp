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

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "position.h"
#include "movegen.h"

// �V�K�ߓ_�ŌŒ�[���̒T���𕹗p����df-pn�A���S���Y�� gpw05.pdf
//  ���q�m�K �c���N�N �R���a�I �썇�d

// �u4.3 ���l����֐��̐݌v�v�Q��
// �܂��ʂ�8 �ߖT(���͂�8 �}�X) �ɂ��Ď��̏����v�Z����(�e8 bit):
//  (1) ���ł��̃}�X(�ʈȊO�̗������Ȃ��C�U���̗����������)
//  (2) �ʂ��ړ��\�ȃ}�X(�U���̗������Ȃ��C ����̋���Ȃ�)
//  (3) ��������ł͋ʂ��ړ��\�ȃ}�X(�󔒂��U���̋����)
//  (4) ��𓮂������̃}�X(�ʈȊO�̗������Ȃ��U���̗�����2�ȏ゠��悤�ȁC �󔒂܂��͎���̋�̂���)
uint32_t Position::TblMate1plydrop[0x10000];
static uint8_t TblKikiCheck[32];				// [��̎��] �� ����ɂȂ�ʒu
static uint8_t TblKikiKind[8][32];			// [����][��̎��] �� ����
static uint8_t TblKikiIntercept[8][12][32];	// [���������̕���][��̕���][��̎��] �� �Ղ������

// info ��(1) ��(2) �̑g�ݍ��킹(���킹��16 bit) ��
// �ւ��āC (1) �̂ǂ����ɋ�ł��s��(2) ���ǂ��̂ɕK�v
// �Ȏ���̎�ނ����炩���ߋ��߂ĕ\�ɕۑ����Ă����D
// �Ώۂ͍��A��A���A�p�A��
//   �����͑ł����l�߂ƂȂ邽�ߏ��O����
//   ���j��8�ߖT�ł͌���Ȃ��̂ŏ��O����
void Position::initMate1ply()
{
	memset(TblMate1plydrop, 0, sizeof(TblMate1plydrop));

	// �����̒�`
	//  Dir05 Dir00 Dir04
	//  Dir03   ��  Dir02
	//  Dir07 Dir01 Dir06
	unsigned int i1;
	unsigned int i2;
	for (i2 = 0; i2 <= 0xFF; i2++) {
		for (i1 = 0; i1 <= 0xFF; i1++) {
			//  (1) ���ł��̃}�X(�ʈȊO�̗������Ȃ��C�U���̗����������)
			//  (2) �ʂ��ړ��\�ȃ}�X(�U���̗������Ȃ��C ����̋���Ȃ�)
			if (i1 & KIKI00) {
				// �����F�ΏۊO
				// �����F�˕��l�ߗp�ɓo�^
				if ((i2 & ~(KIKI00)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GFU);
				}
				// ��荁�͑ΏۊO
				// ��荁(DIR00 �� DIR01 �ȊO�ɓ����Ȃ��Ƌl��))
				if ((i2 & ~(KIKI00 | KIKI01)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GKY);
				}
				// ����͑ΏۊO
				// ����(DIR00 �� DIR02 �� DIR03 �ȊO�ɓ����Ȃ��Ƌl��))
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI03)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GGI);
				}
				// ����(DIR00, DIR04, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI04 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKI)
						| (1u << STO) | (1u << SNY) | (1u << SNK) | (1u << SNG);
				}
				// ����(DIR00, DIR02, DIR03, DIR04, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI03 | KIKI04 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GKI)
						| (1u << GTO) | (1u << GNY) | (1u << GNK) | (1u << GNG);
				}
				// �p�͑ΏۊO
				// ��(DIR00, DIR01, DIR04, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI04 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SHI) | (1u << GHI);
				}
				// �n(DIR00, DIR02, DIR03, DIR04, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI03 | KIKI04 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR00, DIR01, DIR02, DIR03, DIR04, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI02 | KIKI03 | KIKI04 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI01) {
				// �����F�˕��l�ߗp�ɓo�^
				if ((i2 & ~(KIKI01)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SFU);
				}
				// �����F�ΏۊO
				// ��荁(DIR00 �� DIR01 �ȊO�ɓ����Ȃ��Ƌl��))
				if ((i2 & ~(KIKI00 | KIKI01)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKY);
				}
				// ��荁�F�ΏۊO
				// ����(DIR01 �� DIR02 �� DIR03 �ȊO�ɓ����Ȃ��Ƌl��))
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI03)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SGI);
				}
				// ����͑ΏۊO
				// ����(DIR01 �� DIR02 �� DIR03 �� DIR06 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��))
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI03 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKI)
						| (1u << STO) | (1u << SNY) | (1u << SNK) | (1u << SNG);
				}
				// ����(DIR01 �� DIR06 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��))
				if ((i2 & ~(KIKI01 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1 << GKI)
						| (1u << GTO) | (1u << GNY) | (1u << GNK) | (1u << GNG);
				}
				// �p�͑ΏۊO
				// ��(DIR00, DIR01, DIR06, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SHI) | (1u << GHI);
				}
				// �n(DIR01, DIR02, DIR03, DIR06, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI03 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR00, DIR01, DIR02, DIR03, DIR06, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI02 | KIKI03 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI02) {
				// ��荁�͑ΏۊO
				// ��荁�͑ΏۊO
				// ����͑ΏۊO
				// ����͑ΏۊO
				// ����(DIR00, DIR02, DIR04, DIR06�ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI04 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKI)
						| (1u << STO) | (1u << SNY) | (1u << SNK) | (1u << SNG);
				}
				// ����(DIR01, DIR02, DIR04, DIR06�ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI04 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GKI)
						| (1u << GTO) | (1u << GNY) | (1u << GNK) | (1u << GNG);
				}
				// �p�͑ΏۊO
				// ��(DIR02, DIR03, DIR04, DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI02 | KIKI03 | KIKI04 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SHI) | (1u << GHI);
				}
				// �n(DIR00, DIR01, DIR02, DIR04, DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI02 | KIKI04 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR00, DIR01, DIR02, DIR03, DIR04, DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI02 | KIKI03 | KIKI04 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI03) {
				// ��荁�͑ΏۊO
				// ��荁�͑ΏۊO
				// ����͑ΏۊO
				// ����͑ΏۊO
				// ����(DIR00, DIR03, DIR05, DIR07�ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI03 | KIKI05 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKI)
						| (1u << STO) | (1u << SNY) | (1u << SNK) | (1u << SNG);
				}
				// ����(DIR01, DIR03, DIR05, DIR07�ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI03 | KIKI05 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GKI)
						| (1u << GTO) | (1u << GNY) | (1u << GNK) | (1u << GNG);
				}
				// �p�͑ΏۊO
				// ��(DIR02, DIR03, DIR05, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI02 | KIKI03 | KIKI05 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SHI) | (1u << GHI);
				}
				// �n(DIR00, DIR01, DIR03, DIR05, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI03 | KIKI05 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR00, DIR01, DIR02, DIR03, DIR05, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI01 | KIKI02 | KIKI03 | KIKI05 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI04) {
				// ��荁�͑ΏۊO
				// ��荁�͑ΏۊO
				// ����(DIR04 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI04)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SGI);
				}
				// ����(DIR02, DIR04 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI02 | KIKI04)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GGI);
				}
				// �����͑ΏۊO
				// ����(DIR00, DIR02, DIR04 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI04)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GKI)
						| (1u << GTO) | (1u << GNY) | (1u << GNK) | (1u << GNG);
				}
				// �p(DIR04 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI04 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKA) | (1u << GKA);
				}
				// ��͑ΏۊO
				// �n(DIR00, DIR02, DIR04 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI04 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR00, DIR02, DIR04, DIR05, DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI02 | KIKI04 | KIKI05 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI05) {
				// ��荁�͑ΏۊO
				// ��荁�͑ΏۊO
				// ����(DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SGI);
				}
				// ����(DIR03, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI03 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GGI);
				}
				// �����͑ΏۊO
				// ����(DIR00, DIR03, DIR05 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI03 | KIKI05)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GKI)
						| (1u << GTO) | (1u << GNY) | (1u << GNK) | (1u << GNG);
				}
				// �p(DIR05 �� DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI05 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKA) | (1u << GKA);
				}
				// ��͑ΏۊO
				// �n(DIR00, DIR03, DIR05 �� DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI03 | KIKI05 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR00, DIR03, DIR04, DIR05, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI00 | KIKI03 | KIKI04 | KIKI05 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI06) {
				// ��荁�͑ΏۊO
				// ��荁�͑ΏۊO
				// ����(DIR02, DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI02 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SGI);
				}
				// ����(DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GGI);
				}
				// ����(DIR01 �� DIR02 �� DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKI)
						| (1u << STO) | (1u << SNY) | (1u << SNK) | (1u << SNG);
				}
				// �����͑ΏۊO
				// �p(DIR05 �� DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI05 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKA) | (1u << GKA);
				}
				// ��͑ΏۊO
				// �n(DIR01, DIR02, DIR05 �� DIR06 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI05 | KIKI06)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR01, DIR02, DIR04, DIR06 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI02 | KIKI04 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
			if (i1 & KIKI07) {
				// ��荁�͑ΏۊO
				// ��荁�͑ΏۊO
				// ����(DIR03, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI03 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SGI);
				}
				// ����(DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << GGI);
				}
				// ����(DIR01 �� DIR03 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI03 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKI)
						| (1u << STO) | (1u << SNY) | (1u << SNK) | (1u << SNG);
				}
				// �����͑ΏۊO
				// �p(DIR04 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI04 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SKA) | (1u << GKA);
				}
				// ��͑ΏۊO
				// �n(DIR01, DIR03, DIR04 �� DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI03 | KIKI04 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SUM) | (1u << GUM);
				}
				// ��(DIR01, DIR03, DIR05, DIR06, DIR07 �ȊO�ɓ����Ȃ��Ƌl��)
				if ((i2 & ~(KIKI01 | KIKI03 | KIKI05 | KIKI06 | KIKI07)) == 0) {
					TblMate1plydrop[i2 * 256 + i1] |= (1u << SRY) | (1u << GRY);
				}
			}
		}
	}

	// �����̒�`
	//  Dir05 Dir00 Dir04
	//  Dir03   ��  Dir02
	//  Dir07 Dir01 Dir06

	//  ���ɂǂ̕����ɂ���Ή���ɂȂ邩
	// [��̎��] �� ����
	TblKikiCheck[SFU] = (KIKI01);
	TblKikiCheck[SKY] = (KIKI01);
	TblKikiCheck[SKE] = 0;
	TblKikiCheck[SGI] = (KIKI01 | KIKI04 | KIKI05 | KIKI06 | KIKI07);
	TblKikiCheck[SKI] = (KIKI00 | KIKI01 | KIKI02 | KIKI03 | KIKI06 | KIKI07);
	TblKikiCheck[SKA] = (KIKI04 | KIKI05 | KIKI06 | KIKI07);
	TblKikiCheck[SHI] = (KIKI00 | KIKI01 | KIKI02 | KIKI03);
	TblKikiCheck[STO] = TblKikiCheck[SKI];
	TblKikiCheck[SNY] = TblKikiCheck[SKI];
	TblKikiCheck[SNK] = TblKikiCheck[SKI];
	TblKikiCheck[SNG] = TblKikiCheck[SKI];
	TblKikiCheck[SUM] = TblKikiCheck[SKA] | TblKikiCheck[SHI];
	TblKikiCheck[SRY] = TblKikiCheck[SKA] | TblKikiCheck[SHI];

	TblKikiCheck[GFU] = (KIKI00);
	TblKikiCheck[GKY] = (KIKI00);
	TblKikiCheck[GKE] = 0;
	TblKikiCheck[GGI] = (KIKI00 | KIKI04 | KIKI05 | KIKI06 | KIKI07);
	TblKikiCheck[GKI] = (KIKI00 | KIKI01 | KIKI02 | KIKI03 | KIKI04 | KIKI05);
	TblKikiCheck[GKA] = TblKikiCheck[SKA];
	TblKikiCheck[GHI] = TblKikiCheck[SHI];
	TblKikiCheck[GTO] = TblKikiCheck[GKI];
	TblKikiCheck[GNY] = TblKikiCheck[GKI];
	TblKikiCheck[GNK] = TblKikiCheck[GKI];
	TblKikiCheck[GNG] = TblKikiCheck[GKI];
	TblKikiCheck[GUM] = TblKikiCheck[SUM];
	TblKikiCheck[GRY] = TblKikiCheck[SRY];

	//  ������DirXX�����ɋ�����Ƃ��̗���
	// [����][��̎��] �� ����
	// TblKikiKind[0]
	TblKikiKind[0][SFU] = 0;
	TblKikiKind[0][SKY] = 0;
	TblKikiKind[0][SKE] = 0;
	TblKikiKind[0][SGI] = (KIKI02 | KIKI03);
	TblKikiKind[0][SKI] = (KIKI04 | KIKI05);
	TblKikiKind[0][SKA] = (KIKI02 | KIKI03);
	TblKikiKind[0][SHI] = (KIKI01 | KIKI04 | KIKI05);
	TblKikiKind[0][STO] = TblKikiKind[0][SKI];
	TblKikiKind[0][SNY] = TblKikiKind[0][SKI];
	TblKikiKind[0][SNK] = TblKikiKind[0][SKI];
	TblKikiKind[0][SNG] = TblKikiKind[0][SKI];
	TblKikiKind[0][SUM] = TblKikiKind[0][SKA] | (KIKI04 | KIKI05);
	TblKikiKind[0][SRY] = TblKikiKind[0][SHI] | (KIKI02 | KIKI03);

	TblKikiKind[0][GFU] = 0;
	TblKikiKind[0][GKY] = (KIKI01);
	TblKikiKind[0][GKE] = (KIKI06 | KIKI07);
	TblKikiKind[0][GGI] = (KIKI02 | KIKI03);
	TblKikiKind[0][GKI] = (KIKI02 | KIKI03 | KIKI04 | KIKI05);
	TblKikiKind[0][GKA] = TblKikiKind[0][SKA];
	TblKikiKind[0][GHI] = TblKikiKind[0][SHI];
	TblKikiKind[0][GTO] = TblKikiKind[0][GKI];
	TblKikiKind[0][GNY] = TblKikiKind[0][GKI];
	TblKikiKind[0][GNK] = TblKikiKind[0][GKI];
	TblKikiKind[0][GNG] = TblKikiKind[0][GKI];
	TblKikiKind[0][GUM] = TblKikiKind[0][SUM];
	TblKikiKind[0][GRY] = TblKikiKind[0][SRY];

	// TblKikiKind[1]
	TblKikiKind[1][SFU] = 0;
	TblKikiKind[1][SKY] = (KIKI00);
	TblKikiKind[1][SKE] = (KIKI04 | KIKI05);
	TblKikiKind[1][SGI] = (KIKI02 | KIKI03);
	TblKikiKind[1][SKI] = (KIKI02 | KIKI03 | KIKI06 | KIKI07);
	TblKikiKind[1][SKA] = (KIKI02 | KIKI03);
	TblKikiKind[1][SHI] = (KIKI00 | KIKI06 | KIKI07);
	TblKikiKind[1][STO] = TblKikiKind[0][SKI];
	TblKikiKind[1][SNY] = TblKikiKind[0][SKI];
	TblKikiKind[1][SNK] = TblKikiKind[0][SKI];
	TblKikiKind[1][SNG] = TblKikiKind[0][SKI];
	TblKikiKind[1][SUM] = TblKikiKind[0][SKA] | (KIKI06 | KIKI07);
	TblKikiKind[1][SRY] = TblKikiKind[0][SHI] | (KIKI02 | KIKI03);

	TblKikiKind[1][GFU] = 0;
	TblKikiKind[1][GKY] = 0;
	TblKikiKind[1][GKE] = 0;
	TblKikiKind[1][GGI] = (KIKI02 | KIKI03);
	TblKikiKind[1][GKI] = (KIKI06 | KIKI07);
	TblKikiKind[1][GKA] = TblKikiKind[1][SKA];
	TblKikiKind[1][GHI] = TblKikiKind[1][SHI];
	TblKikiKind[1][GTO] = TblKikiKind[1][GKI];
	TblKikiKind[1][GNY] = TblKikiKind[1][GKI];
	TblKikiKind[1][GNK] = TblKikiKind[1][GKI];
	TblKikiKind[1][GNG] = TblKikiKind[1][GKI];
	TblKikiKind[1][GUM] = TblKikiKind[1][SUM];
	TblKikiKind[1][GRY] = TblKikiKind[1][SRY];

	// TblKikiKind[2]
	TblKikiKind[2][SFU] = (KIKI04);
	TblKikiKind[2][SKY] = (KIKI04);
	TblKikiKind[2][SKE] = 0;
	TblKikiKind[2][SGI] = (KIKI00 | KIKI04);
	TblKikiKind[2][SKI] = (KIKI00 | KIKI04 | KIKI06);
	TblKikiKind[2][SKA] = (KIKI00 | KIKI01);
	TblKikiKind[2][SHI] = (KIKI03 | KIKI04 | KIKI06);
	TblKikiKind[2][STO] = TblKikiKind[2][SKI];
	TblKikiKind[2][SNY] = TblKikiKind[2][SKI];
	TblKikiKind[2][SNK] = TblKikiKind[2][SKI];
	TblKikiKind[2][SNG] = TblKikiKind[2][SKI];
	TblKikiKind[2][SUM] = TblKikiKind[2][SKA] | (KIKI04 | KIKI06);
	TblKikiKind[2][SRY] = TblKikiKind[2][SHI] | (KIKI00 | KIKI01);

	TblKikiKind[2][GFU] = (KIKI06);
	TblKikiKind[2][GKY] = (KIKI06);
	TblKikiKind[2][GKE] = 0;
	TblKikiKind[2][GGI] = (KIKI01 | KIKI06);
	TblKikiKind[2][GKI] = (KIKI01 | KIKI04 | KIKI06);
	TblKikiKind[2][GKA] = TblKikiKind[2][SKA];
	TblKikiKind[2][GHI] = TblKikiKind[2][SHI];
	TblKikiKind[2][GTO] = TblKikiKind[2][GKI];
	TblKikiKind[2][GNY] = TblKikiKind[2][GKI];
	TblKikiKind[2][GNK] = TblKikiKind[2][GKI];
	TblKikiKind[2][GNG] = TblKikiKind[2][GKI];
	TblKikiKind[2][GUM] = TblKikiKind[2][SUM];
	TblKikiKind[2][GRY] = TblKikiKind[2][SRY];

	// TblKikiKind[3]
	TblKikiKind[3][SFU] = (KIKI05);
	TblKikiKind[3][SKY] = (KIKI05);
	TblKikiKind[3][SKE] = 0;
	TblKikiKind[3][SGI] = (KIKI00 | KIKI05);
	TblKikiKind[3][SKI] = (KIKI00 | KIKI05 | KIKI07);
	TblKikiKind[3][SKA] = (KIKI00 | KIKI01);
	TblKikiKind[3][SHI] = (KIKI02 | KIKI05 | KIKI07);
	TblKikiKind[3][STO] = TblKikiKind[3][SKI];
	TblKikiKind[3][SNY] = TblKikiKind[3][SKI];
	TblKikiKind[3][SNK] = TblKikiKind[3][SKI];
	TblKikiKind[3][SNG] = TblKikiKind[3][SKI];
	TblKikiKind[3][SUM] = TblKikiKind[3][SKA] | (KIKI05 | KIKI07);
	TblKikiKind[3][SRY] = TblKikiKind[3][SHI] | (KIKI00 | KIKI01);

	TblKikiKind[3][GFU] = (KIKI07);
	TblKikiKind[3][GKY] = (KIKI07);
	TblKikiKind[3][GKE] = 0;
	TblKikiKind[3][GGI] = (KIKI01 | KIKI07);
	TblKikiKind[3][GKI] = (KIKI01 | KIKI05 | KIKI07);
	TblKikiKind[3][GKA] = TblKikiKind[3][SKA];
	TblKikiKind[3][GHI] = TblKikiKind[3][SHI];
	TblKikiKind[3][GTO] = TblKikiKind[3][GKI];
	TblKikiKind[3][GNY] = TblKikiKind[3][GKI];
	TblKikiKind[3][GNK] = TblKikiKind[3][GKI];
	TblKikiKind[3][GNG] = TblKikiKind[3][GKI];
	TblKikiKind[3][GUM] = TblKikiKind[3][SUM];
	TblKikiKind[3][GRY] = TblKikiKind[3][SRY];

	// TblKikiKind[4]
	TblKikiKind[4][SFU] = 0;
	TblKikiKind[4][SKY] = 0;
	TblKikiKind[4][SKE] = 0;
	TblKikiKind[4][SGI] = 0;
	TblKikiKind[4][SKI] = (KIKI00 | KIKI02);
	TblKikiKind[4][SKA] = (KIKI07);
	TblKikiKind[4][SHI] = (KIKI00 | KIKI02 | KIKI05 | KIKI06);
	TblKikiKind[4][STO] = TblKikiKind[4][SKI];
	TblKikiKind[4][SNY] = TblKikiKind[4][SKI];
	TblKikiKind[4][SNK] = TblKikiKind[4][SKI];
	TblKikiKind[4][SNG] = TblKikiKind[4][SKI];
	TblKikiKind[4][SUM] = TblKikiKind[4][SKA] | (KIKI00 | KIKI02);
	TblKikiKind[4][SRY] = TblKikiKind[4][SHI];

	TblKikiKind[4][GFU] = (KIKI02);
	TblKikiKind[4][GKY] = (KIKI02 | KIKI06);
	TblKikiKind[4][GKE] = (KIKI01);
	TblKikiKind[4][GGI] = (KIKI02);
	TblKikiKind[4][GKI] = (KIKI00 | KIKI02);
	TblKikiKind[4][GKA] = TblKikiKind[4][SKA];
	TblKikiKind[4][GHI] = TblKikiKind[4][SHI];
	TblKikiKind[4][GTO] = TblKikiKind[4][GKI];
	TblKikiKind[4][GNY] = TblKikiKind[4][GKI];
	TblKikiKind[4][GNK] = TblKikiKind[4][GKI];
	TblKikiKind[4][GNG] = TblKikiKind[4][GKI];
	TblKikiKind[4][GUM] = TblKikiKind[4][SUM];
	TblKikiKind[4][GRY] = TblKikiKind[4][SRY];

	// TblKikiKind[5]
	TblKikiKind[5][SFU] = 0;
	TblKikiKind[5][SKY] = 0;
	TblKikiKind[5][SKE] = 0;
	TblKikiKind[5][SGI] = 0;
	TblKikiKind[5][SKI] = (KIKI00 | KIKI03);
	TblKikiKind[5][SKA] = (KIKI06);
	TblKikiKind[5][SHI] = (KIKI00 | KIKI03 | KIKI04 | KIKI07);
	TblKikiKind[5][STO] = TblKikiKind[5][SKI];
	TblKikiKind[5][SNY] = TblKikiKind[5][SKI];
	TblKikiKind[5][SNK] = TblKikiKind[5][SKI];
	TblKikiKind[5][SNG] = TblKikiKind[5][SKI];
	TblKikiKind[5][SUM] = TblKikiKind[5][SKA] | (KIKI00 | KIKI03);
	TblKikiKind[5][SRY] = TblKikiKind[5][SHI];

	TblKikiKind[5][GFU] = (KIKI03);
	TblKikiKind[5][GKY] = (KIKI03 | KIKI07);
	TblKikiKind[5][GKE] = (KIKI01);
	TblKikiKind[5][GGI] = (KIKI03);
	TblKikiKind[5][GKI] = (KIKI00 | KIKI03);
	TblKikiKind[5][GKA] = TblKikiKind[5][SKA];
	TblKikiKind[5][GHI] = TblKikiKind[5][SHI];
	TblKikiKind[5][GTO] = TblKikiKind[5][GKI];
	TblKikiKind[5][GNY] = TblKikiKind[5][GKI];
	TblKikiKind[5][GNK] = TblKikiKind[5][GKI];
	TblKikiKind[5][GNG] = TblKikiKind[5][GKI];
	TblKikiKind[5][GUM] = TblKikiKind[5][SUM];
	TblKikiKind[5][GRY] = TblKikiKind[5][SRY];

	// TblKikiKind[6]
	TblKikiKind[6][SFU] = (KIKI02);
	TblKikiKind[6][SKY] = (KIKI02 | KIKI04);
	TblKikiKind[6][SKE] = (KIKI00);
	TblKikiKind[6][SGI] = (KIKI02);
	TblKikiKind[6][SKI] = (KIKI01 | KIKI02);
	TblKikiKind[6][SKA] = (KIKI05);
	TblKikiKind[6][SHI] = (KIKI01 | KIKI02 | KIKI04 | KIKI07);
	TblKikiKind[6][STO] = TblKikiKind[6][SKI];
	TblKikiKind[6][SNY] = TblKikiKind[6][SKI];
	TblKikiKind[6][SNK] = TblKikiKind[6][SKI];
	TblKikiKind[6][SNG] = TblKikiKind[6][SKI];
	TblKikiKind[6][SUM] = TblKikiKind[6][SKA] | (KIKI01 | KIKI02);
	TblKikiKind[6][SRY] = TblKikiKind[6][SHI];

	TblKikiKind[6][GFU] = 0;
	TblKikiKind[6][GKY] = 0;
	TblKikiKind[6][GKE] = 0;
	TblKikiKind[6][GGI] = 0;
	TblKikiKind[6][GKI] = (KIKI01 | KIKI02);
	TblKikiKind[6][GKA] = TblKikiKind[6][SKA];
	TblKikiKind[6][GHI] = TblKikiKind[6][SHI];
	TblKikiKind[6][GTO] = TblKikiKind[6][GKI];
	TblKikiKind[6][GNY] = TblKikiKind[6][GKI];
	TblKikiKind[6][GNK] = TblKikiKind[6][GKI];
	TblKikiKind[6][GNG] = TblKikiKind[6][GKI];
	TblKikiKind[6][GUM] = TblKikiKind[6][SUM];
	TblKikiKind[6][GRY] = TblKikiKind[6][SRY];

	// TblKikiKind[7]
	TblKikiKind[7][SFU] = (KIKI03);
	TblKikiKind[7][SKY] = (KIKI03 | KIKI05);
	TblKikiKind[7][SKE] = (KIKI00);
	TblKikiKind[7][SGI] = (KIKI03);
	TblKikiKind[7][SKI] = (KIKI01 | KIKI03);
	TblKikiKind[7][SKA] = (KIKI04);
	TblKikiKind[7][SHI] = (KIKI01 | KIKI03 | KIKI05 | KIKI06);
	TblKikiKind[7][STO] = TblKikiKind[7][SKI];
	TblKikiKind[7][SNY] = TblKikiKind[7][SKI];
	TblKikiKind[7][SNK] = TblKikiKind[7][SKI];
	TblKikiKind[7][SNG] = TblKikiKind[7][SKI];
	TblKikiKind[7][SUM] = TblKikiKind[7][SKA] | (KIKI01 | KIKI03);
	TblKikiKind[7][SRY] = TblKikiKind[7][SHI];

	TblKikiKind[7][GFU] = 0;
	TblKikiKind[7][GKY] = 0;
	TblKikiKind[7][GKE] = 0;
	TblKikiKind[7][GGI] = 0;
	TblKikiKind[7][GKI] = (KIKI01 | KIKI03);
	TblKikiKind[7][GKA] = TblKikiKind[7][SKA];
	TblKikiKind[7][GHI] = TblKikiKind[7][SHI];
	TblKikiKind[7][GTO] = TblKikiKind[7][GKI];
	TblKikiKind[7][GNY] = TblKikiKind[7][GKI];
	TblKikiKind[7][GNK] = TblKikiKind[7][GKI];
	TblKikiKind[7][GNG] = TblKikiKind[7][GKI];
	TblKikiKind[7][GUM] = TblKikiKind[7][SUM];
	TblKikiKind[7][GRY] = TblKikiKind[7][SRY];

	// �����̒�`
	//  Dir09       Dir08
	//  Dir05 Dir00 Dir04
	//  Dir03   ��  Dir02
	//  Dir07 Dir01 Dir06
	//  Dir11       Dir10

	// [���������̕���][��̕���][��̎��] �� �Ղ������
	//TblKikiIntercept[8][12][32];

	// ����������Dir00���� �F��̕�����02, 03, 06, 07, 10, 11���`�F�b�N����
	TblKikiIntercept[0][2][SFU] = 0;
	TblKikiIntercept[0][2][SKY] = 0;
	TblKikiIntercept[0][2][SKE] = (KIKI04);
	TblKikiIntercept[0][2][SGI] = 0;
	TblKikiIntercept[0][2][SKI] = 0;
	TblKikiIntercept[0][2][SKA] = (KIKI04);
	TblKikiIntercept[0][2][SHI] = 0;
	TblKikiIntercept[0][2][STO] = TblKikiIntercept[0][2][SKI];
	TblKikiIntercept[0][2][SNY] = TblKikiIntercept[0][2][SKI];
	TblKikiIntercept[0][2][SNK] = TblKikiIntercept[0][2][SKI];
	TblKikiIntercept[0][2][SNG] = TblKikiIntercept[0][2][SKI];
	TblKikiIntercept[0][2][SUM] = 0;
	TblKikiIntercept[0][2][SRY] = 0;

	TblKikiIntercept[0][2][GFU] = (KIKI04);
	TblKikiIntercept[0][2][GKY] = (KIKI04);
	TblKikiIntercept[0][2][GKE] = (KIKI04);
	TblKikiIntercept[0][2][GGI] = (KIKI04);
	TblKikiIntercept[0][2][GKI] = 0;
	TblKikiIntercept[0][2][GKA] = TblKikiIntercept[0][2][SKA];
	TblKikiIntercept[0][2][GHI] = TblKikiIntercept[0][2][SHI];
	TblKikiIntercept[0][2][GTO] = TblKikiIntercept[0][2][GKI];
	TblKikiIntercept[0][2][GNY] = TblKikiIntercept[0][2][GKI];
	TblKikiIntercept[0][2][GNK] = TblKikiIntercept[0][2][GKI];
	TblKikiIntercept[0][2][GNG] = TblKikiIntercept[0][2][GKI];
	TblKikiIntercept[0][2][GUM] = TblKikiIntercept[0][2][SUM];
	TblKikiIntercept[0][2][GRY] = TblKikiIntercept[0][2][SRY];

	TblKikiIntercept[0][3][SFU] = 0;
	TblKikiIntercept[0][3][SKY] = 0;
	TblKikiIntercept[0][3][SKE] = (KIKI05);
	TblKikiIntercept[0][3][SGI] = 0;
	TblKikiIntercept[0][3][SKI] = 0;
	TblKikiIntercept[0][3][SKA] = (KIKI05);
	TblKikiIntercept[0][3][SHI] = 0;
	TblKikiIntercept[0][3][STO] = TblKikiIntercept[0][3][SKI];
	TblKikiIntercept[0][3][SNY] = TblKikiIntercept[0][3][SKI];
	TblKikiIntercept[0][3][SNK] = TblKikiIntercept[0][3][SKI];
	TblKikiIntercept[0][3][SNG] = TblKikiIntercept[0][3][SKI];
	TblKikiIntercept[0][3][SUM] = 0;
	TblKikiIntercept[0][3][SRY] = 0;

	TblKikiIntercept[0][3][GFU] = (KIKI05);
	TblKikiIntercept[0][3][GKY] = (KIKI05);
	TblKikiIntercept[0][3][GKE] = (KIKI05);
	TblKikiIntercept[0][3][GGI] = (KIKI05);
	TblKikiIntercept[0][3][GKI] = 0;
	TblKikiIntercept[0][3][GKA] = TblKikiIntercept[0][3][SKA];
	TblKikiIntercept[0][3][GHI] = TblKikiIntercept[0][3][SHI];
	TblKikiIntercept[0][3][GTO] = TblKikiIntercept[0][3][GKI];
	TblKikiIntercept[0][3][GNY] = TblKikiIntercept[0][3][GKI];
	TblKikiIntercept[0][3][GNK] = TblKikiIntercept[0][3][GKI];
	TblKikiIntercept[0][3][GNG] = TblKikiIntercept[0][3][GKI];
	TblKikiIntercept[0][3][GUM] = TblKikiIntercept[0][3][SUM];
	TblKikiIntercept[0][3][GRY] = TblKikiIntercept[0][3][SRY];

	TblKikiIntercept[0][6][SFU] = (KIKI04);
	TblKikiIntercept[0][6][SKY] = (KIKI04);
	TblKikiIntercept[0][6][SKE] = (KIKI02 | KIKI04);
	TblKikiIntercept[0][6][SGI] = (KIKI04);
	TblKikiIntercept[0][6][SKI] = (KIKI04);
	TblKikiIntercept[0][6][SKA] = (KIKI02 | KIKI04);
	TblKikiIntercept[0][6][SHI] = 0;
	TblKikiIntercept[0][6][STO] = TblKikiIntercept[0][6][SKI];
	TblKikiIntercept[0][6][SNY] = TblKikiIntercept[0][6][SKI];
	TblKikiIntercept[0][6][SNK] = TblKikiIntercept[0][6][SKI];
	TblKikiIntercept[0][6][SNG] = TblKikiIntercept[0][6][SKI];
	TblKikiIntercept[0][6][SUM] = (KIKI04);
	TblKikiIntercept[0][6][SRY] = 0;

	TblKikiIntercept[0][6][GFU] = (KIKI02 | KIKI04);
	TblKikiIntercept[0][6][GKY] = (KIKI02 | KIKI04);
	TblKikiIntercept[0][6][GKE] = (KIKI02 | KIKI04);
	TblKikiIntercept[0][6][GGI] = (KIKI02 | KIKI04);
	TblKikiIntercept[0][6][GKI] = (KIKI04);
	TblKikiIntercept[0][6][GKA] = TblKikiIntercept[0][6][SKA];
	TblKikiIntercept[0][6][GHI] = TblKikiIntercept[0][6][SHI];
	TblKikiIntercept[0][6][GTO] = TblKikiIntercept[0][6][GKI];
	TblKikiIntercept[0][6][GNY] = TblKikiIntercept[0][6][GKI];
	TblKikiIntercept[0][6][GNK] = TblKikiIntercept[0][6][GKI];
	TblKikiIntercept[0][6][GNG] = TblKikiIntercept[0][6][GKI];
	TblKikiIntercept[0][6][GUM] = TblKikiIntercept[0][6][SUM];
	TblKikiIntercept[0][6][GRY] = TblKikiIntercept[0][6][SRY];

	TblKikiIntercept[0][7][SFU] = (KIKI05);
	TblKikiIntercept[0][7][SKY] = (KIKI05);
	TblKikiIntercept[0][7][SKE] = (KIKI03 | KIKI05);
	TblKikiIntercept[0][7][SGI] = (KIKI05);
	TblKikiIntercept[0][7][SKI] = (KIKI05);
	TblKikiIntercept[0][7][SKA] = (KIKI03 | KIKI05);
	TblKikiIntercept[0][7][SHI] = 0;
	TblKikiIntercept[0][7][STO] = TblKikiIntercept[0][7][SKI];
	TblKikiIntercept[0][7][SNY] = TblKikiIntercept[0][7][SKI];
	TblKikiIntercept[0][7][SNK] = TblKikiIntercept[0][7][SKI];
	TblKikiIntercept[0][7][SNG] = TblKikiIntercept[0][7][SKI];
	TblKikiIntercept[0][7][SUM] = (KIKI05);
	TblKikiIntercept[0][7][SRY] = 0;

	TblKikiIntercept[0][7][GFU] = (KIKI03 | KIKI05);
	TblKikiIntercept[0][7][GKY] = (KIKI03 | KIKI05);
	TblKikiIntercept[0][7][GKE] = (KIKI03 | KIKI05);
	TblKikiIntercept[0][7][GGI] = (KIKI03 | KIKI05);
	TblKikiIntercept[0][7][GKI] = (KIKI05);
	TblKikiIntercept[0][7][GKA] = TblKikiIntercept[0][7][SKA];
	TblKikiIntercept[0][7][GHI] = TblKikiIntercept[0][7][SHI];
	TblKikiIntercept[0][7][GTO] = TblKikiIntercept[0][7][GKI];
	TblKikiIntercept[0][7][GNY] = TblKikiIntercept[0][7][GKI];
	TblKikiIntercept[0][7][GNK] = TblKikiIntercept[0][7][GKI];
	TblKikiIntercept[0][7][GNG] = TblKikiIntercept[0][7][GKI];
	TblKikiIntercept[0][7][GUM] = TblKikiIntercept[0][7][SUM];
	TblKikiIntercept[0][7][GRY] = TblKikiIntercept[0][7][SRY];

	TblKikiIntercept[0][10][SKE] = (KIKI02 | KIKI04 | KIKI06);
	TblKikiIntercept[0][11][SKE] = (KIKI03 | KIKI05 | KIKI07);

	// ����������Dir01���� �F��̕�����02, 03, 04, 05, 08, 09���`�F�b�N����
	TblKikiIntercept[1][2][SFU] = (KIKI06);
	TblKikiIntercept[1][2][SKY] = (KIKI06);
	TblKikiIntercept[1][2][SKE] = (KIKI06);
	TblKikiIntercept[1][2][SGI] = (KIKI06);
	TblKikiIntercept[1][2][SKI] = 0;
	TblKikiIntercept[1][2][SKA] = (KIKI06);
	TblKikiIntercept[1][2][SHI] = 0;
	TblKikiIntercept[1][2][STO] = TblKikiIntercept[1][2][SKI];
	TblKikiIntercept[1][2][SNY] = TblKikiIntercept[1][2][SKI];
	TblKikiIntercept[1][2][SNK] = TblKikiIntercept[1][2][SKI];
	TblKikiIntercept[1][2][SNG] = TblKikiIntercept[1][2][SKI];
	TblKikiIntercept[1][2][SUM] = 0;
	TblKikiIntercept[1][2][SRY] = 0;

	TblKikiIntercept[1][2][GFU] = 0;
	TblKikiIntercept[1][2][GKY] = 0;
	TblKikiIntercept[1][2][GKE] = (KIKI06);
	TblKikiIntercept[1][2][GGI] = 0;
	TblKikiIntercept[1][2][GKI] = 0;
	TblKikiIntercept[1][2][GKA] = TblKikiIntercept[1][2][SKA];
	TblKikiIntercept[1][2][GHI] = TblKikiIntercept[1][2][SHI];
	TblKikiIntercept[1][2][GTO] = TblKikiIntercept[1][2][GKI];
	TblKikiIntercept[1][2][GNY] = TblKikiIntercept[1][2][GKI];
	TblKikiIntercept[1][2][GNK] = TblKikiIntercept[1][2][GKI];
	TblKikiIntercept[1][2][GNG] = TblKikiIntercept[1][2][GKI];
	TblKikiIntercept[1][2][GUM] = TblKikiIntercept[1][2][SUM];
	TblKikiIntercept[1][2][GRY] = TblKikiIntercept[1][2][SRY];

	TblKikiIntercept[1][3][SFU] = (KIKI07);
	TblKikiIntercept[1][3][SKY] = (KIKI07);
	TblKikiIntercept[1][3][SKE] = (KIKI07);
	TblKikiIntercept[1][3][SGI] = (KIKI07);
	TblKikiIntercept[1][3][SKI] = 0;
	TblKikiIntercept[1][3][SKA] = (KIKI07);
	TblKikiIntercept[1][3][SHI] = 0;
	TblKikiIntercept[1][3][STO] = TblKikiIntercept[1][3][SKI];
	TblKikiIntercept[1][3][SNY] = TblKikiIntercept[1][3][SKI];
	TblKikiIntercept[1][3][SNK] = TblKikiIntercept[1][3][SKI];
	TblKikiIntercept[1][3][SNG] = TblKikiIntercept[1][3][SKI];
	TblKikiIntercept[1][3][SUM] = 0;
	TblKikiIntercept[1][3][SRY] = 0;

	TblKikiIntercept[1][3][GFU] = 0;
	TblKikiIntercept[1][3][GKY] = 0;
	TblKikiIntercept[1][3][GKE] = (KIKI05);
	TblKikiIntercept[1][3][GGI] = 0;
	TblKikiIntercept[1][3][GKI] = 0;
	TblKikiIntercept[1][3][GKA] = TblKikiIntercept[1][3][SKA];
	TblKikiIntercept[1][3][GHI] = TblKikiIntercept[1][3][SHI];
	TblKikiIntercept[1][3][GTO] = TblKikiIntercept[1][3][GKI];
	TblKikiIntercept[1][3][GNY] = TblKikiIntercept[1][3][GKI];
	TblKikiIntercept[1][3][GNK] = TblKikiIntercept[1][3][GKI];
	TblKikiIntercept[1][3][GNG] = TblKikiIntercept[1][3][GKI];
	TblKikiIntercept[1][3][GUM] = TblKikiIntercept[1][3][SUM];
	TblKikiIntercept[1][3][GRY] = TblKikiIntercept[1][3][SRY];

	TblKikiIntercept[1][4][SFU] = (KIKI02 | KIKI06);
	TblKikiIntercept[1][4][SKY] = (KIKI02 | KIKI06);
	TblKikiIntercept[1][4][SKE] = (KIKI02 | KIKI06);
	TblKikiIntercept[1][4][SGI] = (KIKI02 | KIKI06);
	TblKikiIntercept[1][4][SKI] = (KIKI06);
	TblKikiIntercept[1][4][SKA] = (KIKI02 | KIKI06);
	TblKikiIntercept[1][4][SHI] = 0;
	TblKikiIntercept[1][4][STO] = TblKikiIntercept[1][4][SKI];
	TblKikiIntercept[1][4][SNY] = TblKikiIntercept[1][4][SKI];
	TblKikiIntercept[1][4][SNK] = TblKikiIntercept[1][4][SKI];
	TblKikiIntercept[1][4][SNG] = TblKikiIntercept[1][4][SKI];
	TblKikiIntercept[1][4][SUM] = (KIKI06);
	TblKikiIntercept[1][4][SRY] = 0;

	TblKikiIntercept[1][4][GFU] = (KIKI06);
	TblKikiIntercept[1][4][GKY] = (KIKI06);
	TblKikiIntercept[1][4][GKE] = (KIKI02 | KIKI06);
	TblKikiIntercept[1][4][GGI] = (KIKI06);
	TblKikiIntercept[1][4][GKI] = (KIKI06);
	TblKikiIntercept[1][4][GKA] = TblKikiIntercept[1][4][SKA];
	TblKikiIntercept[1][4][GHI] = TblKikiIntercept[1][4][SHI];
	TblKikiIntercept[1][4][GTO] = TblKikiIntercept[1][4][GKI];
	TblKikiIntercept[1][4][GNY] = TblKikiIntercept[1][4][GKI];
	TblKikiIntercept[1][4][GNK] = TblKikiIntercept[1][4][GKI];
	TblKikiIntercept[1][4][GNG] = TblKikiIntercept[1][4][GKI];
	TblKikiIntercept[1][4][GUM] = TblKikiIntercept[1][4][SUM];
	TblKikiIntercept[1][4][GRY] = TblKikiIntercept[1][4][SRY];

	TblKikiIntercept[1][5][SFU] = (KIKI03 | KIKI07);
	TblKikiIntercept[1][5][SKY] = (KIKI03 | KIKI07);
	TblKikiIntercept[1][5][SKE] = (KIKI03 | KIKI07);
	TblKikiIntercept[1][5][SGI] = (KIKI03 | KIKI07);
	TblKikiIntercept[1][5][SKI] = (KIKI07);
	TblKikiIntercept[1][5][SKA] = (KIKI03 | KIKI07);
	TblKikiIntercept[1][5][SHI] = 0;
	TblKikiIntercept[1][5][STO] = TblKikiIntercept[1][5][SKI];
	TblKikiIntercept[1][5][SNY] = TblKikiIntercept[1][5][SKI];
	TblKikiIntercept[1][5][SNK] = TblKikiIntercept[1][5][SKI];
	TblKikiIntercept[1][5][SNG] = TblKikiIntercept[1][5][SKI];
	TblKikiIntercept[1][5][SUM] = (KIKI07);
	TblKikiIntercept[1][5][SRY] = 0;

	TblKikiIntercept[1][5][GFU] = (KIKI07);
	TblKikiIntercept[1][5][GKY] = (KIKI07);
	TblKikiIntercept[1][5][GKE] = (KIKI03 | KIKI07);
	TblKikiIntercept[1][5][GGI] = (KIKI07);
	TblKikiIntercept[1][5][GKI] = (KIKI07);
	TblKikiIntercept[1][5][GKA] = TblKikiIntercept[1][5][SKA];
	TblKikiIntercept[1][5][GHI] = TblKikiIntercept[1][5][SHI];
	TblKikiIntercept[1][5][GTO] = TblKikiIntercept[1][5][GKI];
	TblKikiIntercept[1][5][GNY] = TblKikiIntercept[1][5][GKI];
	TblKikiIntercept[1][5][GNK] = TblKikiIntercept[1][5][GKI];
	TblKikiIntercept[1][5][GNG] = TblKikiIntercept[1][5][GKI];
	TblKikiIntercept[1][5][GUM] = TblKikiIntercept[1][5][SUM];
	TblKikiIntercept[1][5][GRY] = TblKikiIntercept[1][5][SRY];

	TblKikiIntercept[1][8][GKE] = (KIKI02 | KIKI04 | KIKI06);
	TblKikiIntercept[1][9][GKE] = (KIKI03 | KIKI05 | KIKI07);

	// ����������Dir02���� �F��̕�����00, 01, 05, 07���`�F�b�N����
	TblKikiIntercept[2][0][SFU] = (KIKI04);
	TblKikiIntercept[2][0][SKY] = (KIKI04);
	TblKikiIntercept[2][0][SKE] = (KIKI04);
	TblKikiIntercept[2][0][SGI] = (KIKI04);
	TblKikiIntercept[2][0][SKI] = 0;
	TblKikiIntercept[2][0][SKA] = (KIKI04);
	TblKikiIntercept[2][0][SHI] = 0;
	TblKikiIntercept[2][0][STO] = TblKikiIntercept[2][0][SKI];
	TblKikiIntercept[2][0][SNY] = TblKikiIntercept[2][0][SKI];
	TblKikiIntercept[2][0][SNK] = TblKikiIntercept[2][0][SKI];
	TblKikiIntercept[2][0][SNG] = TblKikiIntercept[2][0][SKI];
	TblKikiIntercept[2][0][SUM] = 0;
	TblKikiIntercept[2][0][SRY] = 0;

	TblKikiIntercept[2][0][GFU] = TblKikiIntercept[2][0][SFU];
	TblKikiIntercept[2][0][GKY] = TblKikiIntercept[2][0][SKY];
	TblKikiIntercept[2][0][GKE] = TblKikiIntercept[2][0][SKE];
	TblKikiIntercept[2][0][GGI] = TblKikiIntercept[2][0][SGI];
	TblKikiIntercept[2][0][GKI] = TblKikiIntercept[2][0][SKI];
	TblKikiIntercept[2][0][GKA] = TblKikiIntercept[2][0][SKA];
	TblKikiIntercept[2][0][GHI] = TblKikiIntercept[2][0][SHI];
	TblKikiIntercept[2][0][GTO] = TblKikiIntercept[2][0][GKI];
	TblKikiIntercept[2][0][GNY] = TblKikiIntercept[2][0][GKI];
	TblKikiIntercept[2][0][GNK] = TblKikiIntercept[2][0][GKI];
	TblKikiIntercept[2][0][GNG] = TblKikiIntercept[2][0][GKI];
	TblKikiIntercept[2][0][GUM] = TblKikiIntercept[2][0][SUM];
	TblKikiIntercept[2][0][GRY] = TblKikiIntercept[2][0][SRY];

	TblKikiIntercept[2][1][SFU] = (KIKI06);
	TblKikiIntercept[2][1][SKY] = (KIKI06);
	TblKikiIntercept[2][1][SKE] = (KIKI06);
	TblKikiIntercept[2][1][SGI] = (KIKI06);
	TblKikiIntercept[2][1][SKI] = 0;
	TblKikiIntercept[2][1][SKA] = (KIKI06);
	TblKikiIntercept[2][1][SHI] = 0;
	TblKikiIntercept[2][1][STO] = TblKikiIntercept[2][1][SKI];
	TblKikiIntercept[2][1][SNY] = TblKikiIntercept[2][1][SKI];
	TblKikiIntercept[2][1][SNK] = TblKikiIntercept[2][1][SKI];
	TblKikiIntercept[2][1][SNG] = TblKikiIntercept[2][1][SKI];
	TblKikiIntercept[2][1][SUM] = 0;
	TblKikiIntercept[2][1][SRY] = 0;

	TblKikiIntercept[2][1][GFU] = TblKikiIntercept[2][1][SFU];
	TblKikiIntercept[2][1][GKY] = TblKikiIntercept[2][1][SKY];
	TblKikiIntercept[2][1][GKE] = TblKikiIntercept[2][1][SKE];
	TblKikiIntercept[2][1][GGI] = TblKikiIntercept[2][1][SGI];
	TblKikiIntercept[2][1][GKI] = TblKikiIntercept[2][1][SKI];
	TblKikiIntercept[2][1][GKA] = TblKikiIntercept[2][1][SKA];
	TblKikiIntercept[2][1][GHI] = TblKikiIntercept[2][1][SHI];
	TblKikiIntercept[2][1][GTO] = TblKikiIntercept[2][1][GKI];
	TblKikiIntercept[2][1][GNY] = TblKikiIntercept[2][1][GKI];
	TblKikiIntercept[2][1][GNK] = TblKikiIntercept[2][1][GKI];
	TblKikiIntercept[2][1][GNG] = TblKikiIntercept[2][1][GKI];
	TblKikiIntercept[2][1][GUM] = TblKikiIntercept[2][1][SUM];
	TblKikiIntercept[2][1][GRY] = TblKikiIntercept[2][1][SRY];

	TblKikiIntercept[2][5][SFU] = (KIKI00 | KIKI04);
	TblKikiIntercept[2][5][SKY] = (KIKI00 | KIKI04);
	TblKikiIntercept[2][5][SKE] = (KIKI00 | KIKI04);
	TblKikiIntercept[2][5][SGI] = (KIKI00 | KIKI04);
	TblKikiIntercept[2][5][SKI] = (KIKI04);
	TblKikiIntercept[2][5][SKA] = (KIKI00 | KIKI04);
	TblKikiIntercept[2][5][SHI] = 0;
	TblKikiIntercept[2][5][STO] = TblKikiIntercept[2][5][SKI];
	TblKikiIntercept[2][5][SNY] = TblKikiIntercept[2][5][SKI];
	TblKikiIntercept[2][5][SNK] = TblKikiIntercept[2][5][SKI];
	TblKikiIntercept[2][5][SNG] = TblKikiIntercept[2][5][SKI];
	TblKikiIntercept[2][5][SUM] = (KIKI04);
	TblKikiIntercept[2][5][SRY] = 0;

	TblKikiIntercept[2][5][GFU] = TblKikiIntercept[2][5][SFU];
	TblKikiIntercept[2][5][GKY] = TblKikiIntercept[2][5][SKY];
	TblKikiIntercept[2][5][GKE] = TblKikiIntercept[2][5][SKE];
	TblKikiIntercept[2][5][GGI] = TblKikiIntercept[2][5][SGI];
	TblKikiIntercept[2][5][GKI] = TblKikiIntercept[2][5][SKI];
	TblKikiIntercept[2][5][GKA] = TblKikiIntercept[2][5][SKA];
	TblKikiIntercept[2][5][GHI] = TblKikiIntercept[2][5][SHI];
	TblKikiIntercept[2][5][GTO] = TblKikiIntercept[2][5][GKI];
	TblKikiIntercept[2][5][GNY] = TblKikiIntercept[2][5][GKI];
	TblKikiIntercept[2][5][GNK] = TblKikiIntercept[2][5][GKI];
	TblKikiIntercept[2][5][GNG] = TblKikiIntercept[2][5][GKI];
	TblKikiIntercept[2][5][GUM] = TblKikiIntercept[2][5][SUM];
	TblKikiIntercept[2][5][GRY] = TblKikiIntercept[2][5][SRY];

	TblKikiIntercept[2][7][SFU] = (KIKI01 | KIKI06);
	TblKikiIntercept[2][7][SKY] = (KIKI01 | KIKI06);
	TblKikiIntercept[2][7][SKE] = (KIKI01 | KIKI06);
	TblKikiIntercept[2][7][SGI] = (KIKI01 | KIKI06);
	TblKikiIntercept[2][7][SKI] = (KIKI06);
	TblKikiIntercept[2][7][SKA] = (KIKI01 | KIKI06);
	TblKikiIntercept[2][7][SHI] = 0;
	TblKikiIntercept[2][7][STO] = TblKikiIntercept[2][7][SKI];
	TblKikiIntercept[2][7][SNY] = TblKikiIntercept[2][7][SKI];
	TblKikiIntercept[2][7][SNK] = TblKikiIntercept[2][7][SKI];
	TblKikiIntercept[2][7][SNG] = TblKikiIntercept[2][7][SKI];
	TblKikiIntercept[2][7][SUM] = (KIKI06);
	TblKikiIntercept[2][7][SRY] = 0;

	TblKikiIntercept[2][7][GFU] = TblKikiIntercept[2][7][SFU];
	TblKikiIntercept[2][7][GKY] = TblKikiIntercept[2][7][SKY];
	TblKikiIntercept[2][7][GKE] = TblKikiIntercept[2][7][SKE];
	TblKikiIntercept[2][7][GGI] = TblKikiIntercept[2][7][SGI];
	TblKikiIntercept[2][7][GKI] = TblKikiIntercept[2][7][SKI];
	TblKikiIntercept[2][7][GKA] = TblKikiIntercept[2][7][SKA];
	TblKikiIntercept[2][7][GHI] = TblKikiIntercept[2][7][SHI];
	TblKikiIntercept[2][7][GTO] = TblKikiIntercept[2][7][GKI];
	TblKikiIntercept[2][7][GNY] = TblKikiIntercept[2][7][GKI];
	TblKikiIntercept[2][7][GNK] = TblKikiIntercept[2][7][GKI];
	TblKikiIntercept[2][7][GNG] = TblKikiIntercept[2][7][GKI];
	TblKikiIntercept[2][7][GUM] = TblKikiIntercept[2][7][SUM];
	TblKikiIntercept[2][7][GRY] = TblKikiIntercept[2][7][SRY];

	// ����������Dir03���� �F��̕�����00, 01, 04, 06���`�F�b�N����
	TblKikiIntercept[3][0][SFU] = (KIKI05);
	TblKikiIntercept[3][0][SKY] = (KIKI05);
	TblKikiIntercept[3][0][SKE] = (KIKI05);
	TblKikiIntercept[3][0][SGI] = (KIKI05);
	TblKikiIntercept[3][0][SKI] = 0;
	TblKikiIntercept[3][0][SKA] = (KIKI05);
	TblKikiIntercept[3][0][SHI] = 0;
	TblKikiIntercept[3][0][STO] = TblKikiIntercept[3][0][SKI];
	TblKikiIntercept[3][0][SNY] = TblKikiIntercept[3][0][SKI];
	TblKikiIntercept[3][0][SNK] = TblKikiIntercept[3][0][SKI];
	TblKikiIntercept[3][0][SNG] = TblKikiIntercept[3][0][SKI];
	TblKikiIntercept[3][0][SUM] = 0;
	TblKikiIntercept[3][0][SRY] = 0;

	TblKikiIntercept[3][0][GFU] = TblKikiIntercept[3][0][SFU];
	TblKikiIntercept[3][0][GKY] = TblKikiIntercept[3][0][SKY];
	TblKikiIntercept[3][0][GKE] = TblKikiIntercept[3][0][SKE];
	TblKikiIntercept[3][0][GGI] = TblKikiIntercept[3][0][SGI];
	TblKikiIntercept[3][0][GKI] = TblKikiIntercept[3][0][SKI];
	TblKikiIntercept[3][0][GKA] = TblKikiIntercept[3][0][SKA];
	TblKikiIntercept[3][0][GHI] = TblKikiIntercept[3][0][SHI];
	TblKikiIntercept[3][0][GTO] = TblKikiIntercept[3][0][GKI];
	TblKikiIntercept[3][0][GNY] = TblKikiIntercept[3][0][GKI];
	TblKikiIntercept[3][0][GNK] = TblKikiIntercept[3][0][GKI];
	TblKikiIntercept[3][0][GNG] = TblKikiIntercept[3][0][GKI];
	TblKikiIntercept[3][0][GUM] = TblKikiIntercept[3][0][SUM];
	TblKikiIntercept[3][0][GRY] = TblKikiIntercept[3][0][SRY];

	TblKikiIntercept[3][1][SFU] = (KIKI07);
	TblKikiIntercept[3][1][SKY] = (KIKI07);
	TblKikiIntercept[3][1][SKE] = (KIKI07);
	TblKikiIntercept[3][1][SGI] = (KIKI07);
	TblKikiIntercept[3][1][SKI] = 0;
	TblKikiIntercept[3][1][SKA] = (KIKI07);
	TblKikiIntercept[3][1][SHI] = 0;
	TblKikiIntercept[3][1][STO] = TblKikiIntercept[3][1][SKI];
	TblKikiIntercept[3][1][SNY] = TblKikiIntercept[3][1][SKI];
	TblKikiIntercept[3][1][SNK] = TblKikiIntercept[3][1][SKI];
	TblKikiIntercept[3][1][SNG] = TblKikiIntercept[3][1][SKI];
	TblKikiIntercept[3][1][SUM] = 0;
	TblKikiIntercept[3][1][SRY] = 0;

	TblKikiIntercept[3][1][GFU] = TblKikiIntercept[3][1][SFU];
	TblKikiIntercept[3][1][GKY] = TblKikiIntercept[3][1][SKY];
	TblKikiIntercept[3][1][GKE] = TblKikiIntercept[3][1][SKE];
	TblKikiIntercept[3][1][GGI] = TblKikiIntercept[3][1][SGI];
	TblKikiIntercept[3][1][GKI] = TblKikiIntercept[3][1][SKI];
	TblKikiIntercept[3][1][GKA] = TblKikiIntercept[3][1][SKA];
	TblKikiIntercept[3][1][GHI] = TblKikiIntercept[3][1][SHI];
	TblKikiIntercept[3][1][GTO] = TblKikiIntercept[3][1][GKI];
	TblKikiIntercept[3][1][GNY] = TblKikiIntercept[3][1][GKI];
	TblKikiIntercept[3][1][GNK] = TblKikiIntercept[3][1][GKI];
	TblKikiIntercept[3][1][GNG] = TblKikiIntercept[3][1][GKI];
	TblKikiIntercept[3][1][GUM] = TblKikiIntercept[3][1][SUM];
	TblKikiIntercept[3][1][GRY] = TblKikiIntercept[3][1][SRY];

	TblKikiIntercept[3][4][SFU] = (KIKI00 | KIKI05);
	TblKikiIntercept[3][4][SKY] = (KIKI00 | KIKI05);
	TblKikiIntercept[3][4][SKE] = (KIKI00 | KIKI05);
	TblKikiIntercept[3][4][SGI] = (KIKI00 | KIKI05);
	TblKikiIntercept[3][4][SKI] = (KIKI05);
	TblKikiIntercept[3][4][SKA] = (KIKI00 | KIKI05);
	TblKikiIntercept[3][4][SHI] = 0;
	TblKikiIntercept[3][4][STO] = TblKikiIntercept[3][4][SKI];
	TblKikiIntercept[3][4][SNY] = TblKikiIntercept[3][4][SKI];
	TblKikiIntercept[3][4][SNK] = TblKikiIntercept[3][4][SKI];
	TblKikiIntercept[3][4][SNG] = TblKikiIntercept[3][4][SKI];
	TblKikiIntercept[3][4][SUM] = (KIKI05);
	TblKikiIntercept[3][4][SRY] = 0;

	TblKikiIntercept[3][4][GFU] = TblKikiIntercept[3][4][SFU];
	TblKikiIntercept[3][4][GKY] = TblKikiIntercept[3][4][SKY];
	TblKikiIntercept[3][4][GKE] = TblKikiIntercept[3][4][SKE];
	TblKikiIntercept[3][4][GGI] = TblKikiIntercept[3][4][SGI];
	TblKikiIntercept[3][4][GKI] = TblKikiIntercept[3][4][SKI];
	TblKikiIntercept[3][4][GKA] = TblKikiIntercept[3][4][SKA];
	TblKikiIntercept[3][4][GHI] = TblKikiIntercept[3][4][SHI];
	TblKikiIntercept[3][4][GTO] = TblKikiIntercept[3][4][GKI];
	TblKikiIntercept[3][4][GNY] = TblKikiIntercept[3][4][GKI];
	TblKikiIntercept[3][4][GNK] = TblKikiIntercept[3][4][GKI];
	TblKikiIntercept[3][4][GNG] = TblKikiIntercept[3][4][GKI];
	TblKikiIntercept[3][4][GUM] = TblKikiIntercept[3][4][SUM];
	TblKikiIntercept[3][4][GRY] = TblKikiIntercept[3][4][SRY];

	TblKikiIntercept[3][6][SFU] = (KIKI01 | KIKI07);
	TblKikiIntercept[3][6][SKY] = (KIKI01 | KIKI07);
	TblKikiIntercept[3][6][SKE] = (KIKI01 | KIKI07);
	TblKikiIntercept[3][6][SGI] = (KIKI01 | KIKI07);
	TblKikiIntercept[3][6][SKI] = (KIKI07);
	TblKikiIntercept[3][6][SKA] = (KIKI01 | KIKI07);
	TblKikiIntercept[3][6][SHI] = 0;
	TblKikiIntercept[3][6][STO] = TblKikiIntercept[3][6][SKI];
	TblKikiIntercept[3][6][SNY] = TblKikiIntercept[3][6][SKI];
	TblKikiIntercept[3][6][SNK] = TblKikiIntercept[3][6][SKI];
	TblKikiIntercept[3][6][SNG] = TblKikiIntercept[3][6][SKI];
	TblKikiIntercept[3][6][SUM] = (KIKI07);
	TblKikiIntercept[3][6][SRY] = 0;

	TblKikiIntercept[3][6][GFU] = TblKikiIntercept[3][6][SFU];
	TblKikiIntercept[3][6][GKY] = TblKikiIntercept[3][6][SKY];
	TblKikiIntercept[3][6][GKE] = TblKikiIntercept[3][6][SKE];
	TblKikiIntercept[3][6][GGI] = TblKikiIntercept[3][6][SGI];
	TblKikiIntercept[3][6][GKI] = TblKikiIntercept[3][6][SKI];
	TblKikiIntercept[3][6][GKA] = TblKikiIntercept[3][6][SKA];
	TblKikiIntercept[3][6][GHI] = TblKikiIntercept[3][6][SHI];
	TblKikiIntercept[3][6][GTO] = TblKikiIntercept[3][6][GKI];
	TblKikiIntercept[3][6][GNY] = TblKikiIntercept[3][6][GKI];
	TblKikiIntercept[3][6][GNK] = TblKikiIntercept[3][6][GKI];
	TblKikiIntercept[3][6][GNG] = TblKikiIntercept[3][6][GKI];
	TblKikiIntercept[3][6][GUM] = TblKikiIntercept[3][6][SUM];
	TblKikiIntercept[3][6][GRY] = TblKikiIntercept[3][6][SRY];

	// ����������Dir04���� �F��̕�����01, 03, 11���`�F�b�N����
	TblKikiIntercept[4][1][SFU] = (KIKI02);
	TblKikiIntercept[4][1][SKY] = (KIKI02);
	TblKikiIntercept[4][1][SKE] = (KIKI02);
	TblKikiIntercept[4][1][SGI] = 0;
	TblKikiIntercept[4][1][SKI] = 0;
	TblKikiIntercept[4][1][SKA] = 0;
	TblKikiIntercept[4][1][SHI] = (KIKI02);
	TblKikiIntercept[4][1][STO] = TblKikiIntercept[4][1][SKI];
	TblKikiIntercept[4][1][SNY] = TblKikiIntercept[4][1][SKI];
	TblKikiIntercept[4][1][SNK] = TblKikiIntercept[4][1][SKI];
	TblKikiIntercept[4][1][SNG] = TblKikiIntercept[4][1][SKI];
	TblKikiIntercept[4][1][SUM] = 0;
	TblKikiIntercept[4][1][SRY] = 0;

	TblKikiIntercept[4][1][GFU] = (KIKI02);
	TblKikiIntercept[4][1][GKY] = (KIKI02);
	TblKikiIntercept[4][1][GKE] = (KIKI02);
	TblKikiIntercept[4][1][GGI] = 0;
	TblKikiIntercept[4][1][GKI] = (KIKI02);
	TblKikiIntercept[4][1][GKA] = TblKikiIntercept[4][1][SKA];
	TblKikiIntercept[4][1][GHI] = TblKikiIntercept[4][1][SHI];
	TblKikiIntercept[4][1][GTO] = TblKikiIntercept[4][1][GKI];
	TblKikiIntercept[4][1][GNY] = TblKikiIntercept[4][1][GKI];
	TblKikiIntercept[4][1][GNK] = TblKikiIntercept[4][1][GKI];
	TblKikiIntercept[4][1][GNG] = TblKikiIntercept[4][1][GKI];
	TblKikiIntercept[4][1][GUM] = TblKikiIntercept[4][1][SUM];
	TblKikiIntercept[4][1][GRY] = TblKikiIntercept[4][1][SRY];

	TblKikiIntercept[4][3][SFU] = (KIKI00);
	TblKikiIntercept[4][3][SKY] = (KIKI00);
	TblKikiIntercept[4][3][SKE] = (KIKI00);
	TblKikiIntercept[4][3][SGI] = 0;
	TblKikiIntercept[4][3][SKI] = 0;
	TblKikiIntercept[4][3][SKA] = 0;
	TblKikiIntercept[4][3][SHI] = (KIKI00);
	TblKikiIntercept[4][3][STO] = TblKikiIntercept[4][3][SKI];
	TblKikiIntercept[4][3][SNY] = TblKikiIntercept[4][3][SKI];
	TblKikiIntercept[4][3][SNK] = TblKikiIntercept[4][3][SKI];
	TblKikiIntercept[4][3][SNG] = TblKikiIntercept[4][3][SKI];
	TblKikiIntercept[4][3][SUM] = 0;
	TblKikiIntercept[4][3][SRY] = 0;

	TblKikiIntercept[4][3][GFU] = (KIKI00);
	TblKikiIntercept[4][3][GKY] = (KIKI00);
	TblKikiIntercept[4][3][GKE] = (KIKI00);
	TblKikiIntercept[4][3][GGI] = 0;
	TblKikiIntercept[4][3][GKI] = (KIKI00);
	TblKikiIntercept[4][3][GKA] = TblKikiIntercept[4][3][SKA];
	TblKikiIntercept[4][3][GHI] = TblKikiIntercept[4][3][SHI];
	TblKikiIntercept[4][3][GTO] = TblKikiIntercept[4][3][GKI];
	TblKikiIntercept[4][3][GNY] = TblKikiIntercept[4][3][GKI];
	TblKikiIntercept[4][3][GNK] = TblKikiIntercept[4][3][GKI];
	TblKikiIntercept[4][3][GNG] = TblKikiIntercept[4][3][GKI];
	TblKikiIntercept[4][3][GUM] = TblKikiIntercept[4][3][SUM];
	TblKikiIntercept[4][3][GRY] = TblKikiIntercept[4][3][SRY];

	// ����������Dir04���� �F��̕�����01, 03, 11���`�F�b�N����
	TblKikiIntercept[4][11][SFU] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][SKY] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][SKE] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][SGI] = (KIKI02);
	TblKikiIntercept[4][11][SKI] = (KIKI02);
	TblKikiIntercept[4][11][SKA] = 0;
	TblKikiIntercept[4][11][SHI] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][STO] = TblKikiIntercept[4][11][SKI];
	TblKikiIntercept[4][11][SNY] = TblKikiIntercept[4][11][SKI];
	TblKikiIntercept[4][11][SNK] = TblKikiIntercept[4][11][SKI];
	TblKikiIntercept[4][11][SNG] = TblKikiIntercept[4][11][SKI];
	TblKikiIntercept[4][11][SUM] = 0;
	TblKikiIntercept[4][11][SRY] = (KIKI02);

	TblKikiIntercept[4][11][GFU] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][GKY] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][GKE] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][GGI] = (KIKI02);
	TblKikiIntercept[4][11][GKI] = (KIKI01 | KIKI02);
	TblKikiIntercept[4][11][GKA] = TblKikiIntercept[4][11][SKA];
	TblKikiIntercept[4][11][GHI] = TblKikiIntercept[4][11][SHI];
	TblKikiIntercept[4][11][GTO] = TblKikiIntercept[4][11][GKI];
	TblKikiIntercept[4][11][GNY] = TblKikiIntercept[4][11][GKI];
	TblKikiIntercept[4][11][GNK] = TblKikiIntercept[4][11][GKI];
	TblKikiIntercept[4][11][GNG] = TblKikiIntercept[4][11][GKI];
	TblKikiIntercept[4][11][GUM] = TblKikiIntercept[4][11][SUM];
	TblKikiIntercept[4][11][GRY] = TblKikiIntercept[4][11][SRY];

	// ����������Dir05���� �F��̕�����01, 02, 10���`�F�b�N����
	TblKikiIntercept[5][1][SFU] = (KIKI03);
	TblKikiIntercept[5][1][SKY] = (KIKI03);
	TblKikiIntercept[5][1][SKE] = (KIKI03);
	TblKikiIntercept[5][1][SGI] = 0;
	TblKikiIntercept[5][1][SKI] = 0;
	TblKikiIntercept[5][1][SKA] = 0;
	TblKikiIntercept[5][1][SHI] = (KIKI03);
	TblKikiIntercept[5][1][STO] = TblKikiIntercept[5][1][SKI];
	TblKikiIntercept[5][1][SNY] = TblKikiIntercept[5][1][SKI];
	TblKikiIntercept[5][1][SNK] = TblKikiIntercept[5][1][SKI];
	TblKikiIntercept[5][1][SNG] = TblKikiIntercept[5][1][SKI];
	TblKikiIntercept[5][1][SUM] = 0;
	TblKikiIntercept[5][1][SRY] = 0;

	TblKikiIntercept[5][1][GFU] = (KIKI03);
	TblKikiIntercept[5][1][GKY] = (KIKI03);
	TblKikiIntercept[5][1][GKE] = (KIKI03);
	TblKikiIntercept[5][1][GGI] = 0;
	TblKikiIntercept[5][1][GKI] = (KIKI03);
	TblKikiIntercept[5][1][GKA] = TblKikiIntercept[5][1][SKA];
	TblKikiIntercept[5][1][GHI] = TblKikiIntercept[5][1][SHI];
	TblKikiIntercept[5][1][GTO] = TblKikiIntercept[5][1][GKI];
	TblKikiIntercept[5][1][GNY] = TblKikiIntercept[5][1][GKI];
	TblKikiIntercept[5][1][GNK] = TblKikiIntercept[5][1][GKI];
	TblKikiIntercept[5][1][GNG] = TblKikiIntercept[5][1][GKI];
	TblKikiIntercept[5][1][GUM] = TblKikiIntercept[5][1][SUM];
	TblKikiIntercept[5][1][GRY] = TblKikiIntercept[5][1][SRY];

	TblKikiIntercept[5][2][SFU] = (KIKI00);
	TblKikiIntercept[5][2][SKY] = (KIKI00);
	TblKikiIntercept[5][2][SKE] = (KIKI00);
	TblKikiIntercept[5][2][SGI] = 0;
	TblKikiIntercept[5][2][SKI] = 0;
	TblKikiIntercept[5][2][SKA] = 0;
	TblKikiIntercept[5][2][SHI] = (KIKI00);
	TblKikiIntercept[5][2][STO] = TblKikiIntercept[5][2][SKI];
	TblKikiIntercept[5][2][SNY] = TblKikiIntercept[5][2][SKI];
	TblKikiIntercept[5][2][SNK] = TblKikiIntercept[5][2][SKI];
	TblKikiIntercept[5][2][SNG] = TblKikiIntercept[5][2][SKI];
	TblKikiIntercept[5][2][SUM] = 0;
	TblKikiIntercept[5][2][SRY] = 0;

	TblKikiIntercept[5][2][GFU] = (KIKI00);
	TblKikiIntercept[5][2][GKY] = (KIKI00);
	TblKikiIntercept[5][2][GKE] = (KIKI00);
	TblKikiIntercept[5][2][GGI] = 0;
	TblKikiIntercept[5][2][GKI] = (KIKI00);
	TblKikiIntercept[5][2][GKA] = TblKikiIntercept[5][2][SKA];
	TblKikiIntercept[5][2][GHI] = TblKikiIntercept[5][2][SHI];
	TblKikiIntercept[5][2][GTO] = TblKikiIntercept[5][2][GKI];
	TblKikiIntercept[5][2][GNY] = TblKikiIntercept[5][2][GKI];
	TblKikiIntercept[5][2][GNK] = TblKikiIntercept[5][2][GKI];
	TblKikiIntercept[5][2][GNG] = TblKikiIntercept[5][2][GKI];
	TblKikiIntercept[5][2][GUM] = TblKikiIntercept[5][2][SUM];
	TblKikiIntercept[5][2][GRY] = TblKikiIntercept[5][2][SRY];

	// ����������Dir05���� �F��̕�����01, 02, 10���`�F�b�N����
	TblKikiIntercept[5][10][SFU] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][SKY] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][SKE] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][SGI] = (KIKI03);
	TblKikiIntercept[5][10][SKI] = (KIKI03);
	TblKikiIntercept[5][10][SKA] = 0;
	TblKikiIntercept[5][10][SHI] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][STO] = TblKikiIntercept[5][10][SKI];
	TblKikiIntercept[5][10][SNY] = TblKikiIntercept[5][10][SKI];
	TblKikiIntercept[5][10][SNK] = TblKikiIntercept[5][10][SKI];
	TblKikiIntercept[5][10][SNG] = TblKikiIntercept[5][10][SKI];
	TblKikiIntercept[5][10][SUM] = 0;
	TblKikiIntercept[5][10][SRY] = (KIKI03);

	TblKikiIntercept[5][10][GFU] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][GKY] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][GKE] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][GGI] = (KIKI03);
	TblKikiIntercept[5][10][GKI] = (KIKI01 | KIKI03);
	TblKikiIntercept[5][10][GKA] = TblKikiIntercept[5][10][SKA];
	TblKikiIntercept[5][10][GHI] = TblKikiIntercept[5][10][SHI];
	TblKikiIntercept[5][10][GTO] = TblKikiIntercept[5][10][GKI];
	TblKikiIntercept[5][10][GNY] = TblKikiIntercept[5][10][GKI];
	TblKikiIntercept[5][10][GNK] = TblKikiIntercept[5][10][GKI];
	TblKikiIntercept[5][10][GNG] = TblKikiIntercept[5][10][GKI];
	TblKikiIntercept[5][10][GUM] = TblKikiIntercept[5][10][SUM];
	TblKikiIntercept[5][10][GRY] = TblKikiIntercept[5][10][SRY];

	// ����������Dir06���� �F��̕�����00, 03, 09���`�F�b�N����
	TblKikiIntercept[6][0][SFU] = (KIKI02);
	TblKikiIntercept[6][0][SKY] = (KIKI02);
	TblKikiIntercept[6][0][SKE] = (KIKI02);
	TblKikiIntercept[6][0][SGI] = 0;
	TblKikiIntercept[6][0][SKI] = (KIKI02);
	TblKikiIntercept[6][0][SKA] = 0;
	TblKikiIntercept[6][0][SHI] = (KIKI02);
	TblKikiIntercept[6][0][STO] = TblKikiIntercept[6][0][SKI];
	TblKikiIntercept[6][0][SNY] = TblKikiIntercept[6][0][SKI];
	TblKikiIntercept[6][0][SNK] = TblKikiIntercept[6][0][SKI];
	TblKikiIntercept[6][0][SNG] = TblKikiIntercept[6][0][SKI];
	TblKikiIntercept[6][0][SUM] = 0;
	TblKikiIntercept[6][0][SRY] = 0;

	TblKikiIntercept[6][0][GFU] = (KIKI02);
	TblKikiIntercept[6][0][GKY] = (KIKI02);
	TblKikiIntercept[6][0][GKE] = (KIKI02);
	TblKikiIntercept[6][0][GGI] = 0;
	TblKikiIntercept[6][0][GKI] = 0;
	TblKikiIntercept[6][0][GKA] = TblKikiIntercept[6][0][SKA];
	TblKikiIntercept[6][0][GHI] = TblKikiIntercept[6][0][SHI];
	TblKikiIntercept[6][0][GTO] = TblKikiIntercept[6][0][GKI];
	TblKikiIntercept[6][0][GNY] = TblKikiIntercept[6][0][GKI];
	TblKikiIntercept[6][0][GNK] = TblKikiIntercept[6][0][GKI];
	TblKikiIntercept[6][0][GNG] = TblKikiIntercept[6][0][GKI];
	TblKikiIntercept[6][0][GUM] = TblKikiIntercept[6][0][SUM];
	TblKikiIntercept[6][0][GRY] = TblKikiIntercept[6][0][SRY];

	TblKikiIntercept[6][3][SFU] = (KIKI01);
	TblKikiIntercept[6][3][SKY] = (KIKI01);
	TblKikiIntercept[6][3][SKE] = (KIKI01);
	TblKikiIntercept[6][3][SGI] = 0;
	TblKikiIntercept[6][3][SKI] = (KIKI01);
	TblKikiIntercept[6][3][SKA] = 0;
	TblKikiIntercept[6][3][SHI] = (KIKI01);
	TblKikiIntercept[6][3][STO] = TblKikiIntercept[6][3][SKI];
	TblKikiIntercept[6][3][SNY] = TblKikiIntercept[6][3][SKI];
	TblKikiIntercept[6][3][SNK] = TblKikiIntercept[6][3][SKI];
	TblKikiIntercept[6][3][SNG] = TblKikiIntercept[6][3][SKI];
	TblKikiIntercept[6][3][SUM] = 0;
	TblKikiIntercept[6][3][SRY] = 0;

	TblKikiIntercept[6][3][GFU] = (KIKI01);
	TblKikiIntercept[6][3][GKY] = (KIKI01);
	TblKikiIntercept[6][3][GKE] = (KIKI01);
	TblKikiIntercept[6][3][GGI] = 0;
	TblKikiIntercept[6][3][GKI] = 0;
	TblKikiIntercept[6][3][GKA] = TblKikiIntercept[6][3][SKA];
	TblKikiIntercept[6][3][GHI] = TblKikiIntercept[6][3][SHI];
	TblKikiIntercept[6][3][GTO] = TblKikiIntercept[6][3][GKI];
	TblKikiIntercept[6][3][GNY] = TblKikiIntercept[6][3][GKI];
	TblKikiIntercept[6][3][GNK] = TblKikiIntercept[6][3][GKI];
	TblKikiIntercept[6][3][GNG] = TblKikiIntercept[6][3][GKI];
	TblKikiIntercept[6][3][GUM] = TblKikiIntercept[6][3][SUM];
	TblKikiIntercept[6][3][GRY] = TblKikiIntercept[6][3][SRY];

	// ����������Dir06���� �F��̕�����00, 03, 09���`�F�b�N����
	TblKikiIntercept[6][9][SFU] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][SKY] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][SKE] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][SGI] = (KIKI02);
	TblKikiIntercept[6][9][SKI] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][SKA] = 0;
	TblKikiIntercept[6][9][SHI] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][STO] = TblKikiIntercept[6][9][SKI];
	TblKikiIntercept[6][9][SNY] = TblKikiIntercept[6][9][SKI];
	TblKikiIntercept[6][9][SNK] = TblKikiIntercept[6][9][SKI];
	TblKikiIntercept[6][9][SNG] = TblKikiIntercept[6][9][SKI];
	TblKikiIntercept[6][9][SUM] = 0;
	TblKikiIntercept[6][9][SRY] = (KIKI02);

	TblKikiIntercept[6][9][GFU] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][GKY] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][GKE] = (KIKI00 | KIKI02);
	TblKikiIntercept[6][9][GGI] = (KIKI02);
	TblKikiIntercept[6][9][GKI] = (KIKI02);
	TblKikiIntercept[6][9][GKA] = TblKikiIntercept[6][9][SKA];
	TblKikiIntercept[6][9][GHI] = TblKikiIntercept[6][9][SHI];
	TblKikiIntercept[6][9][GTO] = TblKikiIntercept[6][9][GKI];
	TblKikiIntercept[6][9][GNY] = TblKikiIntercept[6][9][GKI];
	TblKikiIntercept[6][9][GNK] = TblKikiIntercept[6][9][GKI];
	TblKikiIntercept[6][9][GNG] = TblKikiIntercept[6][9][GKI];
	TblKikiIntercept[6][9][GUM] = TblKikiIntercept[6][9][SUM];
	TblKikiIntercept[6][9][GRY] = TblKikiIntercept[6][9][SRY];

	// ����������Dir07���� �F��̕�����00, 02, 08���`�F�b�N����
	TblKikiIntercept[7][0][SFU] = (KIKI03);
	TblKikiIntercept[7][0][SKY] = (KIKI03);
	TblKikiIntercept[7][0][SKE] = (KIKI03);
	TblKikiIntercept[7][0][SGI] = 0;
	TblKikiIntercept[7][0][SKI] = (KIKI03);
	TblKikiIntercept[7][0][SKA] = 0;
	TblKikiIntercept[7][0][SHI] = (KIKI03);
	TblKikiIntercept[7][0][STO] = TblKikiIntercept[7][0][SKI];
	TblKikiIntercept[7][0][SNY] = TblKikiIntercept[7][0][SKI];
	TblKikiIntercept[7][0][SNK] = TblKikiIntercept[7][0][SKI];
	TblKikiIntercept[7][0][SNG] = TblKikiIntercept[7][0][SKI];
	TblKikiIntercept[7][0][SUM] = 0;
	TblKikiIntercept[7][0][SRY] = 0;

	TblKikiIntercept[7][0][GFU] = (KIKI03);
	TblKikiIntercept[7][0][GKY] = (KIKI03);
	TblKikiIntercept[7][0][GKE] = (KIKI03);
	TblKikiIntercept[7][0][GGI] = 0;
	TblKikiIntercept[7][0][GKI] = 0;
	TblKikiIntercept[7][0][GKA] = TblKikiIntercept[7][0][SKA];
	TblKikiIntercept[7][0][GHI] = TblKikiIntercept[7][0][SHI];
	TblKikiIntercept[7][0][GTO] = TblKikiIntercept[7][0][GKI];
	TblKikiIntercept[7][0][GNY] = TblKikiIntercept[7][0][GKI];
	TblKikiIntercept[7][0][GNK] = TblKikiIntercept[7][0][GKI];
	TblKikiIntercept[7][0][GNG] = TblKikiIntercept[7][0][GKI];
	TblKikiIntercept[7][0][GUM] = TblKikiIntercept[7][0][SUM];
	TblKikiIntercept[7][0][GRY] = TblKikiIntercept[7][0][SRY];

	TblKikiIntercept[7][2][SFU] = (KIKI01);
	TblKikiIntercept[7][2][SKY] = (KIKI01);
	TblKikiIntercept[7][2][SKE] = (KIKI01);
	TblKikiIntercept[7][2][SGI] = 0;
	TblKikiIntercept[7][2][SKI] = (KIKI01);
	TblKikiIntercept[7][2][SKA] = 0;
	TblKikiIntercept[7][2][SHI] = (KIKI01);
	TblKikiIntercept[7][2][STO] = TblKikiIntercept[7][2][SKI];
	TblKikiIntercept[7][2][SNY] = TblKikiIntercept[7][2][SKI];
	TblKikiIntercept[7][2][SNK] = TblKikiIntercept[7][2][SKI];
	TblKikiIntercept[7][2][SNG] = TblKikiIntercept[7][2][SKI];
	TblKikiIntercept[7][2][SUM] = 0;
	TblKikiIntercept[7][2][SRY] = 0;

	TblKikiIntercept[7][2][GFU] = (KIKI01);
	TblKikiIntercept[7][2][GKY] = (KIKI01);
	TblKikiIntercept[7][2][GKE] = (KIKI01);
	TblKikiIntercept[7][2][GGI] = 0;
	TblKikiIntercept[7][2][GKI] = 0;
	TblKikiIntercept[7][2][GKA] = TblKikiIntercept[7][2][SKA];
	TblKikiIntercept[7][2][GHI] = TblKikiIntercept[7][2][SHI];
	TblKikiIntercept[7][2][GTO] = TblKikiIntercept[7][2][GKI];
	TblKikiIntercept[7][2][GNY] = TblKikiIntercept[7][2][GKI];
	TblKikiIntercept[7][2][GNK] = TblKikiIntercept[7][2][GKI];
	TblKikiIntercept[7][2][GNG] = TblKikiIntercept[7][2][GKI];
	TblKikiIntercept[7][2][GUM] = TblKikiIntercept[7][2][SUM];
	TblKikiIntercept[7][2][GRY] = TblKikiIntercept[7][2][SRY];

	// ����������Dir07���� �F��̕�����00, 02, 08���`�F�b�N����
	TblKikiIntercept[7][8][SFU] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][SKY] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][SKE] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][SGI] = (KIKI03);
	TblKikiIntercept[7][8][SKI] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][SKA] = 0;
	TblKikiIntercept[7][8][SHI] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][STO] = TblKikiIntercept[7][8][SKI];
	TblKikiIntercept[7][8][SNY] = TblKikiIntercept[7][8][SKI];
	TblKikiIntercept[7][8][SNK] = TblKikiIntercept[7][8][SKI];
	TblKikiIntercept[7][8][SNG] = TblKikiIntercept[7][8][SKI];
	TblKikiIntercept[7][8][SUM] = 0;
	TblKikiIntercept[7][8][SRY] = (KIKI03);

	TblKikiIntercept[7][8][GFU] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][GKY] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][GKE] = (KIKI00 | KIKI03);
	TblKikiIntercept[7][8][GGI] = (KIKI03);
	TblKikiIntercept[7][8][GKI] = (KIKI03);
	TblKikiIntercept[7][8][GKA] = TblKikiIntercept[7][8][SKA];
	TblKikiIntercept[7][8][GHI] = TblKikiIntercept[7][8][SHI];
	TblKikiIntercept[7][8][GTO] = TblKikiIntercept[7][8][GKI];
	TblKikiIntercept[7][8][GNY] = TblKikiIntercept[7][8][GKI];
	TblKikiIntercept[7][8][GNK] = TblKikiIntercept[7][8][GKI];
	TblKikiIntercept[7][8][GNG] = TblKikiIntercept[7][8][GKI];
	TblKikiIntercept[7][8][GUM] = TblKikiIntercept[7][8][SUM];
	TblKikiIntercept[7][8][GRY] = TblKikiIntercept[7][8][SRY];
}

template<Color us>
uint32_t Position::infoRound8King() const
{
	int i;
	uint32_t info = 0;
		// ........ ........ ........ XXXXXXXX (1) ���ł��̃}�X(�ʈȊO�̗������Ȃ��C�U���̗����������)
		// ........ ........ XXXXXXXX ........ (2) �ʂ��ړ��\�ȃ}�X(�U���̗������Ȃ��C ����̋���Ȃ�)
		// ........ XXXXXXXX ........ ........ (3) ��������ł͋ʂ��ړ��\�ȃ}�X(�󔒂��U���̋����)
		// XXXXXXXX ........ ........ ........ (4) ��𓮂������̃}�X(�ʈȊO�̗������Ȃ��U���̗�����2�ȏ゠��悤�ȁC �󔒂܂��͎���̋�̂���)
	int defKPos = (us == BLACK) ? sq_king<WHITE>() : sq_king<BLACK>();
	const effect_t *aKiki = (us == BLACK) ? effectB : effectW;	// �U�ߕ��̗���
	const effect_t *dKiki = (us == BLACK) ? effectW : effectB;	// �ʕ��̗���
	static const unsigned int disBitS[8] = {KIKI01 <<  0, KIKI00 <<  0, KIKI03 <<  0, KIKI02 <<  0, KIKI07 <<  0, KIKI06 <<  0, KIKI05 <<  0, KIKI04 <<  0};
	static const unsigned int disBitL[8] = {KIKI01 << 16, KIKI00 << 16, KIKI03 << 16, KIKI02 << 16, KIKI07 << 16, KIKI06 << 16, KIKI05 << 16, KIKI04 << 16};
	for (i = 0; i < 8; i++) {
		int z = defKPos + NanohaTbl::Direction[i];			// z �͋ʂ̔��ߖT
		// �ǂɑ΂��Ă͉����Ȃ�
		if (ban[z] != WALL) {
			effect_t kiki = EXIST_EFFECT(dKiki[z]);	// z �ɑ΂���ʕ��̋ʈȊO�̗���
			kiki ^= (1u << i);
			unsigned int akiki = EXIST_EFFECT(aKiki[z]);					// �U�ߕ��̗���
			if (ban[z] == EMP) {
				// ��
				info |= (0x00010000u << i);	// (3)
				if (EXIST_EFFECT(aKiki[z])) {
					if (kiki == 0) {
						info |= (0x00010001u << i);	// (1)+(3)
						if (akiki != 0) {
							// ������2�ȏ゠�邩1�ł��e�̗��������邩?
							if ((akiki & (akiki-1)) !=0) info |= (0x01000000u << i);	// (4)
							else {
								// ban[z]��EMP�Ȃ̂ŁA���������̋�ł̒��˂͂Ȃ�
								if ((akiki & disBitS[i]) && (aKiki[z+NanohaTbl::Direction[i]] & disBitL[i])) {
									info |= (0x01000000u << i);	// (4)
								}
							}
						}
					} else if (kiki > 0 && (kiki & (kiki-1)) == 0) {
						// �����͂ЂƂ� �� �s������Ă���Γ����Ȃ��̂ŗ����Ă��Ȃ��̂Ɠ���
						unsigned long id;
						_BitScanForward(&id, kiki);
						int from = (id < 16) ? z - NanohaTbl::Direction[id] : SkipOverEMP(z, -NanohaTbl::Direction[id]);
						if (pin[from] != 0 && pin[from] != NanohaTbl::Direction[id] && pin[from] != -NanohaTbl::Direction[id]) {
							info |= (0x00010001u << i);	// (1)+(3)
							if (akiki != 0) {
								// ������2�ȏ゠�邩1�ł��e�̗��������邩?
								if ((akiki & (akiki-1)) !=0) info |= (0x01000000u << i);	// (4)
								else {
									// ban[z]��EMP�Ȃ̂ŁA���������̋�ł̒��˂͂Ȃ�
									if ((akiki & disBitS[i]) && (aKiki[z+NanohaTbl::Direction[i]] & disBitL[i])) {
										info |= (0x01000000u << i);	// (4)
									}
								}
							}
						}
					}
				} else {
					info |= (0x00010100u << i);	// (2)+(3)
				}
			} else if (color_of(piece_on(Square(z))) == us) {
				// �U�ߕ��̋����
				if (EXIST_EFFECT(aKiki[z])) {
					info |= (0x00010000u << i);		// (3)
				} else {
					info |= (0x00010100u << i);		// (2)+(3)
				}
			} else {
				// �ʕ��̋����
				if ((akiki & (akiki-1)) !=0) {
					if (kiki == 0) {
						info |= (0x01000000u << i);	// (4)
					} else if (kiki > 0 && (kiki & (kiki-1)) == 0) {
						// �����͂ЂƂ� �� �s������Ă���Γ����Ȃ��̂ŗ����Ă��Ȃ��̂Ɠ���
						unsigned long id;
						_BitScanForward(&id, kiki);
						int from = (id < 16) ? z - NanohaTbl::Direction[id] : SkipOverEMP(z, -NanohaTbl::Direction[id]);
						if (pin[from] != 0 && pin[from] != NanohaTbl::Direction[id] && pin[from] != -NanohaTbl::Direction[id]) {
							info |= (0x01000000u << i);	// (4)
						}
					}
				} else if (akiki != 0) {
					bool bChk = false;
					// �e�̗��������邩?
					if ((akiki & disBitS[i]) && (aKiki[z+NanohaTbl::Direction[i]] & disBitL[i])) {
						bChk = true;
					} else if (akiki & disBitL[i]) {
						int check = SkipOverEMP(z, NanohaTbl::Direction[i]);
						if (aKiki[check] & disBitL[i]) {
							bChk = true;
						}
					}
					if (bChk) {
						if (kiki == 0) {
							info |= (0x01000000u << i);	// (4)
						} else if (kiki > 0 && (kiki & (kiki-1)) == 0) {
							// �����͂ЂƂ� �� �s������Ă���Γ����Ȃ��̂ŗ����Ă��Ȃ��̂Ɠ���
							unsigned long id;
							_BitScanForward(&id, kiki);
							int from = (id < 16) ? z - NanohaTbl::Direction[id] : SkipOverEMP(z, -NanohaTbl::Direction[id]);
							if (pin[from] != 0 && pin[from] != NanohaTbl::Direction[id] && pin[from] != -NanohaTbl::Direction[id]) {
								info |= (0x01000000u << i);	// (4)
							}
						}
					}
				}
			}
		}
	}
	return info;
}

// ��ł��ɂ�藘�����Ղ邩�H
// [����]
//    const uint32_t info	���ߖT�̏��
//    const int kpos	����ʂ̈ʒu
//    const int i	kpos + NanohaTbl::Direction[i] �ɋ��ł�
//    const int kind	�ł�̎��
//
// [�߂�l]
//    uint32_t		���،��ʁF0(�Ղ���̂͂Ȃ�)�ANon-Zero(�������Ղ��ċl�܂Ȃ�)

template<Color us>
uint32_t Position::chkInterceptDrop(const uint32_t info, const int kpos, const int i, const int kind) const
{
	int to = kpos + NanohaTbl::Direction[i];

	// ���̏ꏊ�ɒ����������Ȃ���΃��^�[��
	const effect_t *aKiki = (us == BLACK) ? effectB : effectW;	// �U�ߕ��̗���
	effect_t kiki = aKiki[to] & EFFECT_LONG_MASK;
	if (kiki == 0) return 0;
	kiki >>= EFFECT_LONG_SHIFT;

	// �����̒�`
	//  Dir05 Dir00 Dir04
	//  Dir03   ��  Dir02
	//  Dir07 Dir01 Dir06
	uint32_t ret = 0;
	uint32_t info3 = (info >> 16) & 0xFF;	// (3) ��������ł͋ʂ��ړ��\�ȃ}�X(�󔒂��U���̋����)
	effect_t around[8];
	around[0] = EXIST_EFFECT(aKiki[kpos + DIR00]);
	around[1] = EXIST_EFFECT(aKiki[kpos + DIR01]);
	around[2] = EXIST_EFFECT(aKiki[kpos + DIR02]);
	around[3] = EXIST_EFFECT(aKiki[kpos + DIR03]);
	around[4] = EXIST_EFFECT(aKiki[kpos + DIR04]);
	around[5] = EXIST_EFFECT(aKiki[kpos + DIR05]);
	around[6] = EXIST_EFFECT(aKiki[kpos + DIR06]);
	around[7] = EXIST_EFFECT(aKiki[kpos + DIR07]);
	while (kiki) {
		unsigned long id;	// ���������̕���
		_BitScanForward(&id, kiki);
		kiki &= kiki - 1;
		uint32_t check = TblKikiIntercept[id][i][kind] & info3;	//  TblKikiIntercept[8][12][32];	// [���������̕���][��̕���][��̎��] �� �Ղ������

		while (check) {
			unsigned long cdir;	// �����������
			_BitScanForward(&cdir, check);
			check &= check - 1;
			around[cdir] &= ~((1u << EFFECT_LONG_SHIFT) << id);
			if (around[cdir] == 0) return 1;
		}
	}
	return ret;
}

// ��ړ��ɂ�藘�����Ղ邩�H
// [����]
//    const Color us	���
//    const uint32_t info	���ߖT�̏��
//    const int kpos	����ʂ̈ʒu
//    const int i	kpos + NanohaTbl::Direction[i] �ɋ��ł�
//    const int from	�ړ��O�̋�̈ʒu
//    const int kind	�ړ���̋�̎��
//
// [�߂�l]
//    uint32_t		���،��ʁF0(�Ղ���̂͂Ȃ�)�ANon-Zero(�������Ղ��ċl�܂Ȃ�)

template<Color us>
uint32_t Position::chkInterceptMove(const uint32_t info, const int kpos, const int i, const int from, const int kind) const
{
	int to = kpos + NanohaTbl::Direction[i];

	// ���̏ꏊ�ɒ����������Ȃ���΃��^�[��
	const effect_t *aKiki = (us == BLACK) ? effectB : effectW;	// �U�ߕ��̗���
	effect_t kiki = aKiki[to] & EFFECT_LONG_MASK;
	if (kiki == 0) return 0;
	kiki >>= EFFECT_LONG_SHIFT;

	// �����̒�`
	//  Dir05 Dir00 Dir04
	//  Dir03   ��  Dir02
	//  Dir07 Dir01 Dir06
	uint32_t ret = 0;
	uint32_t info3 = (info >> 16) & 0xFF;	// (3) ��������ł͋ʂ��ړ��\�ȃ}�X(�󔒂��U���̋����)
	effect_t around[8];
	around[0] = EXIST_EFFECT(aKiki[kpos + DIR00]);
	around[1] = EXIST_EFFECT(aKiki[kpos + DIR01]);
	around[2] = EXIST_EFFECT(aKiki[kpos + DIR02]);
	around[3] = EXIST_EFFECT(aKiki[kpos + DIR03]);
	around[4] = EXIST_EFFECT(aKiki[kpos + DIR04]);
	around[5] = EXIST_EFFECT(aKiki[kpos + DIR05]);
	around[6] = EXIST_EFFECT(aKiki[kpos + DIR06]);
	around[7] = EXIST_EFFECT(aKiki[kpos + DIR07]);
	while (kiki) {
		unsigned long id;	// ���������̕���
		_BitScanForward(&id, kiki);
		kiki &= kiki - 1;
		uint32_t check = TblKikiIntercept[id][i][kind] & info3;	//  TblKikiIntercept[8][12][32];	// [���������̕���][��̕���][��̎��] �� �Ղ������

		while (check) {
			unsigned long cdir;	// �����������
			_BitScanForward(&cdir, check);
			check &= check - 1;
			int z = kpos + NanohaTbl::Direction[cdir];
			around[cdir] &= ~((1u << EFFECT_LONG_SHIFT) << id);
			unsigned int k = around[cdir];
			if (k == 0) return 1;
			if (k & (k-1)) continue;	// �Q�ȏ㗘���������OK
			unsigned long ki;
			_BitScanForward(&ki, k);
			int z2 = (ki < 16) ? z - NanohaTbl::Direction[ki] : SkipOverEMP(z, -NanohaTbl::Direction[ki]);
			if (z2 == from) {
				// ��������̗��������Ȃ�
				if (CanAttack(kind, from, to, z) == 0) return 1;
			}
		}
	}
	return ret;
}

// ��łɂ����l�̔���
//  (1) ���ł��̃}�X(�ʈȊO�̗������Ȃ��C�U���̗����������)
//  (2) �ʂ��ړ��\�ȃ}�X(�U���̗������Ȃ��C ����̋���Ȃ�)
// [����]
// [�߂�l]
//    uint32_t		���،��ʁF0(��ł��ŋl�܂Ȃ�)�ANon-Zero(��ł��ŋl��)

template<Color us>
uint32_t Position::CheckMate1plyDrop(const uint32_t info, Move &m) const
{
//	if ((info & 0xFFFF) == 0) return 0;

	uint32_t myHand = hand[us].h & ~HAND_FU_MASK;
	if (myHand == 0) return 0;

	uint32_t h = (us == BLACK) ? TblMate1plydrop[info & 0xFFFF] & 0xFF : (TblMate1plydrop[info & 0xFFFF] >> 16) & 0xFF;
	int enemyKing = (us == BLACK) ? sq_king<WHITE>()   : sq_king<BLACK>();

	uint32_t info1 = (info & 0x00FF);			// (1)
	uint32_t info2 = (info & 0xFF00) >> 8;	// (2)

	// �l�܂��̂ɕK�v�Ȏ�������邩�H
	const int SorG = (us == BLACK) ? SENTE : GOTE;
	uint32_t komaMASK = 0;
	int to;
#define CHK_PIECE(piece)	\
	if ((h & (1u << piece)) && (myHand & HAND_ ## piece ## _MASK)) {	\
		komaMASK |= (1u << piece);		\
		uint32_t dir = TblKikiCheck[SorG | piece] & info1;	\
		bool bErr = true;		\
		while (dir) {	\
			unsigned long id;	\
			_BitScanForward(&id, dir);	\
			if ((TblKikiKind[id][SorG | piece] & info2) == info2) {		\
				if (chkInterceptDrop<us>(info, enemyKing, id, SorG | piece) == 0) {	\
					to = enemyKing + NanohaTbl::Direction[id];		\
					m = cons_move(0, to, Piece(SorG | piece), EMP);	\
					return komaMASK;	\
				} else {				\
					bErr = false;		\
				}			\
			}				\
			dir &= dir - 1;	\
		}					\
		if (bErr) {	this->print_csa(); MYABORT(); }	\
	}

	if (info1) {
		do {
			CHK_PIECE(KI)
			CHK_PIECE(GI)
			CHK_PIECE(HI)
			else CHK_PIECE(KY)	// ���Ԃ͔�Ԃ��`�F�b�N�����Ƃ��͊m�F���Ȃ�
			CHK_PIECE(KA)
		} while (0);
	}
#undef CHK_PIECE

	// �j�n�̃`�F�b�N
///	const effect_t *aKiki = (us == BLACK) ? effectB : effectW;	// �U�ߕ��̗���
	const effect_t *dKiki = (us == BLACK) ? effectW : effectB;	// �ʕ��̗���
	if (info2 == 0 && (myHand & HAND_KE_MASK)) {
		int to1 = (us == BLACK) ? enemyKing - DIR_KEUR : enemyKing - DIR_KEDR;
		int to2 = (us == BLACK) ? enemyKing - DIR_KEUL : enemyKing - DIR_KEDL;
		if (ban[to1] == EMP) {
			effect_t k = dKiki[to1];
			int kdir = (us == BLACK) ? 11 : 9;	// 9:DIR_KEUL, 11:DIR_KEDL
			if (k == 0) {
				if (chkInterceptDrop<us>(info, enemyKing, kdir, SorG | KE) == 0) {
					komaMASK |= (1 << KE);
					m = cons_move(0, to1, Piece(SorG | KE), EMP);
					return komaMASK;
				}
			} else if (chkInterceptDrop<us>(info, enemyKing, kdir, SorG | KE) == 0) {
				// �j�n�ɗ����Ă����s������Ă��邩�ǂ����H
				k &= EFFECT_SHORT_MASK;
				while (k) {
					unsigned long id;
					_BitScanForward(&id, k);
					int df = to1 - NanohaTbl::Direction[id];
					if (pin[df] == 0) break;
					k &= k - 1;
				}
				if (k == 0) {
					k =  dKiki[to1] & EFFECT_LONG_MASK;
					while (k) {
						unsigned long id;
						_BitScanForward(&id, k);
						int df = SkipOverEMP(to1, -NanohaTbl::Direction[id]);
						if (pin[df] == 0) break;
						k &= k - 1;
					}
					if (k == 0) {
						komaMASK |= (1 << KE);
						m = cons_move(0, to1, Piece(SorG | KE), EMP);
						return komaMASK;
					}
				}
			}
		}
		if (ban[to2] == EMP) {
			effect_t k = dKiki[to2];
			int kdir = (us == BLACK) ? 10 : 8;	// 8:DIR_KEUR, 10:DIR_KEDR
			if (k == 0) {
				if (chkInterceptDrop<us>(info, enemyKing, kdir, SorG | KE) == 0) {
					komaMASK |= (1 << KE);
					m = cons_move(0, to2, Piece(SorG | KE), EMP);	
					return komaMASK;
				}
			} else if (chkInterceptDrop<us>(info, enemyKing, kdir, SorG | KE) == 0) {
				// �j�n�ɗ����Ă����s������Ă��邩�ǂ����H
				k &= EFFECT_SHORT_MASK;
				while (k) {
					unsigned long id;
					_BitScanForward(&id, k);
					int df = to2 - NanohaTbl::Direction[id];
					if (pin[df] == 0) break;
					k &= k - 1;
				}
				if (k == 0) {
					k =  dKiki[to2] & EFFECT_LONG_MASK;
					while (k) {
						unsigned long id;
						_BitScanForward(&id, k);
						int df = SkipOverEMP(to2, -NanohaTbl::Direction[id]);
						if (pin[df] == 0) break;
						k &= k - 1;
					}
					if (k == 0) {
						komaMASK |= (1 << KE);
						m = cons_move(0, to2, Piece(SorG | KE), EMP);
						return komaMASK;
					}
				}
			}
		}
	}

	// �l�ނ��l�܂Ȃ����݂̂�Ԃ�
	return 0;
}

// (���X�A�ʒu from1 �ɂ����)�ʒu from2 �̋� kind �͈ʒu to �ɗ�����?
// [�߂�l]
//    int		���،��ʁF0(�����Ȃ�)�ANon-Zero(����)
int Position::CanAttack(const int kind, const int from1, const int from2, const int to) const
{
	switch (kind) {
	case EMP:
	case SFU:
	case SKY:
	case SKE:
	case SOU:
	case GFU:
	case GKY:
	case GKE:
	case GOU:
		assert(0);
		break;
	case SGI:
	case SKI:
	case SKA:
	case SHI:
	case STO:
	case SNY:
	case SNK:
	case SNG:
	case SUM:
	case SRY:
	case GGI:
	case GKI:
	case GKA:
	case GHI:
	case GTO:
	case GNY:
	case GNK:
	case GNG:
	case GUM:
	case GRY:
	default:
		break;
	}

	int diff2 = to - from2;
	int mid;
	static const unsigned int tbl[] = {
		// 0:UP
		  (1u << SGI) | (1u << SKI) | (0x0Fu << STO)
		| (0  << GGI) | (1u << GKI) | (0x0Fu << GTO)
		| (0x00000  << SKA) | (0x10001u << SHI) | (0x10001u << SUM) | (0x10001u << SRY),
		// 1:DOWN
		  (0  << SGI) | (1u << SKI) | (0x0Fu << STO)
		| (1u << GGI) | (1u << GKI) | (0x0Fu << GTO)
		| (0x00000  << SKA) | (0x10001u << SHI) | (0x10001u << SUM) | (0x10001u << SRY),
		// 2:RIGHT or LEFT
		  (0  << SGI) | (1u << SKI) | (0x0Fu << STO)
		| (0  << GGI) | (1u << GKI) | (0x0Fu << GTO)
		| (0x00000  << SKA) | (0x10001u << SHI) | (0x10001u << SUM) | (0x10001u << SRY),
		// 3:UR or UL
		  (1u << SGI) | (1u << SKI) | (0x0Fu << STO)
		| (1u << GGI) | (0  << GKI) | (0x00  << GTO)
		| (0x10001u << SKA) | (0x00000  << SHI) | (0x10001u << SUM) | (0x10001u << SRY),
		// 4:DR or DL
		  (1u << SGI) | (0  << SKI) | (0x00  << STO)
		| (1u << GGI) | (1u << GKI) | (0x0Fu << GTO)
		| (0x10001u << SKA) | (0x00000  << SHI) | (0x10001u << SUM) | (0x10001u << SRY),
		// 5: 2 * (UP or DOWN or LEFT or RIGHT)
		  (0x00000  << SKA) | (0x10001u << SHI) | (0x00000  << SUM) | (0x10001u << SRY),
		// 6: 2 * (UR or UL or DR or DL)
		  (0x10001u << SKA) | (0x00000  << SHI) | (0x10001u << SUM) | (0x00000  << SRY),
	};
	switch (diff2) {
	case DIR_UP:
		if (tbl[0] & (1u << kind)) return 1;
		break;
	case DIR_DOWN:
		if (tbl[1] & (1u << kind)) return 1;
		break;
	case DIR_LEFT:
	case DIR_RIGHT:
		if (tbl[2] & (1u << kind)) return 1;
		break;
	case DIR_UR:
	case DIR_UL:
		if (tbl[3] & (1u << kind)) return 1;
		break;
	case DIR_DR:
	case DIR_DL:
		if (tbl[4] & (1u << kind)) return 1;
		break;
	case 2*DIR_UP:
	case 2*DIR_DOWN:
	case 2*DIR_LEFT:
	case 2*DIR_RIGHT:
		if (tbl[5] & (1u << kind)) {
			mid = to - diff2 / 2;
			if (ban[mid] == EMP) return 1;
			if ((ban[mid] & ~GOTE) == OU) return 1;
		}
		break;
	case 2*DIR_UR:
	case 2*DIR_UL:
	case 2*DIR_DR:
	case 2*DIR_DL:
		if (tbl[6] & (1u << kind)) {
			mid = to - diff2 / 2;
			if (ban[mid] == EMP) return 1;
			if ((ban[mid] & ~GOTE) == OU) return 1;
		}
		break;
	default:
		break;
	}

	return 0;
}

// �ړ��ɂ����l�̔���
// [�߂�l]
//    uint32_t		���،��ʁF0(��ړ��ŋl�܂Ȃ�)�ANon-Zero(��ړ��ŋl��)
// [����]
//    const uint32_t info	
//    effect_t around[8]	
//    Move &m		[out] �l�܂���
//    const int check	����������Ă����̈ʒu
//
template<Color us>
uint32_t Position::CheckMate1plyMove(const uint32_t info, Move &m, const int check) const
{
	// (4) ��𓮂������̃}�X(�ʈȊO�̗������Ȃ��U���̗�����2 �ȏ゠��悤�ȁC �󔒂܂��͎���̋�̂���)
	// (4) �̊e�}�X�ɂ��āC���̃}�X�Ɉړ��\�ȋ�ꂼ��ɂ��āC�ړ��ɂ�� (2)��S�čǂ���
	//     ���ǂ������r�b�g���Z�ɂ�苁�߂�D �S�čǂ��錩���݂�����ꍇ�ɂ́C �ړ��ɂ�藘�����Ղ��ċl�܂Ȃ�
	//     ��O���ǂ����ڍׂȔ�����s���D
	int mNum;
	Move mBuf[32];
	const unsigned int enemyKing = (us == BLACK) ? sq_king<WHITE>()   : sq_king<BLACK>();	// �ʕ��̋ʂ̈ʒu
	const int SorG = (us == BLACK) ? SENTE : GOTE;

	// �ʂ��ړ��\�ȃ}�X���Ȃ��ꍇ�̂݁A�j�n�ł̋l�݂����邩�`�F�b�N���s��
	if ((info & 0x0000FF00) == 0) {
		const effect_t *aKiki = (us == BLACK) ? effectB : effectW;	// �U�ߕ��̗���
		const effect_t *dKiki = (us == BLACK) ? effectW : effectB;	// �ʕ��̗���
		int to, from;

		const int d1 = (us == BLACK) ? -DIR_KEUR : -DIR_KEDR;
		const int d2 = (us == BLACK) ? -DIR_KEUL : -DIR_KEDL;
		const int kdir2 = (us == BLACK) ? 10 : 8;
		const int kdir1 = (us == BLACK) ? 11 : 9;

		to = (us == BLACK) ? enemyKing - DIR_KEUR : enemyKing - DIR_KEDR;
		if (check == 0 || to == check) {
			if ((aKiki[to] & 0x00000F00) && ban[to] != WALL && (ban[to] == EMP || (ban[to] & GOTE) != SorG)) {
				// to �ɗ����Ă���Ȍj�n�����āAto �̈ʒu�Ɉړ��\�ȏꍇ
				effect_t k = dKiki[to];
				bool bCheck = false;
				// to �̈ʒu�̋ʕ��̗����𒲂ׁA�������Ȃ��ꍇ��(����=1 ���� �����Ă����s������Ă���ꍇ)�ɏڂ������ׂ�
				if (k == 0) bCheck = true;
				else if ((k & (k - 1)) == 0) {
					// ��������̎�
					unsigned long id;
					_BitScanForward(&id, k);
					int df = (id < 16) ? to - NanohaTbl::Direction[id] : SkipOverEMP(to, -NanohaTbl::Direction[id]);
					if (pin[df] != 0) bCheck = true;
				}
				if (bCheck) {
					// ���ڂ������ׂ�Bto �Ɍj�n�Ɉړ��\�ŁA�ړ����邱�ƂŁA�������Ղ��ċl�܂Ȃ��Ȃ邩? �� �Y�����Ȃ��ꍇ�͋l��
					// �����ł� kdir1 or kdir2 �̎w��͏�� to �̌v�Z�ɍ��킹��
					from = to + d1;
					if (ban[from] == (SorG | KE) && pin[from] == 0 &&  chkInterceptMove<us>(info, enemyKing, kdir1, from, SorG | KE) == 0) {
						m = cons_move(from, to, ban[from], ban[to], 0);
						return 1;
					}
					from = to + d2;
					if (ban[from] == (SorG | KE) && pin[from] == 0 && chkInterceptMove<us>(info, enemyKing, kdir1, from, SorG | KE) == 0) {
						m = cons_move(from, to, ban[from], ban[to], 0);
						return 1;
					}
				}
			}
		}
		to = (us == BLACK) ? enemyKing - DIR_KEUL : enemyKing - DIR_KEDL;
		if (check == 0 || to == check) {
			if ((aKiki[to] & 0x00000F00) && ban[to] != WALL && (ban[to] == EMP || (ban[to] & GOTE) != SorG)) {
				// to �ɗ����Ă���Ȍj�n�����āAto �̈ʒu�Ɉړ��\�ȏꍇ
				effect_t k = dKiki[to];
				bool bCheck = false;
				// to �̈ʒu�̋ʕ��̗����𒲂ׁA�������Ȃ��ꍇ��(����=1 ���� �����Ă����s������Ă���ꍇ)�ɏڂ������ׂ�
				if (k == 0) bCheck = true;
				else if ((k & (k - 1)) == 0) {
					// ��������̎�
					unsigned long id;
					_BitScanForward(&id, k);
					int df = (id < 16) ? to - NanohaTbl::Direction[id] : SkipOverEMP(to, -NanohaTbl::Direction[id]);
					if (pin[df] != 0) bCheck = true;
				}
				if (bCheck) {
					// ���ڂ������ׂ�Bto �Ɍj�n�Ɉړ��\�ŁA�ړ����邱�ƂŁA�������Ղ��ċl�܂Ȃ��Ȃ邩? �� �Y�����Ȃ��ꍇ�͋l��
					// �����ł� kdir1 or kdir2 �̎w��͏�� to �̌v�Z�ɍ��킹��
					from = to + d1;
					if (ban[from] == (SorG | KE) && pin[from] == 0 && chkInterceptMove<us>(info, enemyKing, kdir2, from, SorG | KE) == 0) {
						m = cons_move(from, to, ban[from], ban[to], 0);
						return 1;
					}
					from = to + d2;
					if (ban[from] == (SorG | KE) && pin[from] == 0 && chkInterceptMove<us>(info, enemyKing, kdir2, from, SorG | KE) == 0) {
						m = cons_move(from, to, ban[from], ban[to], 0);
						return 1;
					}
				}
			}
		}
	}

	// (4) ��𓮂������̃}�X���Ȃ���ΏI��
	if ((info & 0xFF000000) == 0) return 0;

	uint32_t info2 = (info >>  8) & 0xFF;	// (2) �ʂ��ړ��\�ȃ}�X(�U���̗������Ȃ��C ����̋���Ȃ�)
	uint32_t info4 = (info >> 24) & 0xFF;	// (4) ��𓮂������̃}�X(�ʈȊO�̗������Ȃ��U���̗�����2�ȏ゠��悤�ȁC �󔒂܂��͎���̋�̂���)
	uint32_t info24 = (info2 << 8) | info4;
	if (TblMate1plydrop[info24] == 0) return 0;

	const effect_t *aKiki = (us == BLACK) ? effectB : effectW;	// �U�ߕ��̗���
	const effect_t *dKiki = (us == BLACK) ? effectW : effectB;	// �ʕ��̗���
	while (info4) {
		unsigned long id;	// ��𓮂������̃}�X�̕���
		_BitScanForward(&id, info4);
		info4 &= info4 - 1;
		info24 = (info2 << 8) | (1u << id);
		if (TblMate1plydrop[info24] == 0) continue;
		int to = enemyKing + NanohaTbl::Direction[id];
		if (check != 0 && to != check) {
			// ���肪�������Ă���Ƃ���to������̋�̈ʒu�łȂ��Ȃ��蒼��
			continue;
		}
		effect_t kiki = EXIST_EFFECT(aKiki[to]);	// �ړ���̍U�߂̗���
		while (kiki) {
			unsigned long d;	// �����̕���
			_BitScanForward(&d, kiki);
			kiki &= kiki - 1;
			int from = (d < 15) ? to - NanohaTbl::Direction[d] : SkipOverEMP(to, -NanohaTbl::Direction[d]);
			d &= 0x0F;
			// �s���̊m�F
			if (pin[from] != 0 && pin[from] != NanohaTbl::Direction[d] && pin[from] != -NanohaTbl::Direction[d]) continue;
			if ((dKiki[from] & ((1u << EFFECT_LONG_SHIFT) << d)) != 0) {
				// ���������Ƃ��Ă����ɋʕ��̒�������������ꍇ�A���̕����ɓ������Ă�����Ă��܂����ߋl�܂Ȃ�
				continue;
			}

			mNum = 0;
			int kind = ban[from];
			bool bCanPromote = (us == BLACK) ? ((to & 0x0F) <= 3 || (from & 0x0F) <= 3) : ((to & 0x0F) >= 7 || (from & 0x0F) >= 7);
			bool bChkDetail = true;		// �ڍ׃`�F�b�N���邩? (���ƍ��͈ړ��ɂ�藘�����ς��Ȃ��̂ŏڍ׃`�F�b�N���Ȃ�)
			switch (kind) {
			case EMP:	// �œK���̂��߂̃_�~�[
				break;
			case SFU:
			case GFU:
				bChkDetail = false;	// ���͏ڍ׃`�F�b�N���Ȃ�
			case SKA:
			case GKA:
			case SHI:
			case GHI:
				// ���A�p�A��͐���鎞�͕s�����`�F�b�N���Ȃ��B
				if (bCanPromote && (TblKikiCheck[kind | PROMOTED] & (1u << id)) != 0) {
					if (TblMate1plydrop[info24] & ((1u << PROMOTED ) << kind))
						if (chkInterceptMove<us>(info, enemyKing, id, from, kind | PROMOTED) == 0) { mBuf[mNum++] = cons_move(from, to, Piece(kind), ban[to], 1); }
				} else if ((TblKikiCheck[kind] & (1u << id)) != 0) {
					if (TblMate1plydrop[info24] & ((1u             ) << kind))
						if (chkInterceptMove<us>(info, enemyKing, id, from, kind           ) == 0) { mBuf[mNum++] = cons_move(from, to, Piece(kind), ban[to], 0); }
				}
				break;
			case SKY:
			case GKY:
				bChkDetail = false;	// ���Ԃ͏ڍ׃`�F�b�N���Ȃ�
			case SGI:
			case GGI:
				// ���A��͐��A�s�����`�F�b�N����
				if (bCanPromote && (TblKikiCheck[kind | PROMOTED] & (1u << id)) != 0) {
					if (TblMate1plydrop[info24] & ((1u << PROMOTED ) << kind))
						if (chkInterceptMove<us>(info, enemyKing, id, from, kind | PROMOTED) == 0) { mBuf[mNum++] = cons_move(from, to, Piece(kind), ban[to], 1); }
				}
				if ((TblKikiCheck[kind] & (1u << id)) != 0) {
					if (TblMate1plydrop[info24] & ((1u             ) << kind))
						if (chkInterceptMove<us>(info, enemyKing, id, from, kind           ) == 0) { mBuf[mNum++] = cons_move(from, to, Piece(kind), ban[to], 0); }
				}
				break;
			case SKE:
			case GKE:
				// �j�n�͐���̂݊m�F����(to�����ߖT�Ȃ̂�)
				if (bCanPromote && (TblKikiCheck[kind | PROMOTED] & (1u << id)) != 0) {
					if (TblMate1plydrop[info24] & ((1u << PROMOTED ) << kind))
						if (chkInterceptMove<us>(info, enemyKing, id, from, kind | PROMOTED) == 0) { mBuf[mNum++] = cons_move(from, to, Piece(kind), ban[to], 1); }
				}
				break;
			case SRY:
			case GRY:
				// ���̂Ƃ������s�����Ă����̃s�����O��Ă��܂��ꍇ������̂ŁA�v�����I�I
				{
					int diff = enemyKing - from;
					if (diff == 2*DIR_LEFT || diff == 2*DIR_RIGHT || diff == 2*DIR_UP || diff == 2*DIR_DOWN) {
						effect_t k = EXIST_EFFECT(dKiki[to]);
						k &= ~(1u << id);	// �ʂ̗������͂���
						if (k > 0) {
							// to�ɋ�̗��������邽�߁A���Ńs�����Ă�����̃s�����O���I
							break;
						}
					}
				}
			case SKI:
			case GKI:
			case STO:
			case SNY:
			case SNK:
			case SNG:
			case SUM:
			case GTO:
			case GNY:
			case GNK:
			case GNG:
			case GUM:
				// ������ѐ���͈ړ��̂�
				if ((TblKikiCheck[kind] & (1u << id)) != 0) {
					if (TblMate1plydrop[info24] & ((1u             ) << kind))
						if (chkInterceptMove<us>(info, enemyKing, id, from, kind           ) == 0) { mBuf[mNum++] = cons_move(from, to, Piece(kind), ban[to], 0); }
				}
				break;
			case SOU:
			case GOU:	// �ʂŉ���͂ł��Ȃ�
				break;

			default:
				__assume(0);
				break;
			}

			// ���������Ƀ`�F�b�N����
			if (mNum > 0) {
				// ���ƍ��͈ړ��ɂ�藘�����ς��Ȃ��̂ŏڍ׃`�F�b�N���Ȃ��Ŏ肪����͂����Ԃ�
				if (bChkDetail == false) {
					m = mBuf[0];
					return 1;
				}

				// � from ���� to �ɍs�����ƂŁA�ʂ��ړ��\�ɂȂ�Ȃ���? (���܂ŗ����Ă��Ƃ���̗������Ȃ��Ȃ邩??)
				uint32_t info3 = (info >> 16) & 0xFF;	//  (3) ��������ł͋ʂ��ړ��\�ȃ}�X(�󔒂��U���̋����)
				// info3 &= ~info2;			// �~��������Ɣ�ы�ŊԈႤ(�ʂ��ړ��\�ȃ}�X�̓`�F�b�N���Ȃ�)
				info3 &= ~(1u << id);			// to�̈ʒu���`�F�b�N���Ȃ�
				if (info3 == 0) {
					// �ړ��\�ȂƂ���͂Ȃ� �� �l��
					m = mBuf[0];
					return 1;
				}
				unsigned long dd;
				bool bEscape1 = false;
				bool bEscape2 = false;
				while (info3) {
					_BitScanForward(&dd, info3);
					info3 &= info3-1;
					int z = enemyKing + NanohaTbl::Direction[dd];	// ��������ʒu
					effect_t k = EXIST_EFFECT(aKiki[z]);
					int kd = is_promotion(mBuf[0]) ? kind | PROMOTED : kind;
					if (k == 0) {
						// �������Ȃ��ꍇ�A�ړ�����������?
						if (CanAttack(kd, from, to, z) == 0) {
							bEscape1 = true;
						}
						if (mNum < 2 || (CanAttack(kind, from, to, z) == 0)) {
							bEscape2 = true;
						}
						if (bEscape1 && bEscape2) break;
						continue;
					}
					if ((k & (k-1)) > 0) continue;		// ������2�ȏ゠��΂���ȏ㒲�����Ȃ�
					unsigned long ki;
					_BitScanForward(&ki, k);
					int zf = (z < 16) ? z - NanohaTbl::Direction[ki] : SkipOverEMP(z, -NanohaTbl::Direction[ki]);		// z �ɗ����Ă���ʒu
					if (zf != from) continue;		// ��������̗����łȂ��Ȃ炻��ȏ㒲�����Ȃ�

					// � from ���� to �ɍs�����ƂŁAz �̗������Ȃ��Ȃ邩??
						// z �̗������Ȃ��Ȃ����瓦������ �� �l�܂Ȃ�
					if (CanAttack(kd, from, to, z) == 0) {
						bEscape1 = true;
					}
					if (mNum < 2 || (CanAttack(kind, from, to, z) == 0)) {
						bEscape2 = true;
					}
					if (bEscape1 && bEscape2) break;
				}
				if (bEscape1 == false) { m = mBuf[0]; return 1;}
				if (bEscape2 == false) { m = mBuf[1]; return 1;}
			}
		}
	}
	return 0;
}

// 1��ŋl�ނ��m�F����
//  �߂�l�FVALUE_MATE�F�l�݁A����ȊO:�l�܂Ȃ�
template<Color us>
int Position::Mate1ply(Move &m, uint32_t &info)
{
	tnodes++;
	uint32_t ret;

// for DEBUG�Gm�Ƃ�����ŋl�ނƂ������ʂ��o���Ƃ��ɁAm���w������̋ǖʂŖ{����
//  �l��ł���(���@��̐����O)���m�F����
#if 0 //!defined(NDEBUG)
#define CHK_MATE1PLY(m)			\
	do { ExtMove ss[MAX_EVASION], *mlist;		\
		StateInfo newSt;  \
		do_move(m, newSt);			\
		mlist = generate<MV_LEGAL>(*this, ss);		\
		undo_move(m);		\
		if (mlist != ss) {		\
			print_csa(m);		\
			MYABORT();		\
		}				\
	} while (0)
#else//defined(DEBUG)
#define CHK_MATE1PLY(m)
#endif//defined(DEBUG)

	// �ʂ�8�ߖT�̏��Z�o
	info = infoRound8King<us>();

	effect_t kiki = (us == BLACK)
		 ? ( (sq_king<BLACK>() == 0) ? 0 : EXIST_EFFECT(effectW[sq_king<BLACK>()]) )
		 : ( (sq_king<WHITE>() == 0) ? 0 : EXIST_EFFECT(effectB[sq_king<WHITE>()]) );
	if (kiki == 0) {
		// ���肪�������Ă��Ȃ�
		// �ړ��ɂ����l�̔���
		ret = CheckMate1plyMove<us>(info, m);
		if (ret != 0) {
			assert(is_ok(m));
			COUNT_PERFORM(count_Mate1plyMove);
			CHK_MATE1PLY(m);
			return VALUE_MATE;
		}

		// ��łɂ����l�̔���
		ret = CheckMate1plyDrop<us>(info, m);
		if (ret != 0) {
			assert(is_ok(m));
			COUNT_PERFORM(count_Mate1plyDrop);
			CHK_MATE1PLY(m);
			return VALUE_MATE;
		}
	} else if ((kiki & (kiki-1)) == 0) {
		// ������ł͂Ȃ�
		int check;		// ����������Ă����̈ʒu
		unsigned long id;	// �����̕���
		_BitScanForward(&id, kiki);
		if (kiki & EFFECT_SHORT_MASK) {
			check = (us == BLACK) ? sq_king<BLACK>() - NanohaTbl::Direction[id] : sq_king<WHITE>() - NanohaTbl::Direction[id];
		} else {
			check = (us == BLACK) ? SkipOverEMP(sq_king<BLACK>(), -NanohaTbl::Direction[id]) : SkipOverEMP(sq_king<WHITE>(), -NanohaTbl::Direction[id]);
		}

		// �ړ��ɂ����l�̔���
		ret = CheckMate1plyMove<us>(info, m, check);
		if (ret != 0) {
			assert(is_ok(m));
			COUNT_PERFORM(count_Mate1plyMove);
			CHK_MATE1PLY(m);
			return VALUE_MATE;
		}
	}
	return VALUE_ZERO;
}
template int Position::Mate1ply<BLACK>(Move &m, uint32_t &info);
template int Position::Mate1ply<WHITE>(Move &m, uint32_t &info);
