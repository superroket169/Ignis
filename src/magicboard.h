#ifndef MAGICBOARD_H
#define MAGICBOARD_H

#include "types.h"

namespace Ignis
{
    int count_1s(Bitboard b);

    Bitboard getRookMask(Square sq);
    Bitboard getBishopMask(Square sq);

    Bitboard rookAttacksSlow(Square sq, Bitboard block);
    Bitboard bishopAttacksSlow(Square sq, Bitboard block);

    Bitboard set_occupancy(int index, int bits_in_mask, Bitboard attack_mask);
}

#endif