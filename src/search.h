#include "types.h"

namespace Ignis
{
    // inf ve mat farklı olmalı galiba :
    uint32_t constexpr INF = UINT32_MAX;
    uint32_t constexpr MATE_VALUE = UINT32_MAX;
    uint32_t constexpr STALEMATE_VALUE = UINT32_MAX; // şimdilik mat gibi sayılsın

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
    public:
        Engine() = default;

        BitMove getBestMove     (BitBoard& board, size_t maxDepth, int timeMs); // mainSide burada yenilenecek
        int32_t search          (BitBoard& board, size_t depth, int32_t alpha, int32_t beta);

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