#include "search.h"
#include "time.h"
#include<iostream>
#include<algorithm>

namespace Ignis
{
    BitMove Engine::getBestMove(BitBoard& board, size_t maxDepth, int timeMs)
    {
        mainSide = board.getTurn();
        auto moves = board.getValidMoves(mainSide); 
        if (moves.empty() || timeMs <= 0) return BitMove();

        BitMove bestMove = moves[0];

        std::sort(moves.begin(), moves.end(), [&](const BitMove& a, const BitMove& b) {
            return mvvLva(board, a) > mvvLva(board, b);
        });

        Time timer; timer.start();

        for (size_t depth = 1; depth <= maxDepth; ++depth)
        {
            int32_t alpha = -INF;
            int32_t beta  =  INF;
            int32_t localBestScore = -INF;
            BitMove localBestMove = moves[0];

            for (size_t i = 0; i < moves.size(); ++i)
            {
                if (timer.elapsedTime() >= timeMs) goto TIME_UP;

                BitBoard tmp = board;
                const BitMove &mv = moves[i];
                tmp.makeMoveBlind(mv, mv.type);

                int32_t score = -search(tmp, depth - 1, -beta, -alpha);

                // old debug:
                // if (depth == maxDepth)
                // {
                //     std::cout << "DEBUG: Move "
                //             << (char)('a' + file_of(mv.from)) << (rank_of(mv.from)+1)
                //             << "-"
                //             << (char)('a' + file_of(mv.to)) << (rank_of(mv.to)+1)
                //             << " Score: " << score
                //             << " (Turn: " << (board.getTurn() == WHITE ? "W" : "B") << ")"
                //             << std::endl;
                // }

                if (score > localBestScore)
                {
                    localBestScore = score;
                    localBestMove = mv;
                }

                alpha = std::max(alpha, score);
                if (alpha >= beta) break;
            }

            bestMove = localBestMove;
            // bestScore = localBestScore;

            // CLI için test
            std::cout << "info depth " << depth << " time " << timer.elapsedTime() << "s bestmove "
                       << (char)('a' + file_of(bestMove.from)) << (rank_of(bestMove.from) + 1)
                       << (char)('a' + file_of(bestMove.to))   << (rank_of(bestMove.to) + 1)
                       << " score " << localBestScore << std::endl;
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
                return - (MATE_VALUE - (int32_t) depth);
            }

            return STALEMATE_VALUE;
        }

        if (depth == 0)
        {
            return quiescence(board, alpha, beta);
        }

        std::sort(moves.begin(), moves.end(), [&](const BitMove& a, const BitMove& b) {
            return mvvLva(board, a) > mvvLva(board, b);
        });

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

    int32_t Engine::quiescence(BitBoard& board, int32_t alpha, int32_t beta)
    {
        int32_t standPat = evulate(board);
        if (board.getTurn() == BLACK) standPat = -standPat;

        if (standPat >= beta) return beta;
        if (standPat > alpha) alpha = standPat;

        auto moves = board.getValidMoves(board.getTurn());
        std::sort(moves.begin(), moves.end(), [&](const BitMove& a, const BitMove& b) {
            return mvvLva(board, a) > mvvLva(board, b);
        });

        for (const auto& mv : moves)
        {
            if (mvvLva(board, mv) == 0) continue;

            BitBoard tmp = board;
            tmp.makeMoveBlind(mv, mv.type);

            int32_t score = -quiescence(tmp, -beta, -alpha);

            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        }

        return alpha;
    }

    int32_t Engine::mvvLva(const BitBoard& board, const BitMove& mv) const
    {
        Bitboard toBB  = square_bb(mv.to);
        Bitboard enemy = (board.getTurn() == WHITE) ? board.getBLACKS() : board.getWHITES();

        bool isCapture = (toBB & enemy) || mv.type == MoveType::EN_PASSANT;
        if (!isCapture) return 0;

        int32_t victimValue = PAWN_VALUE;
        if      (mv.type == MoveType::EN_PASSANT)  victimValue = PAWN_VALUE;
        else if (toBB & board.getKNIGHTS())         victimValue = KNIGHT_VALUE;
        else if (toBB & board.getBISHOPS())         victimValue = BISHOP_VALUE;
        else if (toBB & board.getROOKS())           victimValue = ROOK_VALUE;
        else if (toBB & board.getQUEENS())          victimValue = QUEEN_VALUE;

        Bitboard fromBB = square_bb(mv.from);
        int32_t attackerValue;
        if      (fromBB & board.getPAWNS())   attackerValue = PAWN_VALUE;
        else if (fromBB & board.getKNIGHTS()) attackerValue = KNIGHT_VALUE;
        else if (fromBB & board.getBISHOPS()) attackerValue = BISHOP_VALUE;
        else if (fromBB & board.getROOKS())   attackerValue = ROOK_VALUE;
        else if (fromBB & board.getQUEENS())  attackerValue = QUEEN_VALUE;
        else                                    attackerValue = QUEEN_VALUE + PAWN_VALUE;

        return 100000 + victimValue * 100 - attackerValue;
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
