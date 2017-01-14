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

#ifndef POSITION_H_INCLUDED
#define POSITION_H_INCLUDED

#include <cassert>
#include <deque>
#include <memory> // For std::unique_ptr
#include <string>

#ifdef _MSC_VER
#else
#include <immintrin.h> // AVX2
#endif

#ifndef NANOHA 
#include "bitboard.h"
#endif

#ifdef EVAL_DIFF
#include "move.h"
#endif

#include "types.h"
#ifdef NANOHA
#include "movegen.h"
#endif

#include <array>

#ifdef EVAL_DIFF
//#include "search.h"
namespace Search {
	struct Stack;
}
#endif // EVAL_DIFF


class Position;
class Thread;

#ifndef NANOHA
namespace PSQT {

  extern Score psq[COLOR_NB][PIECE_TYPE_NB][SQUARE_NB];

  void init();
}
#endif

#ifndef NANOHA
/// CheckInfo struct is initialized at constructor time and keeps info used to
/// detect if a move gives check.

struct CheckInfo {

  explicit CheckInfo(const Position&);

  Bitboard dcCandidates;
  Bitboard pinned;
  Bitboard checkSquares[PIECE_TYPE_NB];
  Square   ksq;
};
#endif

/// StateInfo struct stores information needed to restore a Position object to
/// its previous state when we retract a move. Whenever a move is made on the
/// board (by calling Position::do_move), a StateInfo object must be passed.

struct StateInfo {
#if defined(NANOHA)
	int gamePly;
	int pliesFromNull;

	Piece captured;
	uint32_t hand;
	uint32_t effect;
	Key key;
#ifdef EVAL_DIFF
	PieceNumber_t kncap;
	Piece capHand;
	Piece dropHand;
	PieceNumber_t kndrop;
	int oldcap[2];
	int oldlist[2];
	int newcap[2];
	int newlist[2];
	int changeType;
#endif
  Piece      capturedPiece;
#else
  // Copied when making a move
  Key    pawnKey;
  Key    materialKey;
  Value  nonPawnMaterial[COLOR_NB];
  int    castlingRights;
  int    rule50;
  int    pliesFromNull;
  Score  psq;
  Square epSquare;

  // Not copied when making a move
  Key        key;
  Bitboard   checkersBB;
  PieceType  capturedType;
#endif
  StateInfo* previous;
};

// In a std::deque references to elements are unaffected upon resizing
typedef std::unique_ptr<std::deque<StateInfo>> StateListPtr;

#if defined(NANOHA)
// �������֐�
extern void init_application_once();	// ���s�t�@�C���N�����ɍs��������.
#endif

#ifdef USAPYON2
#ifdef TWIG
template <typename Tl, typename Tr>
inline std::array<Tl, 2> operator += (std::array<Tl, 2>& lhs, const std::array<Tr, 2>& rhs) {
	lhs[0] += rhs[0];
	lhs[1] += rhs[1];
	return lhs;
}
template <typename Tl, typename Tr>
inline std::array<Tl, 2> operator -= (std::array<Tl, 2>& lhs, const std::array<Tr, 2>& rhs) {
	lhs[0] -= rhs[0];
	lhs[1] -= rhs[1];
	return lhs;
}
#endif

#ifdef TWIG
struct EvalSum {
#if defined USE_AVX2_EVAL
	EvalSum(const EvalSum& es) {
		_mm256_store_si256(&mm, es.mm);
	}
	EvalSum& operator = (const EvalSum& rhs) {
		_mm256_store_si256(&mm, rhs.mm);
		return *this;
	}
#elif defined USE_SSE_EVAL
	EvalSum(const EvalSum& es) {
		_mm_store_si128(&m[0], es.m[0]);
		_mm_store_si128(&m[1], es.m[1]);
	}
	EvalSum& operator = (const EvalSum& rhs) {
		_mm_store_si128(&m[0], rhs.m[0]);
		_mm_store_si128(&m[1], rhs.m[1]);
		return *this;
	}
#endif
	EvalSum() {}
	int sum(const Color c) const {
		const int scoreBoard = p[0][0] - p[1][0] + p[2][0];
		const int scoreTurn = p[0][1] + p[1][1] + p[2][1];
		return (c == BLACK ? scoreBoard : -scoreBoard) + scoreTurn;
	}
	EvalSum& operator += (const EvalSum& rhs) {
#if defined USE_AVX2_EVAL
		mm = _mm256_add_epi32(mm, rhs.mm);
#elif defined USE_SSE_EVAL
		m[0] = _mm_add_epi32(m[0], rhs.m[0]);
		m[1] = _mm_add_epi32(m[1], rhs.m[1]);
#else
		p[0][0] += rhs.p[0][0];
		p[0][1] += rhs.p[0][1];
		p[1][0] += rhs.p[1][0];
		p[1][1] += rhs.p[1][1];
		p[2][0] += rhs.p[2][0];
		p[2][1] += rhs.p[2][1];
#endif
		return *this;
	}
	EvalSum& operator -= (const EvalSum& rhs) {
#if defined USE_AVX2_EVAL
		mm = _mm256_sub_epi32(mm, rhs.mm);
#elif defined USE_SSE_EVAL
		m[0] = _mm_sub_epi32(m[0], rhs.m[0]);
		m[1] = _mm_sub_epi32(m[1], rhs.m[1]);
#else
		p[0][0] -= rhs.p[0][0];
		p[0][1] -= rhs.p[0][1];
		p[1][0] -= rhs.p[1][0];
		p[1][1] -= rhs.p[1][1];
		p[2][0] -= rhs.p[2][0];
		p[2][1] -= rhs.p[2][1];
#endif
		return *this;
	}
	EvalSum operator + (const EvalSum& rhs) const { return EvalSum(*this) += rhs; }
	EvalSum operator - (const EvalSum& rhs) const { return EvalSum(*this) -= rhs; }

	union {
		std::array<std::array<int, 2>, 3> p;
		/*
		struct {
		unsigned _int64 data[3];
		unsigned _int64 key; // ehash�p�B
		};
		*/
#if defined USE_AVX2_EVAL
		__m256i mm;
#endif
#if defined USE_AVX2_EVAL || defined USE_SSE_EVAL
		__m128i m[2];
#endif
	};
};
#endif
#endif


/// Position class stores information regarding the board representation as
/// pieces, side to move, hash keys, castling info, etc. Important methods are
/// do_move() and undo_move(), used by the search to update node info when
/// traversing the search tree.
class Thread;

class Position {

public:
  static void init();
  Position() = default; // To define the global object RootPos
  Position(const Position&) = delete;
public:
  Position(const Position& pos, Thread* th) { *this = pos; thisThread = th; }

  // FEN string input/output
#ifdef NANOHA
  Position& set(const std::string& fenStr, StateInfo* si, Thread* th);
#else
  Position& set(const std::string& fenStr, bool isChess960, StateInfo* si, Thread* th);
#endif
  const std::string fen() const;
#ifdef NANOHA
	void print_csa(Move m = MOVE_NONE) const;
	void print(Move m) const;
	void print() const {print(MOVE_NULL);}
	bool operator == (const Position &a) const;
	bool operator != (const Position &a) const;
#else
	void print(Move m = MOVE_NONE) const;
#endif


#ifndef NANOHA
  // Position representation
  Bitboard pieces() const;
  Bitboard pieces(PieceType pt) const;
  Bitboard pieces(PieceType pt1, PieceType pt2) const;
  Bitboard pieces(Color c) const;
  Bitboard pieces(Color c, PieceType pt) const;
  Bitboard pieces(Color c, PieceType pt1, PieceType pt2) const;
#endif
  Piece piece_on(Square s) const;
#ifndef NANOHA
  Square ep_square() const;
#endif
  bool empty(Square s) const;
#if defined(NANOHA)
  Value type_value_of_piece_on(Square s) const;
  Value promote_value_of_piece_on(Square s) const;
  int pin_on(Square s) const;
#else
  template<PieceType Pt> int count(Color c) const;
  template<PieceType Pt> const Square* squares(Color c) const;
  template<PieceType Pt> Square square(Color c) const;
#endif

#ifndef NANOHA
  // Castling
  int can_castle(Color c) const;
  int can_castle(CastlingRight cr) const;
  bool castling_impeded(CastlingRight cr) const;
  Square castling_rook_square(CastlingRight cr) const;
#endif

#ifndef NANOHA
  // Checking
  Bitboard checkers() const;
  Bitboard discovered_check_candidates() const;
  Bitboard pinned_pieces(Color c) const;

  // Attacks to/from a given square
  Bitboard attackers_to(Square s) const;
  Bitboard attackers_to(Square s, Bitboard occupied) const;
  Bitboard attacks_from(Piece pc, Square s) const;
  template<PieceType> Bitboard attacks_from(Square s) const;
  template<PieceType> Bitboard attacks_from(Square s, Color c) const;
#else
  	bool in_check() const;
	bool at_checking() const;	// ����������Ă����Ԃ��H
#endif

  // Properties of moves
#ifdef NANOHA
	bool is_double_pawn(const Color us, int to) const;		// ������H
	bool is_pawn_drop_mate(const Color us, int to) const;	// �ł����l�߂��H
	bool move_gives_check(Move m) const;
	bool move_attacks_square(Move m, Square s) const;
	bool pl_move_is_legal(const Move m) const;
	bool pseudo_legal(const Move m) const {return pl_move_is_legal(m);}
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  Piece captured_piece() const;
  Piece moved_piece(Move m) const;
  bool legal(Move m) const { return pl_move_is_legal(m); }
#else
  bool legal(Move m, Bitboard pinned) const;
  bool pseudo_legal(const Move m) const;
  bool capture(Move m) const;
  bool capture_or_promotion(Move m) const;
  bool gives_check(Move m, const CheckInfo& ci) const;
  bool advanced_pawn_push(Move m) const;
  PieceType captured_piece_type() const;
#endif


#ifndef NANOHA
  // Piece specific
  bool pawn_passed(Color c, Square s) const;
  bool opposite_bishops() const;
#endif

#ifdef NANOHA
  void do_move(Move m, StateInfo& st,int count=1);
  void do_drop(Move m);
  void undo_move(Move m);
  void undo_drop(Move m);
  void do_null_move(StateInfo& st);
  void undo_null_move();
  // ���i�߂��Ƀn�b�V���v�Z�̂ݍs��
#ifndef USAPYON2
  uint64_t calc_hash_no_move(const Move m) const;
#endif
#else
  // Doing and undoing moves
  void do_move(Move m, StateInfo& st, bool givesCheck);
  void undo_move(Move m);
  void do_null_move(StateInfo& st);
  void undo_null_move();
#endif


#if defined(NANOHA)
	// �萶���Ŏg�����߂̊֐�
	// �w��ʒu����w������ɉ�������ʒu�܂ŒT��(WALL or ���� or ����̈ʒu�ɂȂ�)
	int SkipOverEMP(int pos, const int dir) const;
	// �����̍X�V
	void add_effect(const int z);					// �ʒuz�̋�̗����𔽉f����
	void del_effect(const int z, const Piece k);	
	// �����̉��Z
	template<Color>
	void add_effect_straight(const int z, const int dir, const uint32_t bit);
	template<Color>
	void del_effect_straight(const int z, const int dir, const uint32_t bit);
#define AddKikiDirS	add_effect_straight<BLACK>
#define AddKikiDirG	add_effect_straight<WHITE>
#define DelKikiDirS	del_effect_straight<BLACK>
#define DelKikiDirG	del_effect_straight<WHITE>
	// �s�����X�V
	template <Color> void add_pin_info(const int dir);
	template <Color> void del_pin_info(const int dir);
#define AddPinInfS	add_pin_info<BLACK>
#define AddPinInfG	add_pin_info<WHITE>
#define DelPinInfS	del_pin_info<BLACK>
#define DelPinInfG	del_pin_info<WHITE>

public:
	// �萶��
	template<Color> ExtMove* add_straight(ExtMove* mlist, const int from, const int dir) const;
	template<Color> ExtMove* add_move(ExtMove* mlist, const int from, const int dir) const;
#define add_straightB	add_straight<BLACK>
#define add_straightW	add_straight<WHITE>
#define add_moveB	add_move<BLACK>
#define add_moveW	add_move<WHITE>
	ExtMove* gen_move_to(const Color us, ExtMove* mlist, int to) const;		// to�ɓ�����̐���
	ExtMove* gen_drop_to(const Color us, ExtMove* mlist, int to) const;		// to�ɋ��ł�̐���
	template <Color> ExtMove* gen_drop(ExtMove* mlist) const;			// ���ł�̐���
	ExtMove* gen_move_king(const Color us, ExtMove* mlist, int pindir = 0) const;			//�ʂ̓�����̐���
	ExtMove* gen_king_noncapture(const Color us, ExtMove* mlist, int pindir = 0) const;			//�ʂ̓�����̐���
	ExtMove* gen_move_from(const Color us, ExtMove* mlist, int from, int pindir = 0) const;		//from���瓮����̐���

	template <Color> ExtMove* generate_capture(ExtMove* mlist) const;
	template <Color> ExtMove* generate_non_capture(ExtMove* mlist) const;
	template <Color> ExtMove* generate_evasion(ExtMove* mlist) const;
	template <Color> ExtMove* generate_non_evasion(ExtMove* mlist) const;
	template <Color> ExtMove* generate_legal(ExtMove* mlist) const;

	// ����֘A
	template <Color> ExtMove* gen_check_long(ExtMove* mlist) const;
	template <Color> ExtMove* gen_check_short(ExtMove* mlist) const;
	template <Color> ExtMove* gen_check_drop(ExtMove* mlist, bool &bUchifudume) const;
	template <Color> ExtMove* generate_check(ExtMove* mlist, bool &bUchifudume) const;

	// 3��l�ߗp�̎萶��
	template <Color> ExtMove* gen_check_drop3(ExtMove* mlist, bool &bUchifudume) const;
	template <Color> ExtMove* generate_check3(ExtMove* mlist, bool &bUchifudume) const;			// ���萶��
	// ��������̐���(3��l�ߎc��2��p)
	ExtMove *generate_evasion_rest2(const Color us, ExtMove *mBuf, effect_t effect, int &Ai);
	ExtMove *generate_evasion_rest2_MoveAi(const Color us, ExtMove *mBuf, effect_t effect);
	ExtMove *generate_evasion_rest2_DropAi(const Color us, ExtMove *mBuf, effect_t effect, int &check_pos);

	// �ǖʂ̕]��
	static void init_evaluate();
#if defined(EVAL_APERY)
	int make_list_apery(int list0[], int list1[], int nlist) const;
#else
	int make_list(int * pscore, int list0[], int list1[] ) const;
#endif

#ifndef EVAL_DIFF
	int evaluate(const Color us) const;
#endif // !EVAL_DIFF

#ifdef EVAL_DIFF
	int evaluate_raw_all(const Color us) const;
	EvalSum evaluate_make_list_diff(const Color us);
	Value evaluate(const Color us, Search::Stack* ss);
	void init_make_list();
	void make_list_move(PieceNumber_t kn, Piece piece, Square to);
	void make_list_undo_move(PieceNumber_t kn);
	void make_list_capture(PieceNumber_t kn, Piece captureType);
	void make_list_undo_capture(PieceNumber_t kn);
	PieceNumber_t make_list_drop(Piece piece, Square to);
	void make_list_undo_drop(PieceNumber_t kn, Piece piece);

	int list0[MAX_PIECENUMBER];
	int list1[MAX_PIECENUMBER];

	PieceNumber_t listkn[90]; //list0�̋�ԍ�num
	int handcount[32]; //Piece�̎����
	EvalSum doapc(const int index[2]) const;
	bool calcDifference(Search::Stack* ss) const;
#endif

	// ��딻��(bInaniwa �ɃZ�b�g���邽�� const �łȂ�)
	bool IsInaniwa(const Color us);

	// �@�\�F�����錾�ł��邩�ǂ������肷��
	bool IsKachi(const Color us) const;

	// ���肩�ǂ����H
	bool is_check_move(const Color us, Move move) const;

	// ����1��l��
	static void initMate1ply();
	template <Color us> uint32_t infoRound8King() const;
	// ��ł��ɂ�藘�����Ղ邩�H
	template <Color us> uint32_t chkInterceptDrop(const uint32_t info, const int kpos, const int i, const int kind) const;
	// ��ړ��ɂ�藘�����Ղ邩�H
	template <Color us> uint32_t chkInterceptMove(const uint32_t info, const int kpos, const int i, const int from, const int kind) const;
	// ��łɂ����l�̔���
	template <Color us> uint32_t CheckMate1plyDrop(const uint32_t info, Move &m) const;
	int CanAttack(const int kind, const int from1, const int from2, const int to) const;
	// �ړ��ɂ����l�̔���
	template <Color us> uint32_t CheckMate1plyMove(const uint32_t info, Move &m, const int check = 0) const;
	template <Color us> int Mate1ply(Move &m, uint32_t &info);

	// 3��l��
	int Mate3(const Color us, Move &m);
//	int EvasionRest2(const Color us, ExtMove *antichecks, unsigned int &PP, unsigned int &DP, int &dn);
	int EvasionRest2(const Color us, ExtMove *antichecks);

	template<Color>
	effect_t exist_effect(int pos) const;				// ����
	template<Color us>
	int sq_king() const {return (us == BLACK) ? knpos[1] : knpos[2];}

	// �ǖʂ�Huffman����������
	int EncodeHuffman(unsigned char buf[32]) const;
#endif

  // Static exchange evaluation
  Value see(Move m) const;
  Value see_sign(Move m) const;

  // Accessing hash keys
  Key key() const;
  Key key_after(Move m) const;
  Key exclusion_key() const;
#ifndef NANOHA
  Key material_key() const;
  Key pawn_key() const;
#endif

  // Other properties of the position
  Color side_to_move() const;
#ifndef NANOHA
  Phase game_phase() const;
#endif
  int game_ply() const;
#ifndef NANOHA
  bool is_chess960() const;
#endif
  Thread* this_thread() const;
#if defined(NANOHA)
	int64_t tnodes_searched() const;
	void set_tnodes_searched(int64_t n);
#if defined(CHK_PERFORM)
	unsigned long mate3_searched() const;
	void set_mate3_searched(unsigned long  n);
	void inc_mate3_searched(unsigned long  n=1);
#endif // defined(CHK_PERFORM)
#endif
  uint64_t nodes_searched() const;
  void set_nodes_searched(uint64_t n);
#ifdef NANOHA
  bool is_draw(int &ret) const;
#else
  bool is_draw() const;
#endif

#ifndef NANOHA
  int rule50_count() const;
#endif

#ifndef NANOHA
  Score psq_score() const;
  Value non_pawn_material(Color c) const;
#endif

  // Position consistency check, for debugging
  bool pos_is_ok(int* failedStep = nullptr) const;
  void flip();
#ifdef NANOHA
  inline Color flip(Color side) const {
	  return (Color)(side ^ 1);
  }
#endif
#if defined(NANOHA)
	uint32_t handValue_of_side() const {return hand[sideToMove].h; }
	template<Color us> uint32_t handValue() const {return hand[us].h; }
	int get_material() const { return material; }
#endif

#if defined(NANOHA)
	static unsigned char relate_pos(int z1, int z2) {return DirTbl[z1][z2];}	// z1��z2�̈ʒu�֌W.
#endif

private:
  // Initialization helpers (used while setting up a position)
  void clear();
#ifndef NANOHA
  void set_castling_right(Color c, Square rfrom);
#endif
  void set_state(StateInfo* si) const;
  void set_check_info(StateInfo* si) const;

public:
	bool move_is_legal(const Move m) const;

private:

#if defined(NANOHA)
	Key compute_key() const;
	int compute_material() const;
	void init_position(const unsigned char board_ori[9][9], const int Mochigoma_ori[]);
	void make_pin_info();
	void init_effect();
#endif

#ifndef NANOHA
  // Other helpers
  Bitboard check_blockers(Color c, Color kingColor) const;
  void put_piece(Color c, PieceType pt, Square s);
  void remove_piece(Color c, PieceType pt, Square s);
  void move_piece(Color c, PieceType pt, Square from, Square to);
  template<bool Do>
  void do_castling(Color us, Square from, Square& to, Square& rfrom, Square& rto);

  // Data members
  Piece board[SQUARE_NB];
  Bitboard byTypeBB[PIECE_TYPE_NB];
  Bitboard byColorBB[COLOR_NB];
  int pieceCount[COLOR_NB][PIECE_TYPE_NB];
  Square pieceList[COLOR_NB][PIECE_TYPE_NB][16];
  int index[SQUARE_NB];
  int castlingRightsMask[SQUARE_NB];
  Square castlingRookSquare[CASTLING_RIGHT_NB];
  Bitboard castlingPath[CASTLING_RIGHT_NB];
#endif
  uint64_t nodes;
  int gamePly;
  Color sideToMove;
  Thread* thisThread;
  StateInfo* st;
#if defined(NANOHA)
	int64_t tnodes;
	unsigned long count_Mate1plyDrop;		// ��ł��ŋl�񂾉�
	unsigned long count_Mate1plyMove;		// ��ړ��ŋl�񂾉�
	unsigned long count_Mate3ply;			// Mate3()�ŋl�񂾉�
#endif

#ifndef NANOHA
  bool chess960;
#endif

#if defined(NANOHA)
	Piece banpadding[16*2];		// Padding
	Piece ban[16*12];			// �Տ�� (����)
	PieceNumber_t komano[16*12];		// �Տ�� (��ԍ�)
#define MAX_KOMANO	40
	effect_t effect[2][16*12];				// ����
#define effectB	effect[BLACK]
#define effectW	effect[WHITE]

#define IsCheckS()	EXIST_EFFECT(effectW[kingS])	/* ���ʂɉ��肪�������Ă��邩? */
#define IsCheckG()	EXIST_EFFECT(effectB[kingG])	/* ���ʂɉ��肪�������Ă��邩? */
	int pin[16*10];					// �s��(���ƌ�藼�p)
	Hand hand[2];					// ����
#define handS	hand[BLACK]
#define handG	hand[WHITE]
	PieceKind_t knkind[MAX_PIECENUMBER+1];	// knkind[num] : ��ԍ�num�̋���(EMP(0x00) �` GRY(0x1F))
	uint8_t knpos[MAX_PIECENUMBER+1];		// knpos[num]  : ��ԍ�num�̔Տ�̍��W(0:���g�p�A1:��莝��A2:��莝��A0x11-0x99:�Տ�)
									//    ��ԍ�
#define KNS_SOU	1
#define KNE_SOU	1
								//        1    : ����
#define KNS_GOU	2
#define KNE_GOU	2
								//        2    : ����
#define KNS_HI	3
#define KNE_HI	4
								//        3- 4 : ��
#define KNS_KA	5
#define KNE_KA	6
								//        5- 6 : �p
#define KNS_KI	7
#define KNE_KI	10
								//        7-10 : ��
#define KNS_GI	11
#define KNE_GI	14
								//       11-14 : ��
#define KNS_KE	15
#define KNE_KE	18
								//       15-18 : �j
#define KNS_KY	19
#define KNE_KY	22
								//       19-22 : ��
#define KNS_FU	23
#define KNE_FU	40
								//       23-40 : ��
#define kingS	sq_king<BLACK>()
#define kingG	sq_king<WHITE>()
#define hiPos	(&knpos[ 3])
#define kaPos	(&knpos[ 5])
#define kyPos	(&knpos[19])
#define IsHand(x)	((x) <  0x11)
#define OnBoard(x)	((x) >= 0x11)
	int material;
	bool bInaniwa;

#endif

#if defined(NANOHA)
//  static Key zobrist[2][RY+1][0x100];
	static Key zobrist[GRY+1][0x100];
	static Key zobSideToMove;		// ��Ԃ���ʂ���
	static Key zobExclusion;		// NULL MOVE���ǂ�����ʂ���
#ifdef USAPYON2
	static Key zobHand[GRY + 1][32];
#endif
	static unsigned char DirTbl[0xA0][0x100];	// �����p[from][to]

	// ���萶���p�e�[�u��
	static const struct ST_OuteMove2 {
		int from;
		struct {
			int to;
			int narazu;
			int nari;
		} to[6];
	} OuteMove2[32];
	static uint32_t TblMate1plydrop[0x10000];	// ��ł��ŋl�ޔ��f������e�[�u��.

	friend void init_application_once();	// ���s�t�@�C���N�����ɍs��������.
	friend class SearchMateDFPN;
	friend class Book;
#endif

};

extern std::ostream& operator<<(std::ostream& os, const Position& pos);

inline Color Position::side_to_move() const {
  return sideToMove;
}

inline bool Position::empty(Square s) const {
#ifdef NANOHA
	return ban[s] == EMP;
#else
	return board[s] == NO_PIECE;
#endif
}

#if defined(NANOHA)
inline Value Position::promote_value_of_piece_on(Square s) const {
	return Value(NanohaTbl::KomaValuePro[type_of(piece_on(s))]);
}

inline int Position::pin_on(Square s) const {
	return pin[s];
}
#endif

inline Piece Position::piece_on(Square s) const {
#ifdef NANOHA  
	return ban[s];
#else
	return board[s];
#endif
}

inline Piece Position::moved_piece(Move m) const {
	return move_piece(m);
}

#ifndef NANOHA

inline Bitboard Position::pieces() const {
  return byTypeBB[ALL_PIECES];
}

inline Bitboard Position::pieces(PieceType pt) const {
  return byTypeBB[pt];
}

inline Bitboard Position::pieces(PieceType pt1, PieceType pt2) const {
  return byTypeBB[pt1] | byTypeBB[pt2];
}

inline Bitboard Position::pieces(Color c) const {
  return byColorBB[c];
}

inline Bitboard Position::pieces(Color c, PieceType pt) const {
  return byColorBB[c] & byTypeBB[pt];
}

inline Bitboard Position::pieces(Color c, PieceType pt1, PieceType pt2) const {
  return byColorBB[c] & (byTypeBB[pt1] | byTypeBB[pt2]);
}

template<PieceType Pt> inline int Position::count(Color c) const {
  return pieceCount[c][Pt];
}

template<PieceType Pt> inline const Square* Position::squares(Color c) const {
  return pieceList[c][Pt];
}

template<PieceType Pt> inline Square Position::square(Color c) const {
  assert(pieceCount[c][Pt] == 1);
  return pieceList[c][Pt][0];
}

inline Square Position::ep_square() const {
  return st->epSquare;
}

inline int Position::can_castle(CastlingRight cr) const {
  return st->castlingRights & cr;
}

inline int Position::can_castle(Color c) const {
  return st->castlingRights & ((WHITE_OO | WHITE_OOO) << (2 * c));
}

inline bool Position::castling_impeded(CastlingRight cr) const {
  return byTypeBB[ALL_PIECES] & castlingPath[cr];
}

inline Square Position::castling_rook_square(CastlingRight cr) const {
  return castlingRookSquare[cr];
}

template<PieceType Pt>
inline Bitboard Position::attacks_from(Square s) const {
  return  Pt == BISHOP || Pt == ROOK ? attacks_bb<Pt>(s, byTypeBB[ALL_PIECES])
        : Pt == QUEEN  ? attacks_from<ROOK>(s) | attacks_from<BISHOP>(s)
        : StepAttacksBB[Pt][s];
}

template<>
inline Bitboard Position::attacks_from<PAWN>(Square s, Color c) const {
  return StepAttacksBB[make_piece(c, PAWN)][s];
}

inline Bitboard Position::attacks_from(Piece pc, Square s) const {
  return attacks_bb(pc, s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::attackers_to(Square s) const {
  return attackers_to(s, byTypeBB[ALL_PIECES]);
}

inline Bitboard Position::checkers() const {
  return st->checkersBB;
}

inline Bitboard Position::discovered_check_candidates() const {
  return st->blockersForKing[~sideToMove] & pieces(sideToMove);
}

inline Bitboard Position::pinned_pieces(Color c) const {
  return st->blockersForKing[c] & pieces(c);
}

inline Bitboard Position::check_squares(PieceType pt) const {
  return st->checkSquares[pt];
}

inline bool Position::pawn_passed(Color c, Square s) const {
  return !(pieces(~c, PAWN) & passed_pawn_mask(c, s));
}

inline bool Position::advanced_pawn_push(Move m) const {
  return   type_of(moved_piece(m)) == PAWN
        && relative_rank(sideToMove, from_sq(m)) > RANK_4;
}
#endif

inline bool Position::in_check() const {
#if defined(NANOHA)
	int pos = (sideToMove==BLACK) ? sq_king<BLACK>(): sq_king<WHITE>();
	if (pos == 0) return false;
	Color them = flip(sideToMove);
	return EXIST_EFFECT(effect[them][pos]);
#else
	return st->checkersBB != 0;
#endif
}

#if defined(NANOHA)
inline bool Position::at_checking() const {
	int pos = (sideToMove == BLACK) ? sq_king<WHITE>() : sq_king<BLACK>();
	if (pos == 0) return false;
	return EXIST_EFFECT(effect[sideToMove][pos]);
}
#endif

inline Key Position::key() const {
  return st->key;
}

#ifndef NANOHA
inline Key Position::pawn_key() const {
  return st->pawnKey;
}

inline Key Position::material_key() const {
  return st->materialKey;
}

inline Score Position::psq_score() const {
  return st->psq;
}

inline Value Position::non_pawn_material(Color c) const {
  return st->nonPawnMaterial[c];
}
#endif

inline int Position::game_ply() const {
  return gamePly;
}

#ifndef NANOHA
inline int Position::rule50_count() const {
  return st->rule50;
}
#endif

inline uint64_t Position::nodes_searched() const {
  return nodes;
}

inline void Position::set_nodes_searched(uint64_t n) {
  nodes = n;
}

#if defined(NANOHA)
inline int64_t Position::tnodes_searched() const {
	return tnodes;
}

inline void Position::set_tnodes_searched(int64_t n) {
	tnodes = n;
}

#if defined(CHK_PERFORM)
inline unsigned long Position::mate3_searched() const {
	return count_Mate3ply;
}
inline void Position::set_mate3_searched(unsigned long  n) {
	count_Mate3ply = n;
}
inline void Position::inc_mate3_searched(unsigned long  n) {
	count_Mate3ply += n;
}
#endif // defined(CHK_PERFORM)

#endif

#ifndef NANOHA
inline bool Position::opposite_bishops() const {
  return   pieceCount[WHITE][BISHOP] == 1
        && pieceCount[BLACK][BISHOP] == 1
        && opposite_colors(square<BISHOP>(WHITE), square<BISHOP>(BLACK));
}

inline bool Position::is_chess960() const {
  return chess960;
}
#endif

inline bool Position::capture_or_promotion(Move m) const {
#if defined(NANOHA)
	return move_captured(m) != EMP || is_promotion(m);
#else

  assert(is_ok(m));
  return type_of(m) != NORMAL ? type_of(m) != CASTLING : !empty(to_sq(m));
#endif
}

inline bool Position::capture(Move m) const {
#if defined(NANOHA)
	return !empty(move_to(m));
#else

  // Castling is encoded as "king captures the rook"
  assert(is_ok(m));
  return (!empty(to_sq(m)) && type_of(m) != CASTLING) || type_of(m) == ENPASSANT;
#endif
}

inline Piece Position::captured_piece() const {
  return st->capturedPiece;
}

inline Thread* Position::this_thread() const {
  return thisThread;
}

#ifndef NANOHA
inline void Position::put_piece(Color c, PieceType pt, Square s) {

  board[s] = make_piece(c, pt);
  byTypeBB[ALL_PIECES] |= s;
  byTypeBB[pt] |= s;
  byColorBB[c] |= s;
  index[s] = pieceCount[c][pt]++;
  pieceList[c][pt][index[s]] = s;
  pieceCount[c][ALL_PIECES]++;
}

inline void Position::remove_piece(Color c, PieceType pt, Square s) {

  // WARNING: This is not a reversible operation. If we remove a piece in
  // do_move() and then replace it in undo_move() we will put it at the end of
  // the list and not in its original place, it means index[] and pieceList[]
  // are not guaranteed to be invariant to a do_move() + undo_move() sequence.
  byTypeBB[ALL_PIECES] ^= s;
  byTypeBB[pt] ^= s;
  byColorBB[c] ^= s;
  /* board[s] = NO_PIECE;  Not needed, overwritten by the capturing one */
  Square lastSquare = pieceList[c][pt][--pieceCount[c][pt]];
  index[lastSquare] = index[s];
  pieceList[c][pt][index[lastSquare]] = lastSquare;
  pieceList[c][pt][pieceCount[c][pt]] = SQ_NONE;
  pieceCount[c][ALL_PIECES]--;
}

inline void Position::move_piece(Color c, PieceType pt, Square from, Square to) {

  // index[from] is not updated and becomes stale. This works as long as index[]
  // is accessed just by known occupied squares.
  Bitboard from_to_bb = SquareBB[from] ^ SquareBB[to];
  byTypeBB[ALL_PIECES] ^= from_to_bb;
  byTypeBB[pt] ^= from_to_bb;
  byColorBB[c] ^= from_to_bb;
  board[from] = NO_PIECE;
  board[to] = make_piece(c, pt);
  index[to] = index[from];
  pieceList[c][pt][index[to]] = to;
}
#endif

#if defined(NANOHA)
// �ȉ��A�Ȃ̂͌ŗL����
template<Color us>
effect_t Position::exist_effect(int pos) const {
	return effect[us][pos];
}

// �w��ʒu����w������ɉ�������ʒu�܂ŒT��(WALL or ���� or ����̈ʒu�ɂȂ�)
inline int Position::SkipOverEMP(int pos, const int dir) const {
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	if (ban[pos += dir] != EMP) return pos;
	return pos + dir;
}

// ����`�F�b�N(true:pos�̋؂ɕ������遁����ɂȂ�Afalse:pos�̋؂ɕ����Ȃ�)
inline bool Position::is_double_pawn(const Color us, const int pos) const
{
	const Piece *p = &(ban[(pos & ~0x0F)+1]);
	Piece pawn = (us == BLACK) ? SFU : GFU;
	for (int i = 0; i < 9; i++) {
		if (*p++ == pawn) {
			return true;
		}
	}
	return false;
}

// �����֘A
template<Color turn>
inline void Position::add_effect_straight(const int z, const int dir, const uint32_t bit)
{
	int zz = z;
	do {
		zz += dir;
		effect[turn][zz] |= bit;
	} while(ban[zz] == EMP);

	// �����͑���ʂ�������т�
	const int enemyKing = (turn == BLACK) ? GOU : SOU;
	if (ban[zz] == enemyKing) {
		zz += dir;
		if (ban[zz] != WALL) {
			effect[turn][zz] |= bit;
		}
	}
}
template<Color turn>
inline void Position::del_effect_straight(const int z, const int dir, const uint32_t bit)
{
	int zz = z;
	do {
		zz += dir; effect[turn][zz] &= bit;
	} while(ban[zz] == EMP);

	// �����͑���ʂ�������т�
	const int enemyKing = (turn == BLACK) ? GOU : SOU;
	if (ban[zz] == enemyKing) {
		zz += dir;
		if (ban[zz] != WALL) {
			effect[turn][zz] &= bit;
		}
	}
}

// �s�����X�V
template<Color turn>
inline void Position::add_pin_info(const int dir) {
	int z;
	const Color rturn = (turn == BLACK) ? WHITE : BLACK;
	z = (turn == BLACK) ? SkipOverEMP(kingS, -dir) : SkipOverEMP(kingG, -dir);
	if (ban[z] != WALL) {
		if ((turn == BLACK && (ban[z] & GOTE) == 0)
		 || (turn == WHITE && (ban[z] & GOTE) != 0)) {
			effect_t eft = (turn == BLACK) ? EFFECT_KING_S(z) : EFFECT_KING_G(z);
			if (eft & (effect[rturn][z] >> EFFECT_LONG_SHIFT)) pin[z] = dir;
		}
	}
}
template<Color turn>
void Position::del_pin_info(const int dir) {
	int z;
	z = (turn == BLACK) ? SkipOverEMP(kingS, -dir) : SkipOverEMP(kingG, -dir);
	if (ban[z] != WALL) {
		if ((turn == BLACK && (ban[z] & GOTE) == 0)
		 || (turn == WHITE && (ban[z] & GOTE) != 0)) {
			pin[z] = 0;
		}
	}
}
#endif


#endif // #ifndef POSITION_H_INCLUDED
