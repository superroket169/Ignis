#include "types.h"
#include "magicboard.h"
#include "magicnumbers.h" 
#include <iostream>
#include <cmath>

namespace Ignis
{
    Bitboard BitBoard::getRookAttacks(Square sq, Bitboard occupancy)
    {
        Bitboard mask = RookMasks[sq]; 
        occupancy &= mask;
        int index = (int)((occupancy * RookMagic[sq]) >> RookShift[sq]);
        return RookTable[sq][index];
    }

    Bitboard BitBoard::getBishopAttacks(Square sq, Bitboard occupancy)
    {
        Bitboard mask = BishopMasks[sq];
        occupancy &= mask;
        int index = (int)((occupancy * BishopMagic[sq]) >> BishopShift[sq]);
        return BishopTable[sq][index];
    }

    // tablolar
    void BitBoard::initLookups()
    {
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            PawnAttacks[WHITE][sq] = 0; PawnAttacks[BLACK][sq] = 0;
            Rank r = rank_of(sq); File f = file_of(sq);

            // Pawns
            if (r < RANK_8)
            {
                if (f < FILE_H) PawnAttacks[WHITE][sq] |= square_bb(sq + NORTH_EAST);
                if (f > FILE_A) PawnAttacks[WHITE][sq] |= square_bb(sq + NORTH_WEST);
            }
            if (r > RANK_1)
            {
                if (f < FILE_H) PawnAttacks[BLACK][sq] |= square_bb(sq + SOUTH_EAST);
                if (f > FILE_A) PawnAttacks[BLACK][sq] |= square_bb(sq + SOUTH_WEST);
            }

            // Knights
            KnightAttacks[sq] = 0;
            const int dr[] = { 2, 1, -1, -2, -2, -1, 1, 2 };
            const int df[] = { 1, 2, 2, 1, -1, -2, -2, -1 };
            for (int i = 0; i < 8; ++i)
            {
                int tr = int(r) + dr[i]; int tf = int(f) + df[i];
                if (is_ok(Rank(tr)) && is_ok(File(tf)))
                    KnightAttacks[sq] |= square_bb(filerank_square(File(tf), Rank(tr)));
            }

            // King
            KingAttacks[sq] = 0;
            Direction dirs[] = { NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };
            for (Direction d : dirs)
            {
                Rank tr = rank_of(sq); File tf = file_of(sq);

                if (d == NORTH && tr == RANK_8) continue;
                if (d == SOUTH && tr == RANK_1) continue;
                if (d == EAST  && tf == FILE_H) continue;
                if (d == WEST  && tf == FILE_A) continue;

                Square t = sq + d; 
                if (t >= SQ_A1 && t <= SQ_H8) 
                {
                    int r_diff = std::abs(int(rank_of(t)) - int(r));
                    int f_diff = std::abs(int(file_of(t)) - int(f));
                    if(r_diff <= 1 && f_diff <= 1) KingAttacks[sq] |= square_bb(t);
                }
            }
        }

        // magic somethings
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            // Rook
            RookMasks[sq] = getRookMask(sq);
            int rookBits  = count_1s(RookMasks[sq]);
            RookShift[sq] = 64 - rookBits;
            RookMagic[sq] = RookMagics[sq]; 

            int size = (1 << rookBits);
            RookTable[sq].resize(size);

            for (int i = 0; i < size; i++)
            {
                Bitboard occupancy = set_occupancy(i, rookBits, RookMasks[sq]);
                int magicIndex = (int)((occupancy * RookMagic[sq]) >> RookShift[sq]);
                RookTable[sq][magicIndex] = rookAttacksSlow(sq, occupancy);
            }

            // Bishop
            BishopMasks[sq] = getBishopMask(sq);
            int bishopBits  = count_1s(BishopMasks[sq]);
            BishopShift[sq] = 64 - bishopBits;
            BishopMagic[sq] = BishopMagics[sq];

            size = (1 << bishopBits);
            BishopTable[sq].resize(size);

            for (int i = 0; i < size; i++)
            {
                Bitboard occupancy = set_occupancy(i, bishopBits, BishopMasks[sq]);
                int magicIndex = (int)((occupancy * BishopMagic[sq]) >> BishopShift[sq]);
                BishopTable[sq][magicIndex] = bishopAttacksSlow(sq, occupancy);
            }
        }
    }

    // validators
    std::optional<MoveType> BitBoard::pawnValidator(const BitMove& move)
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
        
        // double push        
        else if (move.to == move.from + forward + forward)
        {
            Square pathSquare = move.from + forward;
            if (rank_of(move.from) == startRank && !(square_bb(pathSquare) & occupancy) && !(toBB & occupancy))
            {
                isValid = true;
            }
        }

        // çapraz şeyler
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

        int toIndex = (int)move.to; 
        
        if (toIndex >= 56 || toIndex <= 7)
        {
            return MoveType::PROMOTION;
        }

        return type;
    }

    std::optional<MoveType> BitBoard::knightValidator(const BitMove& move)
    {
        if (!(KnightAttacks[move.from] & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::kingValidator(const BitMove& move)
    {
        if (!(KingAttacks[move.from] & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    // magic uses validators
    std::optional<MoveType> BitBoard::bishopValidator(const BitMove& move)
    {
        Bitboard occupancy = WHITES | BLACKS;
        Bitboard attacks = getBishopAttacks(move.from, occupancy);
        if (!(attacks & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::rookValidator(const BitMove& move)
    {
        Bitboard occupancy = WHITES | BLACKS;
        Bitboard attacks = getRookAttacks(move.from, occupancy);
        if (!(attacks & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::queenValidator(const BitMove& move)
    {
        Bitboard occupancy = WHITES | BLACKS;
        Bitboard attacks = getRookAttacks(move.from, occupancy) | getBishopAttacks(move.from, occupancy);
        if (!(attacks & square_bb(move.to))) return std::nullopt;
        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::moveValidator(const BitMove& move)
    {
        // Friendly Fire
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

        if (!moveType.has_value()) return std::nullopt;

        return moveType;
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
    
    MoveType BitBoard::makeMoveBlind(BitMove& move, MoveType type)
    {
        Bitboard fromBB = square_bb(move.from);
        Bitboard toBB   = square_bb(move.to);
        Bitboard moveMask = fromBB | toBB;
        
        if (type != MoveType::EN_PASSANT)
        {
            if (toBB & WHITES) { WHITES ^= toBB; PAWNS &= ~toBB; KNIGHTS &= ~toBB; BISHOPS &= ~toBB; ROOKS &= ~toBB; QUEENS &= ~toBB; }
            if (toBB & BLACKS) { BLACKS ^= toBB; PAWNS &= ~toBB; KNIGHTS &= ~toBB; BISHOPS &= ~toBB; ROOKS &= ~toBB; QUEENS &= ~toBB; }
        }

        if      (fromBB & PAWNS)   PAWNS   ^= moveMask;
        else if (fromBB & KNIGHTS) KNIGHTS ^= moveMask;
        else if (fromBB & BISHOPS) BISHOPS ^= moveMask;
        else if (fromBB & ROOKS)   ROOKS   ^= moveMask;
        else if (fromBB & QUEENS)  QUEENS  ^= moveMask;
        else if (fromBB & KINGS)   KINGS   ^= moveMask;

        if (side == WHITE) WHITES ^= moveMask;
        else               BLACKS ^= moveMask;


        if (type == MoveType::CASTLING)
        {
            Square rookFrom, rookTo;
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

        // promotipn
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
        if ((fromBB & PAWNS) && (std::abs((int)move.to - (int)move.from) == 16))
        {
             enpassantTarget = square_bb((Square)((move.from + move.to) / 2));
        }

        passTurn();
        return type;
    }

    std::vector<BitMove> BitBoard::getValidMoves(void)
    {
        std::vector<BitMove> moves;
        // WIP
        return moves;
    }

    bool BitBoard::isSquareAttacked(Square sq, Color attacker)
    {
        Bitboard attackerPawns   = (attacker == WHITE) ? (WHITES & PAWNS)   : (BLACKS & PAWNS);
        Bitboard attackerKnights = (attacker == WHITE) ? (WHITES & KNIGHTS) : (BLACKS & KNIGHTS);
        Bitboard attackerKings   = (attacker == WHITE) ? (WHITES & KINGS)   : (BLACKS & KINGS);
        
        Bitboard attackerRookQueens   = (attacker == WHITE) ? (WHITES & (ROOKS | QUEENS)) : (BLACKS & (ROOKS | QUEENS));
        Bitboard attackerBishopQueens = (attacker == WHITE) ? (WHITES & (BISHOPS | QUEENS)) : (BLACKS & (BISHOPS | QUEENS));

        if (PawnAttacks[1 - attacker][sq] & attackerPawns) return true;

        // knight
        if (KnightAttacks[sq] & attackerKnights) return true;

        // king
        if (KingAttacks[sq] & attackerKings) return true;

        // rook & bishop & queen
        Bitboard occupancy = WHITES | BLACKS;
        if (getRookAttacks(sq, occupancy) & attackerRookQueens) return true;
        if (getBishopAttacks(sq, occupancy) & attackerBishopQueens) return true;

        return false;
    }

    bool BitBoard::isKingInCheck(void)
    {
        Bitboard kingBit = (side == WHITE) ? (WHITES & KINGS) : (BLACKS & KINGS);
        Square kingSq = Square(__builtin_ctzll(kingBit));

        return isSquareAttacked(kingSq, (Color)(1 - side));
    }

    // castling helperları. daha temiz bir rok olsun die
    bool BitBoard::checkClearPath(Square sq1, Square sq2)
    {
        Bitboard occupancy = WHITES | BLACKS;
        return !((occupancy & square_bb(sq1)) | (occupancy & square_bb(sq2)));
    }

    bool BitBoard::checkClearPath(Square sq1, Square sq2, Square sq3)
    {
        Bitboard occupancy = WHITES | BLACKS;
        return !((occupancy & square_bb(sq1)) | (occupancy & square_bb(sq2)) | (occupancy & square_bb(sq3)));
    }

    std::optional<MoveType> BitBoard::castlingValidator(const BitMove& move)
    {
        if (isKingInCheck()) return std::nullopt;

        Color enemy = (side == WHITE) ? BLACK : WHITE;

        // white
        if (side == WHITE)
        {
            // king side castling
            if (move.from == SQ_E1 && move.to == SQ_G1)
            {
                if (!whiteCastlingKS)                       return std::nullopt;
                if (!checkClearPath(SQ_F1, SQ_G1))          return std::nullopt;
                if (isSquareAttacked(SQ_F1, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
            // queenside castling
            else if (move.from == SQ_E1 && move.to == SQ_C1)
            {
                if (!whiteCastlingQS)                       return std::nullopt;
                if (!checkClearPath(SQ_D1, SQ_C1, SQ_B1))   return std::nullopt;
                if (isSquareAttacked(SQ_D1, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
        }
        // black
        else
        {
            // kingside castling
            if (move.from == SQ_E8 && move.to == SQ_G8)
            {
                if (!blackCastlingKS)                       return std::nullopt;
                if (!checkClearPath(SQ_F8, SQ_G8))          return std::nullopt;
                if (isSquareAttacked(SQ_F8, enemy))         return std::nullopt;

                return MoveType::CASTLING;
            }
            // queenside castling
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