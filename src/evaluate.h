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



#if !defined(EVALUATE_H_INCLUDED)
#define EVALUATE_H_INCLUDED

#include "types.h"

class Position;

#if 0
Value evaluate(const Position& pos, Value& margin);
#else
namespace Eval {
	const Value Tempo = Value(20); // Must be visible to search
}
#endif
#if defined(NANOHA) && (USE_AVX2_EVAL) && (TWIG)
#include <emmintrin.h>
#include <smmintrin.h>
#endif
#ifdef USAPYON2
extern void ehash_clear();
extern int ehash_probe(uint64_t current_key, int * __restrict pscore);
extern void ehash_store(uint64_t key, int score);
#endif

const Value Tempo = Value(20); // Must be visible to search

std::string trace(const Position& pos);

//template<bool DoTrace = false>
Value evaluate(const Position& pos);

#endif // #ifndef EVALUATE_H_INCLUDED
