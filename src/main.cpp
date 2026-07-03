#include "types.h"
#include "search.h"
#include "time.h"
#include <iostream>
#include <sstream>
#include <string>
#include <cctype>

using namespace Ignis;

char pieceChar(const BitBoard& board, Square sq)
{
    Bitboard bit = square_bb(sq);
    bool isWhite = board.getWHITES() & bit;
    bool isBlack = board.getBLACKS() & bit;
    if (!isWhite && !isBlack) return '.';

    char c;
    if      (board.getPAWNS()   & bit) c = 'p';
    else if (board.getKNIGHTS() & bit) c = 'n';
    else if (board.getBISHOPS() & bit) c = 'b';
    else if (board.getROOKS()   & bit) c = 'r';
    else if (board.getQUEENS()  & bit) c = 'q';
    else                                c = 'k';

    return isWhite ? (char)std::toupper(c) : c;
}

void printBoard(const BitBoard& board)
{
    std::cout << "\n";
    for (int r = 7; r >= 0; r--)
    {
        std::cout << (r + 1) << "  ";
        for (int f = 0; f < 8; f++)
        {
            Square sq = filerank_square(File(f), Rank(r));
            std::cout << pieceChar(board, sq) << " ";
        }
        std::cout << "\n";
    }
    std::cout << "   a b c d e f g h\n\n";
}

bool parseSquare(const std::string& s, Square& out)
{
    if (s.size() != 2) return false;
    char fc = (char)std::tolower(s[0]);
    char rc = s[1];
    if (fc < 'a' || fc > 'h' || rc < '1' || rc > '8') return false;
    out = filerank_square(File(fc - 'a'), Rank(rc - '1'));
    return true;
}

std::string squareStr(Square sq)
{
    std::string s;
    s += (char)('a' + file_of(sq));
    s += (char)('1' + rank_of(sq));
    return s;
}

int main()
{
    BitBoard board;
    Engine engine;

    const size_t SEARCH_DEPTH = 7;
    const int    TIME_BUDGET  = 3600;

    std::cout << "Ignis - basit terminal satranc\n";
    std::cout << "Beyaz: sen (buyuk harfler) | Siyah: motor (kucuk harfler)\n";
    std::cout << "Hamle formati: 'e2 e4'  terfi icin: 'e7 e8 q'\n";

    while (true)
    {
        printBoard(board);

        Color turn = board.getTurn();
        std::cout << (turn == WHITE ? "Beyaz" : "Siyah") << " oynuyor";
        if (board.isKingInCheck()) std::cout << "  [SAH!]";
        std::cout << "\n";

        auto moves = board.getValidMoves(turn);
        if (moves.empty())
        {
            if (board.isKingInCheck())
                std::cout << (turn == WHITE ? "Siyah" : "Beyaz") << " kazandi (mat).\n";
            else
                std::cout << "Pat - beraberlik.\n";
            break;
        }

        if (turn == WHITE)
        {
            std::cout << "Hamlen: ";
            std::string line;
            if (!std::getline(std::cin, line)) break;

            std::istringstream iss(line);
            std::string fromStr, toStr, promoStr;
            iss >> fromStr >> toStr;

            Square from, to;
            if (!parseSquare(fromStr, from) || !parseSquare(toStr, to))
            {
                std::cout << "Gecersiz kare formati, tekrar dene.\n";
                continue;
            }

            PromotionPiece promo = PromotionPiece::None;
            if (iss >> promoStr && !promoStr.empty())
            {
                switch (std::tolower(promoStr[0]))
                {
                    case 'q': promo = PromotionPiece::Queen;  break;
                    case 'r': promo = PromotionPiece::Rook;   break;
                    case 'b': promo = PromotionPiece::Bishop; break;
                    case 'n': promo = PromotionPiece::Knight; break;
                }
            }

            BitMove mv(from, to, MoveType::NORMAL, promo);
            auto result = board.makeMove(mv);
            if (!result.has_value())
            {
                std::cout << "Gecersiz hamle!\n";
                continue;
            }
        }
        else
        {
            std::cout << "Motor dusunuyor (max depth " << SEARCH_DEPTH << ")...\n";
            Time timer; timer.start();
            BitMove best = engine.getBestMove(board, SEARCH_DEPTH, TIME_BUDGET);
            float elapsed = timer.elapsedTime();

            std::cout << "Motor oynadi: " << squareStr(best.from) << squareStr(best.to)
                       << "  (toplam " << elapsed << " sn)\n";

            board.makeMoveBlind(best, best.type);
        }
    }

    return 0;
}
