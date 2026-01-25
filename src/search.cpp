#include "search.h"
#include "time.h"
// __builtin_popcountll();

namespace Ignis
{
    BitMove Engine::getBestMove(BitBoard& board, size_t maxDepth, int timeMs)
    {
        mainSide = board.getTurn();
        auto moves = board.getValidMoves(mainSide); 
        if (moves.empty() || timeMs <= 0) return BitMove();

        BitMove bestMove = moves[0];
        int32_t bestScore = -INF;

        auto start = std::chrono::steady_clock::now();
        Time timer; timer.start();

        for (size_t depth = 1; depth <= maxDepth; ++depth)
        {
            int32_t alpha = -INF;
            int32_t beta  =  INF;
            int32_t localBestScore = -INF;
            BitMove localBestMove = moves[0];

            for (size_t i = 0; i < moves.size(); ++i)
            {
                if (timer.elapsedTime() >= timeMs) goto TIME_UP; // goto kullanarak marjinallik

                BitBoard tmp = board;
                const BitMove &mv = moves[i];
                tmp.makeMoveBlind(mv, mv.type);

                int32_t score = -search(tmp, depth - 1, -beta, -alpha);

                if (score > localBestScore)
                {
                    localBestScore = score;
                    localBestMove = mv;
                }

                alpha = std::max(alpha, score);
                if (alpha >= beta) break;
            }

            bestMove = localBestMove;
            bestScore = localBestScore;
        }

        TIME_UP:
        return bestMove;

        BitMove move;
    }

    int32_t Engine::search (BitBoard& board, size_t depth, int32_t alpha, int32_t beta)
    {
        auto moves = board.getValidMoves(board.getTurn());
        if (moves.empty())
        {
            if (board.isKingInCheck(board.getTurn()))
            {
                if (board.getTurn() == mainSide) return -   (MATE_VALUE - (int32_t)depth);
                else                             return     (MATE_VALUE - (int32_t)depth);
            }
            else return STALEMATE_VALUE;
        }

        if (depth == 0)
        {
            int32_t val = evulate(board);

            if (board.getTurn() != mainSide) val = -val; // negamax
            return val;
        }

        int32_t best = -INF;
        for (const auto& mv : moves)
        {
            BitBoard tmp = board;
            tmp.makeMoveBlind(mv, mv.type);

            int32_t score = -search(tmp, depth - 1, -beta, -alpha); // negamax

            best  = std::max(best, score);
            alpha = std::max(alpha, score);

            if (alpha >= beta) break;
        }
        return best;
    }

    int32_t Engine::evulate(const BitBoard& board)
    {
        return evuPiecesRaw(board);
    }

    int32_t Engine::evuPiecesRaw(const BitBoard& board)
    {
        return  PAWN_VALUE      * popcount(board.getPAWNS() & board.getWHITES())
                + KNIGHT_VALUE  * popcount(board.getKNIGHTS() & board.getWHITES())
                + BISHOP_VALUE  * popcount(board.getBISHOPS() & board.getWHITES())
                + ROOK_VALUE    * popcount(board.getROOKS() & board.getWHITES())
                + QUEEN_VALUE   * popcount(board.getQUEENS() & board.getWHITES())

                - PAWN_VALUE    * popcount(board.getPAWNS() & board.getBLACKS())
                - KNIGHT_VALUE  * popcount(board.getKNIGHTS() & board.getBLACKS())
                - BISHOP_VALUE  * popcount(board.getBISHOPS() & board.getBLACKS())
                - ROOK_VALUE    * popcount(board.getROOKS() & board.getBLACKS())
                - QUEEN_VALUE   * popcount(board.getQUEENS() & board.getBLACKS());
    }

    // helper : 
    inline int popcount(uint64_t bb) { return __builtin_popcountll(bb); }
}