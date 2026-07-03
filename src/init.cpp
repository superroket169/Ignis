#include "types.h"
#include "magicboard.h"
#include "magicnumbers.h"
#include <iostream>
#include <cmath>

namespace Ignis
{
    uint64_t random_u64()
    {
        static uint64_t seed = 1070372;
        seed ^= seed >> 12;
        seed ^= seed << 25;
        seed ^= seed >> 27;
        return seed * 2685821657736338717ULL;
    }

    uint64_t random_magic_candidate()
    {
        return random_u64() & random_u64(); 
    }

    Bitboard BitBoard::getRookAttacks(Square sq, Bitboard occupancy) const
    {
        Bitboard mask = RookMasks[sq]; 
        occupancy &= mask;
        
        int index = (int)((occupancy * RookMagic[sq]) >> RookShift[sq]);
        
        return RookTable[sq][index];
    }

    Bitboard BitBoard::getBishopAttacks(Square sq, Bitboard occupancy) const
    {
        Bitboard mask = BishopMasks[sq];
        occupancy &= mask;
        int index = (int)((occupancy * BishopMagic[sq]) >> BishopShift[sq]);
        return BishopTable[sq][index];
    }

    // Static somethings
    Bitboard BitBoard::PawnAttacks[2][64];
    Bitboard BitBoard::KnightAttacks[64];
    Bitboard BitBoard::KingAttacks[64];
    Bitboard BitBoard::RookMasks[64];
    Bitboard BitBoard::BishopMasks[64];
    Bitboard BitBoard::RookMagic[64];
    Bitboard BitBoard::BishopMagic[64];

    int BitBoard::RookShift[64];
    int BitBoard::BishopShift[64];

    std::vector<Bitboard> BitBoard::RookTable[64];
    std::vector<Bitboard> BitBoard::BishopTable[64];

    Bitboard BitBoard::ZobristPiece[COLOR_NB][PIECE_TYPE_NB][64];
    Bitboard BitBoard::ZobristSide;
    Bitboard BitBoard::ZobristCastle[4];
    Bitboard BitBoard::ZobristEnPassant[8];

    // sorry about one big funciton
    void BitBoard::initLookups()
    {
        // debug controlü
        static bool isInitialized = false;
        if (isInitialized) return;
        isInitialized = true;

        std::cerr << "Ignis: Generating Magic Numbers..." << "\n";

        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            // PAWN
            PawnAttacks[WHITE][sq] = 0;
            PawnAttacks[BLACK][sq] = 0;

            Rank r = rank_of(sq); File f = file_of(sq);

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

            // KNIGHT
            KnightAttacks[sq] = 0;
            const int n_dr[] = { 2, 1, -1, -2, -2, -1, 1, 2 };
            const int n_df[] = { 1, 2, 2, 1, -1, -2, -2, -1 };
            
            for (int i = 0; i < 8; ++i)
            {
                int tr = int(r) + n_dr[i];
                int tf = int(f) + n_df[i];

                if (tr >= 0 && tr <= 7 && tf >= 0 && tf <= 7)
                    KnightAttacks[sq] |= square_bb(filerank_square(File(tf), Rank(tr)));
            }

            // KING
            KingAttacks[sq] = 0;
            Direction dirs[] = { NORTH, SOUTH, EAST, WEST, NORTH_EAST, NORTH_WEST, SOUTH_EAST, SOUTH_WEST };
            for (Direction d : dirs)
            {
                if (d == NORTH && r == RANK_8) continue;
                if (d == SOUTH && r == RANK_1) continue;
                if (d == EAST  && f == FILE_H) continue;
                if (d == WEST  && f == FILE_A) continue;

                Square t = sq + d;

                if (t >= SQ_A1 && t <= SQ_H8)
                {
                    int r_diff = std::abs(int(rank_of(t)) - int(r));
                    int f_diff = std::abs(int(file_of(t)) - int(f));
                    if(r_diff <= 1 && f_diff <= 1) KingAttacks[sq] |= square_bb(t);
                }
            }
        }

        // MAGIC GENERATION
        for (Square sq = SQ_A1; sq <= SQ_H8; ++sq)
        {
            // ROOK
            {
                Bitboard mask = Ignis::getRookMask(sq);
                RookMasks[sq] = mask;
                int bits = count_1s(mask);

                int numVariations = 1 << bits;
                int magicShift = 64 - bits;
                
                RookTable[sq].resize(numVariations);
                std::vector<Bitboard> occupancies(numVariations);
                std::vector<Bitboard> attacks(numVariations);

                for (int i = 0; i < numVariations; i++)
                {
                    occupancies[i] = set_occupancy(i, bits, mask);
                    attacks[i] = rookAttacksSlow(sq, occupancies[i]);
                }

                bool found = false;

                // bilinen en iyi magic numberları ilk önce dener. altta da öyle
                {
                    uint64_t candidate = RookMagics[sq];
                    std::vector<int> used(numVariations, 0);
                    bool fail = false;
                    for (int i = 0; i < numVariations; i++)
                    {
                        int magicIndex = (int)((occupancies[i] * candidate) >> magicShift);
                        if (used[magicIndex] == 0)
                        {
                            used[magicIndex] = 1;
                            RookTable[sq][magicIndex] = attacks[i];
                        }
                        else if (RookTable[sq][magicIndex] != attacks[i])
                        {
                            fail = true; break;
                        }
                    }
                    if (!fail)
                    {
                        RookMagic[sq] = candidate;
                        RookShift[sq] = magicShift;
                        found = true;
                    }
                }

                for (int k = 0; k < 100000000 && !found; k++)
                {
                    uint64_t candidate = random_magic_candidate();
                    if (count_1s((candidate * mask) & 0xFF00000000000000ULL) < 6) continue;

                    std::vector<int> used(numVariations, 0);

                    bool fail = false;
                    for (int i = 0; i < numVariations; i++)
                    {
                        int magicIndex = (int)((occupancies[i] * candidate) >> magicShift);

                        if (used[magicIndex] == 0)
                        {
                            used[magicIndex] = 1;
                            RookTable[sq][magicIndex] = attacks[i];
                        }
                        else if (RookTable[sq][magicIndex] != attacks[i])
                        {
                            fail = true; break;
                        }
                    }
                    if (!fail)
                    {
                        RookMagic[sq] = candidate;
                        RookShift[sq] = magicShift;
                        found = true;
                    }
                }
                if (!found) { std::cerr << "Magic Fail: Rook " << sq << "\n"; exit(1); }
            }

            // BISHOP
            {
                Bitboard mask = Ignis::getBishopMask(sq);
                BishopMasks[sq] = mask;

                int bits = count_1s(mask);
                int numVariations = 1 << bits;
                int magicShift = 64 - bits;

                BishopTable[sq].resize(numVariations);
                std::vector<Bitboard> occupancies(numVariations);
                std::vector<Bitboard> attacks(numVariations);

                for (int i = 0; i < numVariations; i++)
                {
                    occupancies[i] = set_occupancy(i, bits, mask);
                    attacks[i] = bishopAttacksSlow(sq, occupancies[i]);
                }

                bool found = false;

                {
                    uint64_t candidate = BishopMagics[sq];
                    std::vector<int> used(numVariations, 0);
                    bool fail = false;
                    for (int i = 0; i < numVariations; i++)
                    {
                        int magicIndex = (int)((occupancies[i] * candidate) >> magicShift);
                        if (used[magicIndex] == 0)
                        {
                            used[magicIndex] = 1;
                            BishopTable[sq][magicIndex] = attacks[i];
                        }
                        else if (BishopTable[sq][magicIndex] != attacks[i])
                        {
                            fail = true; break;
                        }
                    }
                    if (!fail)
                    {
                        BishopMagic[sq] = candidate;
                        BishopShift[sq] = magicShift;
                        found = true;
                    }
                }

                for (int k = 0; k < 100000000 && !found; k++)
                {
                    uint64_t candidate = random_magic_candidate();
                    if (count_1s((candidate * mask) & 0xFF00000000000000ULL) < 6) continue;

                    std::vector<int> used(numVariations, 0);

                    bool fail = false;
                    for (int i = 0; i < numVariations; i++)
                    {
                        int magicIndex = (int)((occupancies[i] * candidate) >> magicShift);

                        if (used[magicIndex] == 0)
                        {
                            used[magicIndex] = 1;
                            BishopTable[sq][magicIndex] = attacks[i];
                        }
                        else if (BishopTable[sq][magicIndex] != attacks[i])
                        {
                            fail = true; break;
                        }
                    }
                    if (!fail)
                    {
                        BishopMagic[sq] = candidate;
                        BishopShift[sq] = magicShift;
                        found = true;
                    }
                }
                if (!found) { std::cerr << "Magic Fail: Bishop " << sq << "\n"; exit(1); }
            }
        }

        for (int c = 0; c < COLOR_NB; c++)
            for (int pt = 0; pt < PIECE_TYPE_NB; pt++)
                for (int sq = 0; sq < 64; sq++)
                    ZobristPiece[c][pt][sq] = random_u64();

        ZobristSide = random_u64();
        for (int i = 0; i < 4; i++) ZobristCastle[i] = random_u64();
        for (int i = 0; i < 8; i++) ZobristEnPassant[i] = random_u64();

        std::cerr << "Ignis: Initialization Complete." << "\n";
    }

}
