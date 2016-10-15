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


#if !defined(BOOK_H_INCLUDED)
#define BOOK_H_INCLUDED

#include <map>
#include <cstring>	// for memcmp()

#include "move.h"
#include "movegen.h"
#include "misc.h"

class Position;

// ��Ճf�[�^�̃L�[
struct BookKey {
	unsigned char data[32];

	bool operator < (const BookKey &b) const {
		return memcmp(data, b.data, sizeof(BookKey)) < 0;
	}
};

// ��Ճf�[�^�̒��g
struct BookEntry {
	short eval;				// �]���l
	unsigned short hindo;	// �p�x(�o����)
	unsigned short swin;	// ��菟����
	unsigned short gwin;	// ��菟����
	unsigned short dummy[3];	// �\��
	unsigned char move[9][2];
};

// ��Ղ̃N���X
class Book {
private:
	typedef std::map<BookKey, BookEntry> Joseki_type;
	typedef Joseki_type::value_type Joseki_value;
	Joseki_type joseki;
	PRNG prng;
	Book(const Book&);				// warning�΍�.
	Book& operator = (const Book&);	// warning�΍�.
public:
	Book();
	~Book();
	// ������
	void open(const std::string& fileName);
	void close();
	// ��Ճf�[�^����A���݂̋ǖ�k�̍��@�肪���̋ǖʂłǂꂭ�炢�̕p�x�Ŏw���ꂽ����Ԃ��B
	void fromJoseki(Position &pos, int &mNum, ExtMove moves[], BookEntry data[]);
	// ���݂̋ǖʂ��ǂ̂��炢�̕p�x�Ŏw���ꂽ����Ճf�[�^�𒲂ׂ�
	int getHindo(const Position &pos);

	// 
	size_t size() const {return joseki.size();}

	Move get_move(Position& pos, bool findBestMove);
};

#ifdef USAPYON2
extern Book *book[2];
#else
extern Book *book;
#endif

#endif // !defined(BOOK_H_INCLUDED)
