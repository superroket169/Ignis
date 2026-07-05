#include "types.h"
#include "search.h"
#include <iostream>
#include <sstream>
#include <string>

using namespace Ignis;

bool parseSquareUci(const std::string& s, size_t offset, Square& out)
{
    if (s.size() < offset + 2) return false;
    char fc = s[offset];
    char rc = s[offset + 1];
    if (fc < 'a' || fc > 'h' || rc < '1' || rc > '8') return false;
    out = filerank_square(File(fc - 'a'), Rank(rc - '1'));
    return true;
}

bool applyUciMove(BitBoard& board, const std::string& uciMove)
{
    Square from, to;
    if (!parseSquareUci(uciMove, 0, from)) return false;
    if (!parseSquareUci(uciMove, 2, to))   return false;

    PromotionPiece promo = PromotionPiece::None;
    if (uciMove.size() >= 5)
    {
        switch (uciMove[4])
        {
            case 'q': promo = PromotionPiece::Queen;  break;
            case 'r': promo = PromotionPiece::Rook;   break;
            case 'b': promo = PromotionPiece::Bishop; break;
            case 'n': promo = PromotionPiece::Knight; break;
        }
    }

    BitMove mv(from, to, MoveType::NORMAL, promo);
    return board.makeMove(mv).has_value();
}

std::string moveToUci(const BitMove& mv)
{
    std::string s;
    s += (char)('a' + file_of(mv.from));
    s += (char)('1' + rank_of(mv.from));
    s += (char)('a' + file_of(mv.to));
    s += (char)('1' + rank_of(mv.to));

    if (mv.type == MoveType::PROMOTION)
    {
        switch (mv.promotion)
        {
            case PromotionPiece::Queen:  s += 'q'; break;
            case PromotionPiece::Rook:   s += 'r'; break;
            case PromotionPiece::Bishop: s += 'b'; break;
            case PromotionPiece::Knight: s += 'n'; break;
            default: break;
        }
    }
    return s;
}

int main()
{
    BitBoard board;
    Engine engine;

    std::string line;
    while (std::getline(std::cin, line))
    {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci")
        {
            std::cout << "id name Ignis\n";
            std::cout << "id author Ignis\n";
            std::cout << "uciok" << std::endl;
        }
        else if (cmd == "isready")
        {
            std::cout << "readyok" << std::endl;
        }
        else if (cmd == "ucinewgame")
        {
            board  = BitBoard();
            engine = Engine();
        }
        else if (cmd == "position")
        {
            std::string token;
            iss >> token;

            if (token == "startpos")
            {
                board = BitBoard();
                iss >> token;
            }
            else if (token == "fen")
            {
                std::string fen, part;
                while (iss >> part && part != "moves")
                    fen += part + " ";
                board.loadFEN(fen);
                token = part;
            }

            if (token == "moves")
            {
                std::string mv;
                while (iss >> mv)
                    applyUciMove(board, mv);
            }
        }
        else if (cmd == "go")
        {
            std::string token;
            int movetime = -1;
            int wtime = -1, btime = -1, winc = 0, binc = 0;
            int depthLimit = -1;

            while (iss >> token)
            {
                if      (token == "movetime") iss >> movetime;
                else if (token == "wtime")    iss >> wtime;
                else if (token == "btime")    iss >> btime;
                else if (token == "winc")     iss >> winc;
                else if (token == "binc")     iss >> binc;
                else if (token == "depth")    iss >> depthLimit;
            }

            int timeBudgetMs;
            if (movetime > 0)
            {
                timeBudgetMs = movetime - 50;
            }
            else
            {
                int myTime = (board.getTurn() == WHITE) ? wtime : btime;
                int myInc  = (board.getTurn() == WHITE) ? winc  : binc;

                if (myTime > 0)
                    timeBudgetMs = myTime / 30 + myInc / 2;
                else
                    timeBudgetMs = 3000;
            }

            if (timeBudgetMs < 50)    timeBudgetMs = 50;
            if (timeBudgetMs > 60000) timeBudgetMs = 60000;

            size_t depth = (depthLimit > 0) ? (size_t)depthLimit : 64;

            BitMove best = engine.getBestMove(board, depth, timeBudgetMs);

            std::cout << "bestmove " << (best.from == SQ_NONE ? "0000" : moveToUci(best)) << std::endl;
        }
        else if (cmd == "stop")
        {

        }
        else if (cmd == "quit")
        {
            break;
        }
    }

    return 0;
}
