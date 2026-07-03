#include "types.h"
#include "time.h"
#include <vector>

namespace Ignis
{
    int32_t constexpr INF = 2000000000;
    int32_t constexpr MATE_VALUE = 1000000000;
    int32_t constexpr STALEMATE_VALUE = 0;

    enum RawPieceValue
    {
        PAWN_VALUE =    100,
        KNIGHT_VALUE =  310,
        BISHOP_VALUE =  330,
        ROOK_VALUE =    500,
        QUEEN_VALUE =   900,
    };

    enum TTFlag : uint8_t { TT_EXACT, TT_LOWERBOUND, TT_UPPERBOUND };

    struct TTEntry
    {
        Bitboard key   = 0;
        int32_t  score = 0;
        int32_t  depth = -1;
        TTFlag   flag  = TT_EXACT;
        BitMove  bestMove;
    };

    size_t constexpr TT_SIZE = 1ull << 20;
    Bitboard constexpr TT_MASK = TT_SIZE - 1;

    size_t  constexpr MAX_PLY     = 128;
    int32_t constexpr HISTORY_MAX = 1000000;

    struct MoveOrderKey
    {
        int32_t tier;
        int32_t score;
    };

    class Engine
    {
    private:
        Color mainSide = WHITE;
        std::vector<TTEntry> tt;

        Time     timer;
        int      timeBudgetMs = 0;
        bool     timeUp = false;
        uint64_t nodeCount = 0;
        bool     checkTime();

        BitMove killerMoves[MAX_PLY][2];
        int32_t historyTable[COLOR_NB][PIECE_TYPE_NB][64] = {};

        PieceType     pieceTypeAt      (const BitBoard& board, Square sq) const;
        MoveOrderKey  moveOrderTier    (const BitBoard& board, const BitMove& mv, size_t ply, bool ttHit, const BitMove& ttMove) const;
        void          recordQuietCutoff(const BitBoard& board, const BitMove& mv, size_t ply, size_t depth);
        void          resetSearchTables(); // her gercek hamlede (getBestMove basinda) killer'i temizler, history'yi yariya boler

        void orderMoves(std::vector<BitMove>& moves, const BitBoard& board, size_t ply, bool ttHit, const BitMove& ttMove) const;

        // helpers
        int popcount(Bitboard b) { return __builtin_popcountll(b); }
    public:
        Engine() : tt(TT_SIZE) {}

        BitMove getBestMove     (BitBoard& board, size_t maxDepth, int timeMs);
        int32_t search          (BitBoard& board, size_t depth, size_t ply, int32_t alpha, int32_t beta, bool allowNull = true);
        int32_t quiescence      (BitBoard& board, int32_t alpha, int32_t beta);

        int32_t mvvLva          (const BitBoard& board, const BitMove& mv) const;

        int32_t evulate        (const BitBoard& board);

        int32_t evuCentrPos    (const BitBoard& board);
        int32_t evuPiecesRaw   (const BitBoard& board);
        int32_t evuPiecesPos   (const BitBoard& board);
        int32_t evuPawnPos     (const BitBoard& board);
        int32_t evuCastling    (const BitBoard& board);
    };
}
