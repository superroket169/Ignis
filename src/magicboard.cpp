#include "magicboard.h"

namespace Ignis
{
    // Brian Kernighan's bit alg
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

    // mask makers
    Bitboard getRookMask(Square sq)
    {
        Bitboard mask = 0;
        Direction dirs[] = { NORTH, SOUTH, EAST, WEST };

        for (Direction d : dirs)
        {
            Square t = sq;
            for(int i=0; i<8; ++i) 
            {
                Rank r = rank_of(t); File f = file_of(t);
                if (d == NORTH && r >= RANK_7) break;
                if (d == SOUTH && r <= RANK_2) break;
                if (d == EAST  && f >= FILE_G) break;
                if (d == WEST  && f <= FILE_B) break;
                t += d; 
                mask |= square_bb(t);
            }
        }
        return mask;
    }

    Bitboard getBishopMask(Square sq)
    {
        Bitboard mask = 0;
        Direction dirs[] = { NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };

        for (Direction d : dirs)
        {
            Square t = sq;
            for(int i=0; i<8; ++i)
            {
                Rank r = rank_of(t); File f = file_of(t);
                if (d == NORTH_EAST && (r >= RANK_7 || f >= FILE_G)) break;
                if (d == NORTH_WEST && (r >= RANK_7 || f <= FILE_B)) break;
                if (d == SOUTH_EAST && (r <= RANK_2 || f >= FILE_G)) break;
                if (d == SOUTH_WEST && (r <= RANK_2 || f <= FILE_B)) break;
                t += d;
                mask |= square_bb(t);
            }
        }
        return mask;
    }

    // attack calculaters
    Bitboard rookAttacksSlow(Square sq, Bitboard block)
    {
        Bitboard attacks = 0;
        Direction dirs[] = { NORTH, SOUTH, EAST, WEST };

        for (Direction d : dirs)
        {
            Square t = sq;
            for(int i=0; i<8; ++i)
            {
                Rank r = rank_of(t); File f = file_of(t);
                if (d == NORTH && r == RANK_8) break;
                if (d == SOUTH && r == RANK_1) break;
                if (d == EAST  && f == FILE_H) break;
                if (d == WEST  && f == FILE_A) break;
                t += d;
                attacks |= square_bb(t);
                if (block & square_bb(t)) break;
            }
        }
        return attacks;
    }

    Bitboard bishopAttacksSlow(Square sq, Bitboard block)
    {
        Bitboard attacks = 0;
        Direction dirs[] = { NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };

        for (Direction d : dirs)
        {
            Square t = sq;
            for(int i=0; i<8; ++i)
            {
                Rank r = rank_of(t); File f = file_of(t);
                if (d == NORTH_EAST && (r == RANK_8 || f == FILE_H)) break;
                if (d == NORTH_WEST && (r == RANK_8 || f == FILE_A)) break;
                if (d == SOUTH_EAST && (r == RANK_1 || f == FILE_H)) break;
                if (d == SOUTH_WEST && (r == RANK_1 || f == FILE_A)) break;
                t += d;
                attacks |= square_bb(t);
                if (block & square_bb(t)) break;
            }
        }
        return attacks;
    }
    
    Bitboard set_occupancy(int index, int bits_in_mask, Bitboard attack_mask)
    {
        Bitboard occupancy = 0ULL;
        for (int i = 0; i < bits_in_mask; i++)
        {
            Square sq = Square(__builtin_ctzll(attack_mask));
            attack_mask &= attack_mask - 1;
            if (index & (1 << i)) occupancy |= square_bb(sq);
        }
        return occupancy;
    }
}