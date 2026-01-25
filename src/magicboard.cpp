#include "magicboard.h"
#include <vector>

namespace Ignis
{
    int count_1s(Bitboard b)
    {
        int r = 0;
        while (b)
        {
            r++;
            b &= b - 1;
        }
        return r;
    }

    Bitboard getRookMask(Square sq)
    {
        Bitboard mask = 0;
        Rank r = rank_of(sq);
        File f = file_of(sq);

        // N
        for (int tr = r + 1; tr < RANK_8; tr++) mask |= square_bb(filerank_square(f, Rank(tr)));
        // S
        for (int tr = r - 1; tr > RANK_1; tr--) mask |= square_bb(filerank_square(f, Rank(tr)));
        // E
        for (int tf = f + 1; tf < FILE_H; tf++) mask |= square_bb(filerank_square(File(tf), r));
        // W
        for (int tf = f - 1; tf > FILE_A; tf--) mask |= square_bb(filerank_square(File(tf), r));

        return mask;
    }

    Bitboard getBishopMask(Square sq)
    {
        Bitboard mask = 0;
        Rank r = rank_of(sq);
        File f = file_of(sq);

        // NE
        for (int tr = r+1, tf = f+1; tr < RANK_8 && tf < FILE_H; tr++, tf++) mask |= square_bb(filerank_square(File(tf), Rank(tr)));
        // SE
        for (int tr = r-1, tf = f+1; tr > RANK_1 && tf < FILE_H; tr--, tf++) mask |= square_bb(filerank_square(File(tf), Rank(tr)));
        // SW
        for (int tr = r-1, tf = f-1; tr > RANK_1 && tf > FILE_A; tr--, tf--) mask |= square_bb(filerank_square(File(tf), Rank(tr)));
        // NW
        for (int tr = r+1, tf = f-1; tr < RANK_8 && tf > FILE_A; tr++, tf--) mask |= square_bb(filerank_square(File(tf), Rank(tr)));

        return mask;
    }

    Bitboard rookAttacksSlow(Square sq, Bitboard block)
    {
        Bitboard attacks = 0;
        Rank r = rank_of(sq);
        File f = file_of(sq);

        // N
        for (int tr = r + 1; tr <= RANK_8; tr++)
        {
            Bitboard b = square_bb(filerank_square(f, Rank(tr)));
            attacks |= b;
            if (block & b) break;
        }
        // S
        for (int tr = r - 1; tr >= RANK_1; tr--)
        {
            Bitboard b = square_bb(filerank_square(f, Rank(tr)));
            attacks |= b;
            if (block & b) break;
        }
        // E
        for (int tf = f + 1; tf <= FILE_H; tf++)
        {
            Bitboard b = square_bb(filerank_square(File(tf), r));
            attacks |= b;
            if (block & b) break;
        }
        // W
        for (int tf = f - 1; tf >= FILE_A; tf--)
        {
            Bitboard b = square_bb(filerank_square(File(tf), r));
            attacks |= b;
            if (block & b) break;
        }
        return attacks;
    }

    Bitboard bishopAttacksSlow(Square sq, Bitboard block)
    {
        Bitboard attacks = 0;
        Rank r = rank_of(sq);
        File f = file_of(sq);

        // NE
        for (int tr = r+1, tf = f+1; tr <= RANK_8 && tf <= FILE_H; tr++, tf++)
        {
            Bitboard b = square_bb(filerank_square(File(tf), Rank(tr)));
            attacks |= b;
            if (block & b) break;
        }
        // SE
        for (int tr = r-1, tf = f+1; tr >= RANK_1 && tf <= FILE_H; tr--, tf++)
        {
            Bitboard b = square_bb(filerank_square(File(tf), Rank(tr)));
            attacks |= b;
            if (block & b) break;
        }
        // SW
        for (int tr = r-1, tf = f-1; tr >= RANK_1 && tf >= FILE_A; tr--, tf--)
        {
            Bitboard b = square_bb(filerank_square(File(tf), Rank(tr)));
            attacks |= b;
            if (block & b) break;
        }
        // NW
        for (int tr = r+1, tf = f-1; tr <= RANK_8 && tf >= FILE_A; tr++, tf--)
        {
            Bitboard b = square_bb(filerank_square(File(tf), Rank(tr)));
            attacks |= b;
            if (block & b) break;
        }
        return attacks;
    }
    
    Bitboard set_occupancy(int index, int bits_in_mask, Bitboard attack_mask)
    {
        Bitboard occupancy = 0ULL;
        for (int i = 0; i < bits_in_mask; i++)
        {
            int square = -1;
            if (attack_mask != 0)
            {
                Bitboard lsb = attack_mask & -attack_mask;
                
                for (int k = 0; k < 64; k++)
                    if((lsb >> k) & 1) { square = k; break; }
                
                attack_mask &= ~lsb;
            }
            
            if (square != -1)
                if (index & (1 << i))
                    occupancy |= (1ULL << square);
        }
        return occupancy;
    }
}