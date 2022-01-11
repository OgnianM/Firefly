/*
    Firefly Chess Engine
    Copyright (C) 2022  Ognyan Mirev

    This program is free software: you can redistribute it and/or modify
            it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
            but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/


#include "chess_gui.h"
#include <thread>

using namespace std;

BoardGUI::BoardGUI(int square_size) : square_size(square_size),
                                      wnd(sf::VideoMode(square_size*8, square_size*8), "Chess"),
                                      dark_square(sf::Vector2f(square_size, square_size)),
                                      light_square(sf::Vector2f(square_size, square_size)),
                                      highlight_square(sf::Vector2f(square_size, square_size))

{
    dark_square.setFillColor(sf::Color::Cyan);
    light_square.setFillColor(sf::Color::White);
    highlight_square.setFillColor(sf::Color::Red);
    pieces.loadFromFile("../chess_pieces.png");
    pieces_sprite.setTexture(pieces);

    float target_width = 6*square_size, target_height = 2*square_size;

    x_scale = target_width / pieces.getSize().x,
            y_scale = target_height / pieces.getSize().y;

    unscaled_square_size = square_size / x_scale;
    pieces_sprite.scale(x_scale,y_scale);
}


void BoardGUI::Update(const chess::board &b)
{
    auto board = b;
    board.flip_board();
    auto draw_square = [&](int8_t x, int8_t y, sf::RectangleShape& rect)
    {
        rect.setPosition(x*square_size, y*square_size);
        wnd.draw(rect);
        auto piece = board.get_piece(x,y);


        if (piece != '.')
        {
            int img_y = 1;
            if (piece > 'A' && piece < 'Z' )
            {
                if (board.flipped)
                img_y = 0;
                piece = tolower(piece);

            }
            else if (piece > 'a' && piece < 'z' )
            {
                if (!board.flipped)
                    img_y = 0;
            }
            static unordered_map<char, int> piece_map =
                    {
                            {'k', 0},
                            {'q', 1},
                            {'b', 2},
                            {'n', 3},
                            {'r', 4},
                            {'p',5}
                    };

            int img_x = piece_map[piece];

            pieces_sprite.setTextureRect({img_x * unscaled_square_size, img_y*unscaled_square_size, unscaled_square_size, unscaled_square_size});
            pieces_sprite.setPosition(x*square_size, y*square_size);
            wnd.draw(pieces_sprite);
        }
    };

    wnd.clear();
    bool color = true;
    for (int8_t x = 0; x < 8; x++)
    {
        for (int8_t y = 0; y < 8; y++)
        {
            for (auto& i : highlighted_squares)
            {
                if (i.dst == get_index(x,y))
                {
                    draw_square(x,y,highlight_square);
                    goto neg_color;
                }
            }
            draw_square(x,y,color ? light_square : dark_square);
            neg_color:
            color = !color;
        }
        color = !color;
    }
    //wnd.draw(pieces_sprite);
    wnd.display();
}


chess::move BoardGUI::GetMove(chess::board &b)
{
    auto board = b;
    board.flip_board();

    chess::movegen_result possible_moves;
    auto game_state = board.generate_moves(possible_moves);

    if (game_state == chess::game_state::draw || game_state == chess::game_state::checkmate)
    {
        cout << "Game end value = " << game_state << endl;
        board.set_default_position();
    }

    int selx = -1,sely = -1;
    sf::Event ev;
    while (true)
    {
        this_thread::sleep_for(1ms);
        if(wnd.pollEvent(ev))
            switch (ev.type) {
                case sf::Event::Closed:
                    wnd.close();
                    break;
                case sf::Event::MouseButtonPressed:
                    if (ev.mouseButton.button == sf::Mouse::Left)
                    {
                        if (selx != -1)
                        {
                            int dstx = ev.mouseButton.x / this->square_size, dsty = ev.mouseButton.y / this->square_size;

                            for (chess::move i : highlighted_squares)
                            {
                                if (get_index(dstx, dsty) == i.dst)
                                {
                                    highlighted_squares.clear();
                                    Update(b);

                                    i.flip();

                                    return i;
                                }
                            }
                            selx= -1;
                            highlighted_squares.clear();
                            Update(b);

                        }else {
                            selx = ev.mouseButton.x / this->square_size, sely = ev.mouseButton.y / this->square_size;

                            for (uint8_t i = 0; i < possible_moves.moves_count; i++)
                            {
                                if (get_index(selx, sely) == possible_moves.moves[i].src) {
                                    highlighted_squares.push_back(possible_moves.moves[i]);
                                }
                            }

                            if (highlighted_squares.empty()) selx=-1;
                        }
                        Update(b);
                    }
                    break;
            }
        //std::this_thread::sleep_for(5ms);
    }


}


void BoardGUI::Play(chess::board &board)
{
    while(true)
    {
        Update(board);
        board.make_move(GetMove(board));
    }
}