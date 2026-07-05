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
        resetSearchTables();

        auto moves = board.getValidMoves(mainSide);
        if (moves.empty() || timeMs <= 0) return BitMove();

        BitMove bestMove = moves[0];

        orderMoves(moves, board, 0, false, BitMove());

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

                const BitMove &mv = moves[i];
                Undo u = board.makeMoveBlind(mv, mv.type);

                int32_t score = -search(board, depth - 1, 1, -beta, -alpha);

                board.unmakeMove(mv, u);

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
                       << " nodes " << nodeCount
                       << " time " << (int)(timer.elapsedTime() * 1000.0f)
                       << " pv "
                       << (char)('a' + file_of(bestMove.from)) << (rank_of(bestMove.from) + 1)
                       << (char)('a' + file_of(bestMove.to))   << (rank_of(bestMove.to) + 1);

            if (bestMove.type == MoveType::PROMOTION)
            {
                switch (bestMove.promotion)
                {
                    case PromotionPiece::Queen:  std::cout << 'q'; break;
                    case PromotionPiece::Rook:   std::cout << 'r'; break;
                    case PromotionPiece::Bishop: std::cout << 'b'; break;
                    case PromotionPiece::Knight: std::cout << 'n'; break;
                    default: break;
                }
            }

            std::cout << std::endl;

            if (timer.elapsedTime() * 1000.0f >= (float)timeMs) break;
        }

        return bestMove;
    }

    int32_t Engine::search (BitBoard& board, size_t depth, size_t ply, int32_t alpha, int32_t beta, bool allowNull)
    {
        if (checkTime()) return 0;
        if (board.isRepetition()) return STALEMATE_VALUE;

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
            NullUndo nu = board.makeNullMove();

            int32_t nullScore = -search(board, depth - 1 - R, ply + 1, -beta, -beta + 1, false);

            board.unmakeNullMove(nu);

            if (nullScore >= beta) return beta;
        }

        orderMoves(moves, board, ply, ttHit, entry.bestMove);

        int32_t alphaOrig = alpha;
        int32_t best = -INF;
        BitMove bestMoveHere = moves[0];
        bool firstMove = true;

        for (const auto& mv : moves)
        {
            Undo u = board.makeMoveBlind(mv, mv.type);

            int32_t score;
            if (firstMove)
            {
                score = -search(board, depth - 1, ply + 1, -beta, -alpha); // negamax, tam pencere
                firstMove = false;
            }
            else
            {
                score = -search(board, depth - 1, ply + 1, -alpha - 1, -alpha);
                if (score > alpha && score < beta)
                    score = -search(board, depth - 1, ply + 1, -beta, -alpha);
            }

            board.unmakeMove(mv, u);

            if (score > best) { best = score; bestMoveHere = mv; }
            alpha = std::max(alpha, score);

            if (alpha >= beta)
            {
                recordQuietCutoff(board, mv, ply, depth);
                break;
            }
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
        if (board.isRepetition()) return STALEMATE_VALUE;

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

            Undo u = board.makeMoveBlind(mv, mv.type);
            int32_t score = -quiescence(board, -beta, -alpha);
            board.unmakeMove(mv, u);

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

    PieceType Engine::pieceTypeAt(const BitBoard& board, Square sq) const
    {
        Bitboard sqBB = square_bb(sq);
        if (sqBB & board.getPAWNS())   return PAWN;
        if (sqBB & board.getKNIGHTS()) return KNIGHT;
        if (sqBB & board.getBISHOPS()) return BISHOP;
        if (sqBB & board.getROOKS())   return ROOK;
        if (sqBB & board.getQUEENS())  return QUEEN;
        if (sqBB & board.getKINGS())   return KING;
        return NO_PIECE_TYPE;
    }

    MoveOrderKey Engine::moveOrderTier(const BitBoard& board, const BitMove& mv, size_t ply, bool ttHit, const BitMove& ttMove) const
    {
        if (ttHit && mv == ttMove) return { 4, 0 };

        int32_t mvv = mvvLva(board, mv);
        if (mvv > 0) return { 3, mvv };

        if (ply < MAX_PLY)
        {
            if (mv == killerMoves[ply][0]) return { 2, 1 };
            if (mv == killerMoves[ply][1]) return { 2, 0 };
        }

        if (mv.type == MoveType::PROMOTION) return { 2, 0 };

        PieceType pt = pieceTypeAt(board, mv.from);
        return { 1, historyTable[board.getTurn()][pt][mv.to] };
    }

    void Engine::recordQuietCutoff(const BitBoard& board, const BitMove& mv, size_t ply, size_t depth)
    {
        if (mvvLva(board, mv) > 0) return;        // capture ise killer/history'e katma
        if (mv.type == MoveType::PROMOTION) return;

        if (ply < MAX_PLY && !(mv == killerMoves[ply][0]))
        {
            killerMoves[ply][1] = killerMoves[ply][0];
            killerMoves[ply][0] = mv;
        }

        PieceType pt = pieceTypeAt(board, mv.from);
        int32_t& h = historyTable[board.getTurn()][pt][mv.to];
        h += (int32_t)(depth * depth);
        if (h > HISTORY_MAX) h = HISTORY_MAX;
    }

    void Engine::orderMoves(std::vector<BitMove>& moves, const BitBoard& board, size_t ply, bool ttHit, const BitMove& ttMove) const
    {
        std::vector<std::pair<MoveOrderKey, BitMove>> keyed;
        keyed.reserve(moves.size());

        for (const auto& mv : moves)
            keyed.emplace_back(moveOrderTier(board, mv, ply, ttHit, ttMove), mv);

        std::sort(keyed.begin(), keyed.end(), [](const auto& a, const auto& b) {
            if (a.first.tier != b.first.tier) return a.first.tier > b.first.tier;
            return a.first.score > b.first.score;
        });

        for (size_t i = 0; i < moves.size(); ++i) moves[i] = keyed[i].second;
    }

    void Engine::resetSearchTables()
    {
        for (size_t p = 0; p < MAX_PLY; ++p)
        {
            killerMoves[p][0] = BitMove();
            killerMoves[p][1] = BitMove();
        }

        for (int c = 0; c < COLOR_NB; ++c)
            for (int pt = 0; pt < PIECE_TYPE_NB; ++pt)
                for (int sq = 0; sq < 64; ++sq)
                    historyTable[c][pt][sq] /= 2;
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
        constexpr int32_t PawnMidPST[64] = {
             0,  0,  0,  0,  0,  0,  0,  0,
             5, 10, 10,-20,-20, 10, 10,  5,
             5, -5,-10,  0,  0,-10, -5,  5,
             0,  0,  0, 20, 20,  0,  0,  0,
             5,  5, 10, 25, 25, 10,  5,  5,
            10, 10, 20, 30, 30, 20, 10, 10,
            50, 50, 50, 50, 50, 50, 50, 50,
             0,  0,  0,  0,  0,  0,  0,  0
        };

        constexpr int32_t PawnEndPST[64] = {
              0,   0,   0,   0,   0,   0,   0,   0,
             10,  10,  10,  10,  10,  10,  10,  10,
             20,  20,  20,  20,  20,  20,  20,  20,
             30,  30,  30,  30,  30,  30,  30,  30,
             50,  50,  50,  50,  50,  50,  50,  50,
             80,  80,  80,  80,  80,  80,  80,  80,
            120, 120, 120, 120, 120, 120, 120, 120,
              0,   0,   0,   0,   0,   0,   0,   0
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

        constexpr int32_t KingMidPST[64] = {
             20, 30, 10,  0,  0, 10, 30, 20,
             20, 20,  0,  0,  0,  0, 20, 20,
            -10,-20,-20,-20,-20,-20,-20,-10,
            -20,-30,-30,-40,-40,-30,-30,-20,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30,
            -30,-40,-40,-50,-50,-40,-40,-30
        };

        constexpr int32_t KingEndPST[64] = {
            -50,-30,-30,-30,-30,-30,-30,-50,
            -30,-30,  0,  0,  0,  0,-30,-30,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-10, 30, 40, 40, 30,-10,-30,
            -30,-10, 30, 40, 40, 30,-10,-30,
            -30,-10, 20, 30, 30, 20,-10,-30,
            -30,-20,-10,  0,  0,-10,-20,-30,
            -50,-40,-30,-20,-20,-30,-40,-50
        };

        constexpr int mirrorSq(int sq) { return sq ^ 56; }

        constexpr int32_t PHASE_MAX = 24;
    }

    int32_t Engine::evuPiecesPos(const BitBoard& board)
    {
        int32_t phase = popcount(board.getKNIGHTS()) * 1
                      + popcount(board.getBISHOPS()) * 1
                      + popcount(board.getROOKS())   * 2
                      + popcount(board.getQUEENS())  * 4;
        if (phase > PHASE_MAX) phase = PHASE_MAX;

        int32_t score = 0;

        auto addPST = [&](Bitboard pieces, const int32_t* pst)
        {
            Bitboard white = pieces & board.getWHITES();
            while (white) { int sq = __builtin_ctzll(white); score += pst[sq]; white &= white - 1; }

            Bitboard black = pieces & board.getBLACKS();
            while (black) { int sq = __builtin_ctzll(black); score -= pst[mirrorSq(sq)]; black &= black - 1; }
        };

        auto addTaperedPST = [&](Bitboard pieces, const int32_t* mgPst, const int32_t* egPst)
        {
            auto blended = [&](int sq) { return (mgPst[sq] * phase + egPst[sq] * (PHASE_MAX - phase)) / PHASE_MAX; };

            Bitboard white = pieces & board.getWHITES();
            while (white) { int sq = __builtin_ctzll(white); score += blended(sq); white &= white - 1; }

            Bitboard black = pieces & board.getBLACKS();
            while (black) { int sq = __builtin_ctzll(black); score -= blended(mirrorSq(sq)); black &= black - 1; }
        };

        addTaperedPST(board.getPAWNS(), PawnMidPST, PawnEndPST);
        addPST(board.getKNIGHTS(), KnightPST);
        addPST(board.getBISHOPS(), BishopPST);
        addPST(board.getROOKS(),   RookPST);
        addPST(board.getQUEENS(),  QueenPST);
        addTaperedPST(board.getKINGS(), KingMidPST, KingEndPST);

        return score;
    }

    // helper :
    inline int popcount(uint64_t bb) { return __builtin_popcountll(bb); }
}
