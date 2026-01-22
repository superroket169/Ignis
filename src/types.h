#ifndef TYPES_H

#define TYPES_H
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

/**
 * int that header, we'll define :
 * - bitBoard s
 * - color/side enums
 * - piece type enums
 * - rank/file enums
 * - square enums
 * - direction enums (for bit sliding)
 * - sorry about my inexperienceive coding style
 */

namespace Ignis
{
    using Bitboard = uint64_t;

    enum Square
    {
        SQ_A1, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
        SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
        SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
        SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
        SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
        SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
        SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
        SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8 = 63, // should be 64 ? idk 
        SQ_NONE
    };

    enum Color
    {
        WHITE,
        BLACK,
        COLOR_NB = 2
    };

    enum PieceType
    {
        NO_PIECE_TYPE, PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING,
        PIECE_TYPE_NB = 8
    };

    enum Piece
    {
        NO_PIECE,
        W_PAWN, W_KNIGHT, W_BISHOP, W_ROOK, W_QUEEN, W_KING,
        B_PAWN, B_KNIGHT, B_BISHOP, B_ROOK, B_QUEEN, B_KING,
        PIECE_NB = 16
    };

    enum Direction
    {
        NORTH = 8,
        EAST  = 1,
        SOUTH = -NORTH,
        WEST  = -EAST,

        NORTH_EAST = NORTH + EAST,
        SOUTH_EAST = SOUTH + EAST,
        SOUTH_WEST = SOUTH + WEST,
        NORTH_WEST = NORTH + WEST
    };

    enum File : uint8_t
    {
        FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H  = 8,
        FILE_NB = 8
    };

    enum Rank : uint8_t
    {
        RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8  = 8,
        RANK_NB = 8
    };

    // is_ok functions
    constexpr bool is_ok(Square s) { return s >= SQ_A1 && s <= SQ_H8; }
    constexpr bool is_ok(File s) { return s >= FILE_A && s <= FILE_H; }
    constexpr bool is_ok(Rank s) { return s >= RANK_1 && s <= RANK_8; }

    constexpr File file_of(Square s) { return File (s & 7); } // (s % 8)
    constexpr Rank rank_of(Square s) { return Rank (s >> 3); } // (s / 8)
    // x * 8 = x << 3
    
    constexpr Direction pawn_dir(Color c) { return c == WHITE ? NORTH : SOUTH; }
    constexpr Bitboard square_bb(Square s) { return 1ULL << s; } // translation to bitboard from square
    constexpr Square filerank_square(File f, Rank r) { return Square((r << 3) + f); }

    // default operators
    constexpr Square& operator++(Square& d) { return d = Square(int(d) + 1); }
    constexpr Square& operator--(Square& d) { return d = Square(int(d) - 1); }

    constexpr Piece& operator++(Piece& d) { return d = Piece(int(d) + 1); }
    
    // Square & direction operations
    constexpr Square operator+(Square s, Direction d) { return Square(int(s) + int(d)); }
    constexpr Square operator-(Square s, Direction d) { return Square(int(s) - int(d)); }
    constexpr Square& operator+=(Square& s, Direction d) { return s = s + d; }
    constexpr Square& operator-=(Square& s, Direction d) { return s = s - d; }

    // File operators
    constexpr File operator+(File s1, File s2) { return File(int(s1) + int(s2)); }
    constexpr File operator-(File s1, File s2) { return File(int(s1) - int(s2)); }
    constexpr File& operator+=(File& s1, File s2) { return s1 = s1 + s2; }
    constexpr File& operator-=(File& s1, File s2) { return s1 = s1 - s2; }

    // Rank operators
    constexpr Rank operator+(Rank s1, Rank s2) { return Rank(int(s1) + int(s2)); }
    constexpr Rank operator-(Rank s1, Rank s2) { return Rank(int(s1) - int(s2)); }
    constexpr Rank& operator+=(Rank& s1, Rank s2) { return s1 = s1 + s2; }
    constexpr Rank& operator-=(Rank& s1, Rank s2) { return s1 = s1 - s2; }

    // Bitboard Operations
    constexpr Bitboard operator&(Bitboard b, Square s) { return b & square_bb(s); }
    constexpr Bitboard operator|(Bitboard b, Square s) { return b | square_bb(s); }
    constexpr Bitboard operator^(Bitboard b, Square s) { return b ^ square_bb(s); }
    constexpr Bitboard& operator|=(Bitboard& b, Square s) { return b = b | s; }
    constexpr Bitboard& operator^=(Bitboard& b, Square s) { return b = b ^ s; }
    
    enum MoveType
    {
        NORMAL,
        CAPTURE,
        PROMOTION,
        EN_PASSANT,
        CASTLING
    };
    
    // bunu neden enum class yaptım hiçbir fikrim yok / çakışma olmasın diye
    enum class PromotionPiece
    {
        None,
        Queen,
        Rook,
        Bishop,
        Knight
    };

    class BitMove
    {
    public:
        Square from;
        Square to;
        MoveType type;
        PromotionPiece promotion = PromotionPiece::None; // Varsayılan: Yok

        // default constructor
        BitMove() : from(SQ_NONE), to(SQ_NONE), type(NORMAL), promotion(PromotionPiece::None) {}

        // For normal moves constructor : 
        BitMove(Square f, Square t, MoveType ty = NORMAL) 
            : from(f), to(t), type(ty), promotion(PromotionPiece::None) {}
            
        // promotion & specia moves constructor : 
        BitMove(Square f, Square t, MoveType ty, PromotionPiece promo) 
            : from(f), to(t), type(ty), promotion(promo) {}

        bool operator==(const BitMove& other) const { return from == other.from && to == other.to && promotion == other.promotion; }
    };

    class BitBoard
    {
    private:
        // begin positions/locations
        Bitboard PAWNS      = 0b0000000011111111000000000000000000000000000000001111111100000000; // we need more readablity
        Bitboard KNIGHTS    = 0b0100001000000000000000000000000000000000000000000000000001000010;
        Bitboard BISHOPS    = 0b0010010000000000000000000000000000000000000000000000000000100100;
        Bitboard ROOKS      = 0b1000000100000000000000000000000000000000000000000000000010000001;
        Bitboard QUEENS     = 0b0001000000000000000000000000000000000000000000000000000000001000; // which side should queen be? idk
        Bitboard KINGS      = 0b0000100000000000000000000000000000000000000000000000000000010000;

        Bitboard WHITES     = 0b0000000000000000000000000000000000000000000000001111111111111111;
        Bitboard BLACKS     = 0b1111111111111100000000000000000000000000000000000000000000000000;

        // piece move controlers:
        Bitboard PawnAttacks[2][64];
        Bitboard KnightAttacks[64];
        Bitboard BishopAttacks[64];
        Bitboard RookAttacks[64];
        Bitboard QueenAttacks[64];
        Bitboard KingAttacks[64];

        // magic board şeyleri
        uint64_t RookMagic[64];
        uint64_t BishopMagic[64];

        // maskeler
        Bitboard RookMasks[64];
        Bitboard BishopMasks[64];
        
        // shiftler
        int RookShift[64];
        int BishopShift[64];

        // asıl final şeyleri. magicnumber!
        std::vector<Bitboard> RookTable[64];
        std::vector<Bitboard> BishopTable[64];

        // lookup initilazitons
        void initLookups();

        // specific values
        Bitboard enpassantTarget = 0b0000000000000000000000000000000000000000000000000000000000000000;

        // Castling values
        // white versions
        bool whiteCastlingQS = false; // can white queen side castling
        bool whiteCastlingKS = false; // can king side castling 

        // black versions
        bool blackCastlingQS = false;
        bool blackCastlingKS = false;

        // Move Side
        Color side = WHITE;
        void passTurn() { side = side == WHITE ? BLACK : WHITE;}

    public:
        // Constructors 
        // BitBoard() = default;
        BitBoard()
        {
            initLookups(); 
            // diğer değişkenleri tek tek değerlendirmemiz gerekiyor mu?
        };

        // get specific values
        bool getCastled(Color c) { return c == WHITE ? whiteCastlingQS && whiteCastlingKS : blackCastlingQS && blackCastlingKS; }

        // Move Functions
        std::optional<MoveType>     moveValidator   (const BitMove& move); // validates move
        std::optional<MoveType>     makeMove        (BitMove& move);       // for not validated moves (could return std::nullopt) uses moveValidator
        MoveType                    makeMoveBlind   (BitMove& move, MoveType type); // for already validated moves
        std::vector<BitMove>        getValidMoves   (void);                // returns valid moves

        // magic number helperları
        Bitboard getRookAttacks(Square sq, Bitboard occupancy);
        Bitboard getBishopAttacks(Square sq, Bitboard occupancy);

        // Validator helper functions
        std::optional<MoveType>     pawnValidator   (const BitMove& move);
        std::optional<MoveType>     knightValidator (const BitMove& move);
        std::optional<MoveType>     bishopValidator (const BitMove& move);
        std::optional<MoveType>     rookValidator   (const BitMove& move);
        std::optional<MoveType>     queenValidator  (const BitMove& move);
        std::optional<MoveType>     kingValidator   (const BitMove& move);

        // check & rook helpers
        bool checkClearPath(Square sq1, Square sq2);
        bool checkClearPath(Square sq1, Square sq2, Square sq3);
        std::optional<MoveType> castlingValidator(const BitMove& move);

        // game state & check helpers
        bool isSquareAttacked(Square sq, Color attackingSide);
        bool isKingInCheck  (void);
        bool getGameState   (void);

        // FEN functions
        void            loadFEN(const std::string& fen);
        std::string     getFEN(void) const;

        // hashing functions
        Bitboard getHash(Bitboard k);
    };
}// namespace Ignis

#endif