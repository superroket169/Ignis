#include "search.h"
#include "time.h"
#include<iostream>
#include<algorithm>

namespace Ignis
{
    bool Engine::checkTime()
    {
        if (timeUp) return true;

        nodeCount++;
        if ((nodeCount & 2047) == 0 && timer.elapsedTime() * 1000.0f >= (float)timeBudgetMs)
            timeUp = true;

        return timeUp;
    }

    BitMove Engine::getBestMove(BitBoard& board, size_t maxDepth, int timeMs)
    {
        mainSide = board.getTurn();
        auto moves = board.getValidMoves(mainSide);
        if (moves.empty() || timeMs <= 0) return BitMove();

        BitMove bestMove = moves[0];

        std::sort(moves.begin(), moves.end(), [&](const BitMove& a, const BitMove& b) {
            return mvvLva(board, a) > mvvLva(board, b);
        });

        timer.start();
        timeBudgetMs = timeMs;
        timeUp = false;
        nodeCount = 0;

        for (size_t depth = 1; depth <= maxDepth; ++depth)
        {
            int32_t alpha = -INF;
            int32_t beta  =  INF;
            int32_t localBestScore = -INF;
            BitMove localBestMove = moves[0];
            bool depthCompleted = true;

            for (size_t i = 0; i < moves.size(); ++i)
            {
                if (timer.elapsedTime() * 1000.0f >= (float)timeMs) { depthCompleted = false; break; }

                BitBoard tmp = board;
                const BitMove &mv = moves[i];
                tmp.makeMoveBlind(mv, mv.type);

                int32_t score = -search(tmp, depth - 1, -beta, -alpha);

                if (timeUp) { depthCompleted = false; break; }

                if (score > localBestScore)
                {
                    localBestScore = score;
                    localBestMove = mv;
                }

                alpha = std::max(alpha, score);
                if (alpha >= beta) break;
            }

            if (!depthCompleted) break;

            bestMove = localBestMove;

            // uci standart info
            std::cout << "info depth " << depth
                       << " score cp " << localBestScore
                       << " time " << (int)(timer.elapsedTime() * 1000.0f)
                       << " pv "
                       << (char)('a' + file_of(bestMove.from)) << (rank_of(bestMove.from) + 1)
                       << (char)('a' + file_of(bestMove.to))   << (rank_of(bestMove.to) + 1)
                       << std::endl;

            if (timer.elapsedTime() * 1000.0f >= (float)timeMs) break;
        }

        return bestMove;
    }

    int32_t Engine::search (BitBoard& board, size_t depth, int32_t alpha, int32_t beta, bool allowNull)
    {
        if (checkTime()) return 0;

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

        Bitboard key = board.getHash();
        TTEntry& entry = tt[key & TT_MASK];
        bool ttHit = (entry.depth >= 0 && entry.key == key);

        if (ttHit && (size_t)entry.depth >= depth)
        {
            if (entry.flag == TT_EXACT) return entry.score;
            if (entry.flag == TT_LOWERBOUND && entry.score >= beta)  return entry.score;
            if (entry.flag == TT_UPPERBOUND && entry.score <= alpha) return entry.score;
        }

        // nmp

        Bitboard mySide       = (board.getTurn() == WHITE) ? board.getWHITES() : board.getBLACKS();
        Bitboard nonPawnKing  = board.getKNIGHTS() | board.getBISHOPS() | board.getROOKS() | board.getQUEENS();
        bool hasNonPawnMaterial = (mySide & nonPawnKing) != 0;

        if (allowNull && depth >= 3 && !board.isKingInCheck(board.getTurn()) && hasNonPawnMaterial)
        {
            const size_t R = 2; // null-move indirgeme miktari
            BitBoard nullBoard = board;
            nullBoard.makeNullMove();

            int32_t nullScore = -search(nullBoard, depth - 1 - R, -beta, -beta + 1, false);
            if (nullScore >= beta) return beta;
        }

        std::sort(moves.begin(), moves.end(), [&](const BitMove& a, const BitMove& b) {
            if (ttHit && a == entry.bestMove) return true;
            if (ttHit && b == entry.bestMove) return false;
            return mvvLva(board, a) > mvvLva(board, b);
        });

        int32_t alphaOrig = alpha;
        int32_t best = -INF;
        BitMove bestMoveHere = moves[0];

        for (const auto& mv : moves)
        {
            BitBoard tmp = board;
            tmp.makeMoveBlind(mv, mv.type);

            int32_t score = -search(tmp, depth - 1, -beta, -alpha); // negamax

            if (score > best) { best = score; bestMoveHere = mv; }
            alpha = std::max(alpha, score);

            if (alpha >= beta) break;
        }

        TTFlag flag = TT_EXACT;
        if      (best <= alphaOrig) flag = TT_UPPERBOUND;
        else if (best >= beta)      flag = TT_LOWERBOUND;

        entry.key      = key;
        entry.score    = best;
        entry.depth    = (int32_t)depth;
        entry.flag     = flag;
        entry.bestMove = bestMoveHere;

        return best;
    }

    int32_t Engine::quiescence(BitBoard& board, int32_t alpha, int32_t beta)
    {
        if (checkTime()) return 0;

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
        return evuPiecesRaw(board) + evuPiecesPos(board);
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

    // piece-square tablolari (PST)
    namespace
    {
        constexpr int32_t PawnPST[64] = {
             0,  0,  0,  0,  0,  0,  0,  0,
             5, 10, 10,-20,-20, 10, 10,  5,
             5, -5,-10,  0,  0,-10, -5,  5,
             0,  0,  0, 20, 20,  0,  0,  0,
             5,  5, 10, 25, 25, 10,  5,  5,
            10, 10, 20, 30, 30, 20, 10, 10,
            50, 50, 50, 50, 50, 50, 50, 50,
             0,  0,  0,  0,  0,  0,  0,  0
        };

        constexpr int32_t KnightPST[64] = {
            -50,-40,-30,-30,-30,-30,-40,-50,
            -40,-20,  0,  5,  5,  0,-20,-40,
            -30,  5, 10, 15, 15, 10,  5,-30,
            -30,  0, 15, 20, 20, 15,  0,-30,
            -30,  5, 15, 20, 20, 15,  5,-30,
            -30,  0, 10, 15, 15, 10,  0,-30,
            -40,-20,  0,  0,  0,  0,-20,-40,
            -50,-40,-30,-30,-30,-30,-40,-50
        };

        constexpr int32_t BishopPST[64] = {
            -20,-10,-10,-10,-10,-10,-10,-20,
            -10,  5,  0,  0,  0,  0,  5,-10,
            -10, 10, 10, 10, 10, 10, 10,-10,
            -10,  0, 10, 10, 10, 10,  0,-10,
            -10,  5,  5, 10, 10,  5,  5,-10,
            -10,  0,  5, 10, 10,  5,  0,-10,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -20,-10,-10,-10,-10,-10,-10,-20
        };

        constexpr int32_t RookPST[64] = {
             0,  0,  0,  5,  5,  0,  0,  0,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
            -5,  0,  0,  0,  0,  0,  0, -5,
             5, 10, 10, 10, 10, 10, 10,  5,
             0,  0,  0,  0,  0,  0,  0,  0
        };

        constexpr int32_t QueenPST[64] = {
            -20,-10,-10, -5, -5,-10,-10,-20,
            -10,  0,  5,  0,  0,  0,  0,-10,
            -10,  5,  5,  5,  5,  5,  0,-10,
              0,  0,  5,  5,  5,  5,  0, -5,
             -5,  0,  5,  5,  5,  5,  0, -5,
            -10,  0,  5,  5,  5,  5,  0,-10,
            -10,  0,  0,  0,  0,  0,  0,-10,
            -20,-10,-10, -5, -5,-10,-10,-20
        };

        constexpr int32_t KingPST[64] = {
             20, 30, 10,  0,  0, 10, 30, 20,
             20, 20,  0,  0,  0,  0, 20, 20,
            -10,-20,-20,-20,-20,-20,-20,-10,
            -20,-30,-30,-40,-40,-30,-30,-20,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30
        };

        constexpr int mirrorSq(int sq) { return sq ^ 56; }
    }

    int32_t Engine::evuPiecesPos(const BitBoard& board)
    {
        int32_t score = 0;

        auto addPST = [&](Bitboard pieces, const int32_t* pst)
        {
            Bitboard white = pieces & board.getWHITES();
            while (white) { int sq = __builtin_ctzll(white); score += pst[sq]; white &= white - 1; }

            Bitboard black = pieces & board.getBLACKS();
            while (black) { int sq = __builtin_ctzll(black); score -= pst[mirrorSq(sq)]; black &= black - 1; }
        };

        addPST(board.getPAWNS(),   PawnPST);
        addPST(board.getKNIGHTS(), KnightPST);
        addPST(board.getBISHOPS(), BishopPST);
        addPST(board.getROOKS(),   RookPST);
        addPST(board.getQUEENS(),  QueenPST);
        addPST(board.getKINGS(),   KingPST);

        return score;
    }

    // helper :
    inline int popcount(uint64_t bb) { return __builtin_popcountll(bb); }
}
