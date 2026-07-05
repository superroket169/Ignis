#include "types.h"
#include <sstream>
#include <vector>
#include <cctype>

namespace Ignis
{
    // helper
    std::vector<std::string> split(const std::string& s, char delimiter)
    {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(s);

        while(std::getline(tokenStream, token, delimiter)) tokens.push_back(token);

        return tokens;
    }

    void BitBoard::loadFEN(const std::string& fen)
    {
        // reset
        PAWNS = KNIGHTS = BISHOPS = ROOKS = QUEENS = KINGS = 0;
        WHITES = BLACKS = 0;
        whiteCastlingKS = whiteCastlingQS = blackCastlingKS = blackCastlingQS = false;
        enpassantTarget = 0;

        std::vector<std::string> parts = split(fen, ' ');

        std::string placement = parts[0];
        int rank = 7; // Rank 8 (Index 7)
        int file = 0; // File A (Index 0)

        for(char c : placement)
        {
            if(c == '/')
            {
                rank--;
                file = 0;
            }
            else if(isdigit(c))
                file += (c - '0');
            else
            {
                Square sq   = (Square)(rank * 8 + file);
                Bitboard bb = (1ULL << sq);

                if(isupper(c)) WHITES |= bb;
                else           BLACKS |= bb;

                char lowerC = tolower(c);
                switch(lowerC)
                {
                    case 'p': PAWNS   |= bb; break;
                    case 'n': KNIGHTS |= bb; break;
                    case 'b': BISHOPS |= bb; break;
                    case 'r': ROOKS   |= bb; break;
                    case 'q': QUEENS  |= bb; break;
                    case 'k': KINGS   |= bb; break;
                }
                file++;
            }
        }

        // Side
        if(parts.size() > 1)
            side = (parts[1] == "w") ? WHITE : BLACK;

        // Castlings
        if(parts.size() > 2)
        {
            std::string castling = parts[2];
            if(castling != "-")
            {
                for (char c : castling)
                {
                    if (c == 'K') whiteCastlingKS = true;
                    if (c == 'Q') whiteCastlingQS = true;
                    if (c == 'k') blackCastlingKS = true;
                    if (c == 'q') blackCastlingQS = true;
                }
            }
        }

        // Enpassant
        if(parts.size() > 3)
        {
            std::string ep = parts[3];
            if(ep != "-")
            {
                File f      = (File)(ep[0] - 'a');
                Rank r      = (Rank)(ep[1] - '1');
                Square sq   = (Square)(r * 8 + f);

                enpassantTarget = (1ULL << sq);
            }
        }

        history.clear();
        hash = computeHashFromScratch();
    }

    std::string BitBoard::getFEN() const
    {
        std::string fen = "";

        for(int rank = 7; rank >= 0; --rank)
        {
            int emptyCount = 0;
            for(int file = 0; file < 8; ++file)
            {
                Square sq = (Square)(rank * 8 + file);
                Bitboard bb = square_bb(sq);

                char pieceChar = 0;

                if(WHITES & bb)
                {
                    if      (PAWNS   & bb) pieceChar = 'P';
                    else if (KNIGHTS & bb) pieceChar = 'N';
                    else if (BISHOPS & bb) pieceChar = 'B';
                    else if (ROOKS   & bb) pieceChar = 'R';
                    else if (QUEENS  & bb) pieceChar = 'Q';
                    else if (KINGS   & bb) pieceChar = 'K';
                }
                else if(BLACKS & bb)
                {
                    if      (PAWNS   & bb) pieceChar = 'p';
                    else if (KNIGHTS & bb) pieceChar = 'n';
                    else if (BISHOPS & bb) pieceChar = 'b';
                    else if (ROOKS   & bb) pieceChar = 'r';
                    else if (QUEENS  & bb) pieceChar = 'q';
                    else if (KINGS   & bb) pieceChar = 'k';
                }

                if(pieceChar != 0)
                {
                    if(emptyCount > 0)
                    {
                        fen += std::to_string(emptyCount);
                        emptyCount = 0;
                    }
                    fen += pieceChar;
                }
                else emptyCount++;
            }
            if(emptyCount > 0) fen += std::to_string(emptyCount);
            if(rank > 0) fen += "/";
        }

        fen += (side == WHITE) ? " w " : " b ";

        std::string castling = "";
        if(whiteCastlingKS) castling += "K";
        if(whiteCastlingQS) castling += "Q";
        if(blackCastlingKS) castling += "k";
        if(blackCastlingQS) castling += "q";

        fen += (castling.empty() ? "-" : castling);
        fen += " ";

        if (enpassantTarget != 0)
        {
            int epSquare = -1;
            for(int i=0; i<64; ++i)
                if(enpassantTarget & (1ULL << i)) { epSquare = i; break; }

            if(epSquare != -1)
            {
                fen += (char)('a' + (epSquare & 7));      // File
                fen += (char)('1' + (epSquare >> 3));     // Rank
            }
            else fen += "-";
        }
        else fen += "-";

        fen += " 0 1";

        return fen;
    }

} // namespace Ignis
