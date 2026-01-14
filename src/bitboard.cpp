#include "types.h"

namespace Ignis
{
    // bu fonksiyon hızlı validation ve magic boards için bitboard lar oluşturur, sadece bir kere çalıştırılır
    void BitBoard::initLookups()
    {

        // ---------------------------------------------------------
        // PAWN ATTACKS (RENKLERE GÖRE AYRI)
        // ---------------------------------------------------------
        // NOT: types.h içinde PawnAttacks[2][64] yaptığını varsayıyorum!
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            // BEYAZ PİYONLAR (Kuzey-Doğu, Kuzey-Batı)
            PawnAttacks[WHITE][sq] = 0;
            Rank r = rank_of(sq);
            File f = file_of(sq);
            
            // Beyaz piyon 8. rankta olamaz ama lookup güvenliği için hesaplasak da olur.
            if (r < RANK_8) 
            {
                if (f < FILE_H) PawnAttacks[WHITE][sq] |= square_bb(sq + NORTH_EAST);
                if (f > FILE_A) PawnAttacks[WHITE][sq] |= square_bb(sq + NORTH_WEST);
            }

            // SİYAH PİYONLAR (Güney-Doğu, Güney-Batı)
            PawnAttacks[BLACK][sq] = 0;
            if (r > RANK_1)
            {
                if (f < FILE_H) PawnAttacks[BLACK][sq] |= square_bb(sq + SOUTH_EAST);
                if (f > FILE_A) PawnAttacks[BLACK][sq] |= square_bb(sq + SOUTH_WEST);
            }
        }

        // ---------------------------------------------------------
        // KNIGHT ATTACKS
        // ---------------------------------------------------------
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            KnightAttacks[sq] = 0;
            
            const int dr[] = { 2, 1, -1, -2, -2, -1, 1, 2 };
            const int df[] = { 1, 2, 2, 1, -1, -2, -2, -1 };

            Rank rank = rank_of(sq);
            File file = file_of(sq);

            for (int i = 0; i < 8; ++i)
            {
                int tempRank = int(rank) + dr[i];
                int tempFile = int(file) + df[i];

                if (is_ok(Rank(tempRank)) && is_ok(File(tempFile)))
                {
                    KnightAttacks[sq] |= square_bb(filerank_square(File(tempFile), Rank(tempRank)));
                }
            }
        }

        // ---------------------------------------------------------
        // BISHOP ATTACKS
        // ---------------------------------------------------------
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            BishopAttacks[sq] = 0;

            Direction dirs[] = { NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };

            for (Direction d : dirs)
            {
                Square tempSq = sq;
                
                while (true)
                {
                    Rank rank = rank_of(tempSq);
                    File file = file_of(tempSq);

                    if ((d == NORTH_EAST || d == NORTH_WEST) && rank == RANK_8) break;
                    if ((d == SOUTH_EAST || d == SOUTH_WEST) && rank == RANK_1) break;
                    if ((d == NORTH_EAST || d == SOUTH_EAST) && file == FILE_H) break;
                    if ((d == NORTH_WEST || d == SOUTH_WEST) && file == FILE_A) break;

                    tempSq += d; 

                    if (!is_ok(tempSq)) break; // Güvenlik?
                    
                    BishopAttacks[sq] |= square_bb(tempSq);
                }
            }
        }

        // ---------------------------------------------------------
        // ROOK ATTACKS
        // ---------------------------------------------------------
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            RookAttacks[sq] = 0;

            Direction dirs[] = { NORTH, SOUTH, EAST, WEST };

            for (Direction d : dirs)
            {
                Square tempSq = sq;

                while (true)
                {
                    Rank rank = rank_of(tempSq);
                    File file = file_of(tempSq);

                    if (d == NORTH && rank == RANK_8) break;
                    if (d == SOUTH && rank == RANK_1) break;
                    if (d == EAST  && file == FILE_H) break;
                    if (d == WEST  && file == FILE_A) break;

                    tempSq += d;

                    if (!is_ok(tempSq)) break;
                    
                    RookAttacks[sq] |= square_bb(tempSq);
                }
            }
        }

        // ---------------------------------------------------------
        // QUEEN ATTACKS
        // ---------------------------------------------------------
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            QueenAttacks[sq] = RookAttacks[sq] | BishopAttacks[sq];
        }

        // ---------------------------------------------------------
        // KING ATTACKS
        // ---------------------------------------------------------
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            KingAttacks[sq] = 0;

            Direction dirs[] = { NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };

            for (Direction d : dirs)
            {
                Rank rank = rank_of(sq);
                File file = file_of(sq);

                bool unsafe = false;
                
                if ((d == NORTH || d == NORTH_EAST || d == NORTH_WEST) && rank == RANK_8) unsafe = true;
                if ((d == SOUTH || d == SOUTH_EAST || d == SOUTH_WEST) && rank == RANK_1) unsafe = true;
                if ((d == EAST  || d == NORTH_EAST || d == SOUTH_EAST) && file == FILE_H) unsafe = true;
                if ((d == WEST  || d == NORTH_WEST || d == SOUTH_WEST) && file == FILE_A) unsafe = true;

                if (!unsafe)
                {
                    Square tempSq = sq + d;
                    if (is_ok(tempSq))
                    {
                        KingAttacks[sq] |= square_bb(tempSq);
                    }
                }
            }
        }
    }
    
    std::optional<MoveType> BitBoard::pawnValidator (const BitMove& move)
    {
        if (!(PawnAttacks[move.from] & square_bb(move.to))) return std::nullopt;

        return MoveType::NORMAL;
    }
    
    std::optional<MoveType> BitBoard::knightValidator (const BitMove& move)
    {
        if (!(KnightAttacks[move.from] & square_bb(move.to))) return std::nullopt;

        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::bishopValidator (const BitMove& move)
    {
        if (!(BishopAttacks[move.from] & square_bb(move.to))) return std::nullopt;

        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::rookValidator (const BitMove& move)
    {
        if (!(RookAttacks[move.from] & square_bb(move.to))) return std::nullopt;

        return MoveType::NORMAL;
    }

    std::optional<MoveType> BitBoard::queenValidator (const BitMove& move)
    {
        if (!(QueenAttacks[move.from] & square_bb(move.to))) return std::nullopt;

        return MoveType::NORMAL;
    }

    // şimdilik rok vs. yok
    std::optional<MoveType> BitBoard::kingValidator (const BitMove& move)
    {
        if (!(KingAttacks[move.from] & square_bb(move.to))) return std::nullopt;

        return MoveType::NORMAL;
    }

    /**
     * friendly fire kontrolü burada yapılacak
     */
    std::optional<MoveType> BitBoard::moveValidator(const BitMove& move)
    {
        // friendly fire control:
        Bitboard targetBit = square_bb(move.to);
        Bitboard myPieces = (side == WHITE) ? WHITES : BLACKS;
        if (myPieces & targetBit) return std::nullopt;

        std::optional<MoveType> moveType;
        if      (targetBit & PAWNS)     moveType = pawnValidator(move);
        else if (targetBit & KNIGHTS)   moveType = knightValidator(move);
        else if (targetBit & BISHOPS)   moveType = bishopValidator(move);
        else if (targetBit & ROOKS)     moveType = rookValidator(move);
        else if (targetBit & QUEENS)    moveType = queenValidator(move);
        else if (targetBit & KINGS)     moveType = kingValidator(move);

        if (!moveType.has_value()) return std::nullopt;
        
    }

    std::optional<MoveType> BitBoard::makeMove(BitMove& move)
    {
    
    }
    
    /**
     * körü körüne hamleyi yapar, kontrol etmez,
     */
    MoveType BitBoard::makeMoveBlind(BitMove& move)
    {
        Bitboard from = square_bb(move.from);
        Bitboard to   = square_bb(move.to);
        Bitboard moveMask = from | to;
        
        if      (to & PAWNS)   PAWNS   ^= to;
        else if (to & KNIGHTS) KNIGHTS ^= to;
        else if (to & BISHOPS) BISHOPS ^= to;
        else if (to & ROOKS)   ROOKS   ^= to;
        else if (to & QUEENS)  QUEENS  ^= to;
        // şah yenemez o yüzden eklenmedi

        if (to & WHITES) WHITES ^= to;
        else if (to & BLACKS) BLACKS ^= to;

        // hareket eden taş yerleştirmesi 
        if      (from & PAWNS)   PAWNS   ^= moveMask;
        else if (from & KNIGHTS) KNIGHTS ^= moveMask;
        else if (from & BISHOPS) BISHOPS ^= moveMask;
        else if (from & ROOKS)   ROOKS   ^= moveMask;
        else if (from & QUEENS)  QUEENS  ^= moveMask;
        else if (from & KINGS)   KINGS   ^= moveMask;

        // renk güncelleme
        if (side == WHITE) WHITES ^= moveMask;
        else               BLACKS ^= moveMask;

        passTurn();

        return MoveType::NORMAL;
    }

    std::vector<BitMove> BitBoard::getValidMoves(void)
    {
    
    }

}