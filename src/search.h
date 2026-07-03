#include "types.h"

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

    class Engine
    {
    private:
        Color mainSide = WHITE;

        // helpers
        int popcount(Bitboard b) { return __builtin_popcountll(b); }
    public:
        Engine() = default;

        BitMove getBestMove     (BitBoard& board, size_t maxDepth, int timeMs); // mainSide burada yenilenecek
        int32_t search          (BitBoard& board, size_t depth, int32_t alpha, int32_t beta);
        int32_t quiescence      (BitBoard& board, int32_t alpha, int32_t beta);

        int32_t mvvLva          (const BitBoard& board, const BitMove& mv) const;

        // evulate fonctions :
        int32_t evulate        (const BitBoard& board);

        // şimdilik boş olacak evulate helperları :
        int32_t evuCentrPos    (const BitBoard& board);
        int32_t evuPiecesRaw   (const BitBoard& board);
        int32_t evuPiecesPos   (const BitBoard& board);
        int32_t evuPawnPos     (const BitBoard& board);
        int32_t evuCastling    (const BitBoard& board);
    };
}
