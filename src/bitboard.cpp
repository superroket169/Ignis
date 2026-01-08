#include "types.h"

namespace Ignis
{
    void BitBoard::initLookups()
    {
        // ----- Knigt Attacks ----- 
        for (Square sq = SQ_A1; sq < 64; ++sq)
        {
            KnightAttacks[sq] = 0;

            Rank rank = rank_of(sq);
            File file = file_of(sq);

            // delta rank / delta file combinations
            Rank dr[] = { (Rank)2, (Rank)2, (Rank)1, (Rank)1, (Rank)-1, (Rank)-1, (Rank)-2, (Rank)-2 };
            File df[] = { (File)1, (File)-1, (File)2, (File)-2, (File)2, (File)-2, (File)1, (File)-1 };
            
            for (int i = 0; i < 8; ++i)
            {
                Rank targetR = rank + dr[i];
                File targetF = file + df[i];

                // Eğer tahtanın dışına çıkmıyorsa
                // ekle
                if (is_ok(targetF) && is_ok(targetR))
                {
                    Square targetSq = filerank_square(targetF, targetR);
                    KnightAttacks[sq] |= square_bb((Square)targetSq);
                }
            }
        }

    }
    
    std::optional<MoveType> BitBoard::knightValidator(const BitMove& move)
    {
        // at kontrolü 
        // tek satırda!
        if (!(KnightAttacks[move.from] & square_bb(move.to))) return std::nullopt;
        
        // friendly fire kontrolü ana fonksiyonda bir kere yapılacak "moveValidator"

        // friendly fire control
        // Bitboard targetBit = square__bb(move.to);
        // Bitboard myPieces = (side == WHITE) ? WHITES : BLACKS;
        // if (myPieces & targetBit) return std::nullopt;

        // hamlenin sessiz ve yeme hamlesi olması önemsiz
        // Bitboard enemyPieces = (side == WHITE) ? BLACKS : WHITES;
        // if (enemyPieces & targetBit) return MoveType::NORMAL;

        return MoveType::NORMAL;
    }

    std::optional<MoveType> pawnValidator (const BitMove& move)
    {

    }

    std::optional<MoveType> knightValidator (const BitMove& move)
    {

    }

    /**
     * friendly fire kontrolü burada yapılacak
     */
    std::optional<MoveType> BitBoard::moveValidator(const BitMove& move)
    {
        // friendly fire control:
        Bitboard targetBit = (1ULL << move.to);
        Bitboard myPieces = (side == WHITE) ? WHITES : BLACKS;
        if (myPieces & targetBit) return std::nullopt;

        
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