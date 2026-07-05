#include "types.h"
#include "magicboard.h"
#include <iostream>
#include <cmath>

namespace Ignis
{
    // validators
    std::optional<MoveType> BitBoard::pawnValidator(const BitMove& move) const
    {
        Bitboard toBB = square_bb(move.to);
        Direction forward   = (side == WHITE) ? NORTH : SOUTH;
        Rank      startRank = (side == WHITE) ? RANK_2 : RANK_7;
        
        Bitboard  enemy     = (side == WHITE) ? BLACKS : WHITES;
        Bitboard  occupancy = WHITES | BLACKS;

        bool isValid = false;
        MoveType type = MoveType::NORMAL;

        if (move.to == move.from + forward)
        {
            if (!(toBB & occupancy)) isValid = true;
        }
        
        else if (move.to == move.from + forward + forward)
        {
            Square pathSquare = move.from + forward;
            if (rank_of(move.from) == startRank && !(square_bb(pathSquare) & occupancy) && !(toBB & occupancy))
            {
                isValid = true;
            }
        }

        else if (PawnAttacks[side][move.from] & toBB)
        {
            if (toBB & enemy) isValid = true;
            else if (toBB & enpassantTarget)
            {
                isValid = true;
                type = MoveType::EN_PASSANT;
            }
        }

        if (!isValid) return std::nullopt;
        
        Rank r = rank_of(move.to);
        if (r == RANK_1 || r == RANK_8) return MoveType::PROMOTION;
                
        return type;
    }

    std::optional<MoveType> BitBoard::knightValidator(const BitMove& move) const
    {
        if (!(KnightAttacks[move.from] & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::kingValidator(const BitMove& move) const
    {
        if (!(KingAttacks[move.from] & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    // magic uses validators
    std::optional<MoveType> BitBoard::bishopValidator(const BitMove& move) const
    {
        Bitboard occupancy = WHITES | BLACKS;
        Bitboard attacks = getBishopAttacks(move.from, occupancy);
        if (!(attacks & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::rookValidator(const BitMove& move) const
    {
        Bitboard occupancy = WHITES | BLACKS;
        Bitboard attacks = getRookAttacks(move.from, occupancy);
        if (!(attacks & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::queenValidator(const BitMove& move) const
    {
        Bitboard occupancy = WHITES | BLACKS;
        Bitboard attacks = getRookAttacks(move.from, occupancy) | getBishopAttacks(move.from, occupancy);
        if (!(attacks & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::moveValidator(const BitMove& move) const
    {
        Bitboard targetBit = square_bb(move.to);
        Bitboard myPieces = (side == WHITE) ? WHITES : BLACKS;
        if (myPieces & targetBit) return std::nullopt;

        Bitboard movingPiece = square_bb(move.from);
        std::optional<MoveType> moveType;

        if      (movingPiece & PAWNS)       moveType = pawnValidator(move);
        else if (movingPiece & KNIGHTS)     moveType = knightValidator(move);
        else if (movingPiece & BISHOPS)     moveType = bishopValidator(move);
        else if (movingPiece & ROOKS)       moveType = rookValidator(move);
        else if (movingPiece & QUEENS)      moveType = queenValidator(move);
        else if (movingPiece & KINGS)
                                        {
                                            moveType = castlingValidator(move);
                                            if (!moveType.has_value()) moveType = kingValidator(move);
                                        }

        if (!moveType.has_value())          return std::nullopt;
        if (!pinnedControl(move, moveType.value())) return std::nullopt;

        return moveType;
    }

    bool BitBoard::pinnedControl(const BitMove& move, MoveType& moveType) const
    {
        Color movingSide = side;
        BitBoard* self = const_cast<BitBoard*>(this);

        Undo u = self->makeMoveBlind(move, moveType);
        bool inCheck = self->isKingInCheck(movingSide);
        self->unmakeMove(move, u);

        return !inCheck;
    }

    std::optional<MoveType> BitBoard::makeMove(BitMove& move)
    {
        auto typeOpt = moveValidator(move);
        if (!typeOpt.has_value()) return std::nullopt;
        
        MoveType type = typeOpt.value();
        
        if (type == MoveType::PROMOTION && move.promotion == PromotionPiece::None)
        {
            move.promotion = PromotionPiece::Queen;
        }

        makeMoveBlind(move, type);
        return type;
    }
    
    Undo BitBoard::makeMoveBlind(const BitMove& move, MoveType type)
    {
        Undo u;
        u.type          = type;
        u.movedSide     = side;
        u.prevEnpassant = enpassantTarget;
        u.prevWhiteKS   = whiteCastlingKS;
        u.prevWhiteQS   = whiteCastlingQS;
        u.prevBlackKS   = blackCastlingKS;
        u.prevBlackQS   = blackCastlingQS;
        u.prevHash      = hash;

        Bitboard fromBB = square_bb(move.from);
        Bitboard toBB   = square_bb(move.to);
        Bitboard moveMask = fromBB | toBB;

        bool movedWasPawn = (fromBB & PAWNS);
        bool movedWasKing = (fromBB & KINGS);

        if (type != MoveType::EN_PASSANT)
        {
            auto capturedTypeAt = [&](Bitboard bb) -> PieceType
            {
                if (bb & PAWNS)   return PAWN;
                if (bb & KNIGHTS) return KNIGHT;
                if (bb & BISHOPS) return BISHOP;
                if (bb & ROOKS)   return ROOK;
                if (bb & QUEENS)  return QUEEN;
                return NO_PIECE_TYPE;
            };

            if (toBB & WHITES) { u.capturedType = capturedTypeAt(toBB); WHITES ^= toBB; PAWNS &= ~toBB; KNIGHTS &= ~toBB; BISHOPS &= ~toBB; ROOKS &= ~toBB; QUEENS &= ~toBB; }
            if (toBB & BLACKS) { u.capturedType = capturedTypeAt(toBB); BLACKS ^= toBB; PAWNS &= ~toBB; KNIGHTS &= ~toBB; BISHOPS &= ~toBB; ROOKS &= ~toBB; QUEENS &= ~toBB; }
        }
        else
        {
            u.capturedType = PAWN;
        }

        if      (fromBB & PAWNS)   PAWNS   ^= moveMask;
        else if (fromBB & KNIGHTS) KNIGHTS ^= moveMask;
        else if (fromBB & BISHOPS) BISHOPS ^= moveMask;
        else if (fromBB & ROOKS)   ROOKS   ^= moveMask;
        else if (fromBB & QUEENS)  QUEENS  ^= moveMask;
        else if (fromBB & KINGS)   KINGS   ^= moveMask;

        if (side == WHITE) WHITES ^= moveMask;
        else               BLACKS ^= moveMask;

        // castlings
        if (movedWasKing)
        {
            if (side == WHITE) { whiteCastlingKS = false; whiteCastlingQS = false; }
            else                { blackCastlingKS = false; blackCastlingQS = false; }
        }
        if (move.from == SQ_H1 || move.to == SQ_H1) whiteCastlingKS = false;
        if (move.from == SQ_A1 || move.to == SQ_A1) whiteCastlingQS = false;
        if (move.from == SQ_H8 || move.to == SQ_H8) blackCastlingKS = false;
        if (move.from == SQ_A8 || move.to == SQ_A8) blackCastlingQS = false;

        if (type == MoveType::CASTLING)
        {
            Square rookFrom = SQ_A1, rookTo = SQ_A1;
            if (move.to == SQ_G1)      { rookFrom = SQ_H1; rookTo = SQ_F1; }
            else if (move.to == SQ_C1) { rookFrom = SQ_A1; rookTo = SQ_D1; }
            else if (move.to == SQ_G8) { rookFrom = SQ_H8; rookTo = SQ_F8; }
            else if (move.to == SQ_C8) { rookFrom = SQ_A8; rookTo = SQ_D8; }

            Bitboard rookMask = square_bb(rookFrom) | square_bb(rookTo);
            
            ROOKS ^= rookMask;
            if (side == WHITE) WHITES ^= rookMask;
            else               BLACKS ^= rookMask;
        }

        else if (type == MoveType::EN_PASSANT)
        {
            Square captureSq = (side == WHITE) ? (move.to + SOUTH) : (move.to + NORTH);
            Bitboard captureBB = square_bb(captureSq);

            PAWNS &= ~captureBB;
            if (side == WHITE) BLACKS &= ~captureBB;
            else               WHITES &= ~captureBB;
        }

        else if (type == MoveType::PROMOTION)
        {
            PAWNS &= ~toBB;

            switch (move.promotion)
            {
                case PromotionPiece::Queen:  QUEENS  |= toBB; break;
                case PromotionPiece::Rook:   ROOKS   |= toBB; break;
                case PromotionPiece::Bishop: BISHOPS |= toBB; break;
                case PromotionPiece::Knight: KNIGHTS |= toBB; break;
                default:                     QUEENS  |= toBB; break;
            }
        }

        enpassantTarget = 0;
        if (movedWasPawn && (std::abs((int)move.to - (int)move.from) == 16))
        {
            enpassantTarget = square_bb((Square)((move.from + move.to) / 2));
        }

        passTurn();
        hash = computeHashFromScratch();
        history.push_back(hash);
        return u;
    }

    void BitBoard::unmakeMove(const BitMove& move, const Undo& u)
    {
        side            = u.movedSide;
        enpassantTarget = u.prevEnpassant;
        whiteCastlingKS = u.prevWhiteKS;
        whiteCastlingQS = u.prevWhiteQS;
        blackCastlingKS = u.prevBlackKS;
        blackCastlingQS = u.prevBlackQS;
        hash            = u.prevHash;
        history.pop_back();

        Bitboard fromBB   = square_bb(move.from);
        Bitboard toBB     = square_bb(move.to);
        Bitboard moveMask = fromBB | toBB;

        if (u.type == MoveType::PROMOTION)
        {
            QUEENS  &= ~toBB;
            ROOKS   &= ~toBB;
            BISHOPS &= ~toBB;
            KNIGHTS &= ~toBB;
            PAWNS   |= fromBB;
        }
        else
        {
            if      (toBB & PAWNS)   PAWNS   ^= moveMask;
            else if (toBB & KNIGHTS) KNIGHTS ^= moveMask;
            else if (toBB & BISHOPS) BISHOPS ^= moveMask;
            else if (toBB & ROOKS)   ROOKS   ^= moveMask;
            else if (toBB & QUEENS)  QUEENS  ^= moveMask;
            else if (toBB & KINGS)   KINGS   ^= moveMask;
        }

        if (u.movedSide == WHITE) WHITES ^= moveMask;
        else                      BLACKS ^= moveMask;

        if (u.type == MoveType::EN_PASSANT)
        {
            Square captureSq = (u.movedSide == WHITE) ? (move.to + SOUTH) : (move.to + NORTH);
            Bitboard captureBB = square_bb(captureSq);

            PAWNS |= captureBB;
            if (u.movedSide == WHITE) BLACKS |= captureBB;
            else                      WHITES |= captureBB;
        }
        else if (u.capturedType != NO_PIECE_TYPE)
        {
            switch (u.capturedType)
            {
                case PAWN:   PAWNS   |= toBB; break;
                case KNIGHT: KNIGHTS |= toBB; break;
                case BISHOP: BISHOPS |= toBB; break;
                case ROOK:   ROOKS   |= toBB; break;
                case QUEEN:  QUEENS  |= toBB; break;
                default: break;
            }

            if (u.movedSide == WHITE) BLACKS |= toBB;
            else                      WHITES |= toBB;
        }

        if (u.type == MoveType::CASTLING)
        {
            Square rookFrom = SQ_A1, rookTo = SQ_A1;
            if (move.to == SQ_G1)      { rookFrom = SQ_H1; rookTo = SQ_F1; }
            else if (move.to == SQ_C1) { rookFrom = SQ_A1; rookTo = SQ_D1; }
            else if (move.to == SQ_G8) { rookFrom = SQ_H8; rookTo = SQ_F8; }
            else if (move.to == SQ_C8) { rookFrom = SQ_A8; rookTo = SQ_D8; }

            Bitboard rookMask = square_bb(rookFrom) | square_bb(rookTo);

            ROOKS ^= rookMask;
            if (u.movedSide == WHITE) WHITES ^= rookMask;
            else                      BLACKS ^= rookMask;
        }
    }

    bool BitBoard::isRepetition() const
    {
        Bitboard key = getHash();
        int count = 0;
        for (Bitboard h : history) if (h == key) count++;
        return count >= 3;
    }

    NullUndo BitBoard::makeNullMove()
    {
        NullUndo u;
        u.prevEnpassant = enpassantTarget;
        u.prevHash      = hash;

        enpassantTarget = 0;
        passTurn();
        hash = computeHashFromScratch();

        return u;
    }

    void BitBoard::unmakeNullMove(const NullUndo& u)
    {
        passTurn();
        enpassantTarget = u.prevEnpassant;
        hash            = u.prevHash;
    }

    std::vector<BitMove> BitBoard::getValidMoves(Color side) const
    {
        std::vector<BitMove> moves; moves.reserve(40);
        Bitboard pieces = (side == WHITE) ? WHITES : BLACKS;
        const Bitboard myPieces = pieces;
        
        while(pieces)
        {
            Square from = Square(__builtin_ctzll(pieces));
            Bitboard fromBB = square_bb(from);
            Bitboard targets = 0;

            if (fromBB & PAWNS)
            {
                Direction dir = (side == WHITE) ? NORTH : SOUTH;

                BitMove pushMove(from, (Square)(from + dir));
                if (moveValidator(pushMove).has_value())
                {
                    if (rank_of(from + dir) == RANK_1 || rank_of(from + dir) == RANK_8)
                    {
                        moves.push_back(BitMove(from, from + dir, MoveType::PROMOTION, PromotionPiece::Queen));
                        moves.push_back(BitMove(from, from + dir, MoveType::PROMOTION, PromotionPiece::Rook));
                        moves.push_back(BitMove(from, from + dir, MoveType::PROMOTION, PromotionPiece::Bishop));
                        moves.push_back(BitMove(from, from + dir, MoveType::PROMOTION, PromotionPiece::Knight));
                    }
                    else moves.push_back(pushMove);
                }

                BitMove doubleMove(from, (Square)(from + dir + dir));
                if (moveValidator(doubleMove).has_value())
                {
                    moves.push_back(doubleMove);
                }

                targets = PawnAttacks[side][from] & ((WHITES | BLACKS) | enpassantTarget);
            }
            else if (fromBB & KNIGHTS)  targets = KnightAttacks[from];
            else if (fromBB & BISHOPS)  targets = getBishopAttacks(from, WHITES | BLACKS);
            else if (fromBB & ROOKS)    targets = getRookAttacks(from, WHITES | BLACKS);
            else if (fromBB & QUEENS)   targets = getBishopAttacks(from, WHITES | BLACKS) | getRookAttacks(from, WHITES | BLACKS);
            else if (fromBB & KINGS)
            {
                targets = KingAttacks[from];
                
                if (side == WHITE)
                {
                    if (whiteCastlingKS) { BitMove m(SQ_E1, SQ_G1); auto t = moveValidator(m); if(t) { m.type = t.value(); moves.push_back(m); } }
                    if (whiteCastlingQS) { BitMove m(SQ_E1, SQ_C1); auto t = moveValidator(m); if(t) { m.type = t.value(); moves.push_back(m); } }
                }
                else
                {
                    if (blackCastlingKS) { BitMove m(SQ_E8, SQ_G8); auto t = moveValidator(m); if(t) { m.type = t.value(); moves.push_back(m); } }
                    if (blackCastlingQS) { BitMove m(SQ_E8, SQ_C8); auto t = moveValidator(m); if(t) { m.type = t.value(); moves.push_back(m); } }
                }
            }

            targets &= ~myPieces;

            while(targets)
            {
                Square to = Square(__builtin_ctzll(targets));
                BitMove m(from, to);

                auto t = moveValidator(m);
                if (t.has_value())
                {
                    if (t.value() == MoveType::PROMOTION)
                    {
                        moves.push_back(BitMove(from, to, MoveType::PROMOTION, PromotionPiece::Queen));
                        moves.push_back(BitMove(from, to, MoveType::PROMOTION, PromotionPiece::Rook));
                        moves.push_back(BitMove(from, to, MoveType::PROMOTION, PromotionPiece::Bishop));
                        moves.push_back(BitMove(from, to, MoveType::PROMOTION, PromotionPiece::Knight));
                    }
                    else
                    {
                        m.type = t.value();
                        moves.push_back(m);
                    }
                }

                targets &= targets - 1;
            }

            pieces &= pieces -1;
        }

        return moves;
    }

    bool BitBoard::isSquareAttacked(Square sq, Color attacker) const
    {
        Bitboard attackerPawns   = (attacker == WHITE) ? (WHITES & PAWNS)   : (BLACKS & PAWNS);
        Bitboard attackerKnights = (attacker == WHITE) ? (WHITES & KNIGHTS) : (BLACKS & KNIGHTS);
        Bitboard attackerKings   = (attacker == WHITE) ? (WHITES & KINGS)   : (BLACKS & KINGS);
        
        Bitboard attackerRookQueens   = (attacker == WHITE) ? (WHITES & (ROOKS | QUEENS)) : (BLACKS & (ROOKS | QUEENS));
        Bitboard attackerBishopQueens = (attacker == WHITE) ? (WHITES & (BISHOPS | QUEENS)) : (BLACKS & (BISHOPS | QUEENS));

        if (PawnAttacks[(attacker == WHITE) ? BLACK : WHITE][sq] & attackerPawns) return true;
        if (KnightAttacks[sq] & attackerKnights) return true;
        if (KingAttacks[sq] & attackerKings) return true;
        Bitboard occupancy = WHITES | BLACKS;
        if (getRookAttacks(sq, occupancy) & attackerRookQueens) return true;
        if (getBishopAttacks(sq, occupancy) & attackerBishopQueens) return true;

        return false;
    }

    bool BitBoard::isKingInCheck() const
    {
        return isKingInCheck(this->side);
    }

    bool BitBoard::isKingInCheck(Color c) const
    {
        Bitboard kingBit = (c == WHITE) ? (WHITES & KINGS) : (BLACKS & KINGS);
        if (!kingBit) return true;

        Square kingSq = Square(__builtin_ctzll(kingBit));
        return isSquareAttacked(kingSq, (Color)(1 - c));
    }

    Bitboard BitBoard::getHash() const
    {
        return hash;
    }

    Bitboard BitBoard::computeHashFromScratch() const
    {
        Bitboard h = 0;

        auto hashPieces = [&](Bitboard pieces, PieceType pt)
        {
            Bitboard white = pieces & WHITES;
            while (white) { int sq = __builtin_ctzll(white); h ^= ZobristPiece[WHITE][pt][sq]; white &= white - 1; }

            Bitboard black = pieces & BLACKS;
            while (black) { int sq = __builtin_ctzll(black); h ^= ZobristPiece[BLACK][pt][sq]; black &= black - 1; }
        };

        hashPieces(PAWNS,   PAWN);
        hashPieces(KNIGHTS, KNIGHT);
        hashPieces(BISHOPS, BISHOP);
        hashPieces(ROOKS,   ROOK);
        hashPieces(QUEENS,  QUEEN);
        hashPieces(KINGS,   KING);

        if (side == BLACK) h ^= ZobristSide;

        if (whiteCastlingKS) h ^= ZobristCastle[0];
        if (whiteCastlingQS) h ^= ZobristCastle[1];
        if (blackCastlingKS) h ^= ZobristCastle[2];
        if (blackCastlingQS) h ^= ZobristCastle[3];

        if (enpassantTarget)
        {
            int sq = __builtin_ctzll(enpassantTarget);
            h ^= ZobristEnPassant[file_of((Square)sq)];
        }

        return h;
    }

    bool BitBoard::checkClearPath(Square sq1, Square sq2) const
    {
        Bitboard occupancy = WHITES | BLACKS;
        if ((square_bb(sq1) | square_bb(sq2)) & occupancy) return false;
        return true;
    }

    bool BitBoard::checkClearPath(Square sq1, Square sq2, Square sq3) const
    {
        Bitboard occupancy = WHITES | BLACKS;
        if ((square_bb(sq1) | square_bb(sq2) | square_bb(sq3)) & occupancy) return false;
        return true;
    }

    std::optional<MoveType> BitBoard::castlingValidator(const BitMove& move) const
    {
        if (isKingInCheck()) return std::nullopt;

        Color enemy = (side == WHITE) ? BLACK : WHITE;

        if (side == WHITE)
        {
            if (move.from == SQ_E1 && move.to == SQ_G1)
            {
                if (!whiteCastlingKS)                       return std::nullopt;
                if (!checkClearPath(SQ_F1, SQ_G1))          return std::nullopt;
                if (isSquareAttacked(SQ_F1, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
            else if (move.from == SQ_E1 && move.to == SQ_C1)
            {
                if (!whiteCastlingQS)                       return std::nullopt;
                if (!checkClearPath(SQ_D1, SQ_C1, SQ_B1))   return std::nullopt;
                if (isSquareAttacked(SQ_D1, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
        }
        else
        {
            if (move.from == SQ_E8 && move.to == SQ_G8)
            {
                if (!blackCastlingKS)                       return std::nullopt;
                if (!checkClearPath(SQ_F8, SQ_G8))          return std::nullopt;
                if (isSquareAttacked(SQ_F8, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
            else if (move.from == SQ_E8 && move.to == SQ_C8)
            {
                if (!blackCastlingQS)                       return std::nullopt;
                if (!checkClearPath(SQ_D8, SQ_C8, SQ_B8))   return std::nullopt;
                if (isSquareAttacked(SQ_D8, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
        }

        return std::nullopt;
    }
}
