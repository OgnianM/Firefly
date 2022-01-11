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


#ifndef FIREFLY_CHESS_GUI_H
#define FIREFLY_CHESS_GUI_H


#include <chess/board.h>
#include <SFML/Graphics.hpp>

struct BoardGUI
{
    BoardGUI(int square_size=120);
    void Update(const chess::board& board);
    chess::move GetMove(chess::board& board);

    void Play(chess::board& board);
private:

    std::vector<chess::move> highlighted_squares;
    float x_scale, y_scale;
    int square_size, unscaled_square_size;
    sf::RenderWindow wnd;
    sf::Texture pieces;
    sf::Sprite pieces_sprite;
    sf::RectangleShape dark_square;
    sf::RectangleShape light_square, highlight_square;
};

#endif //FIREFLY_CHESS_GUI_H
