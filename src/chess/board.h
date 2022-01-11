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

#ifndef FIREFLY_BOARD_H
#define FIREFLY_BOARD_H

#include <utils/bit_manip.h>
#include <iostream>
#include <vector>
#include <map>
#include <cassert>
//#include <external/Fathom/src/tbprobe.h>
#include <config.h>
#include <testing/timer.h>
#include <external/xxHash/xxh3.h>
#include <utils/utils.h>

#define PAWNS 0
#define ROOKS 1
#define BISHOPS 2
#define KNIGHTS 3
#define KINGS 4
#define QUEENS 5

#define NO_PIECE 6
#define NO_SQUARE 8


#define DISENTANGLED_PIECES(board) auto pieces = board.disentangle_pieces(); \
uint64_t& kings = pieces.kings, &queens = pieces.queens, &rooks = pieces.rooks,\
&bishops = pieces.bishops, &knights = pieces.knights, &pawns = pieces.pawns;



namespace chess {

extern const int tb_maxcount ;


    typedef char piece_type;

    enum game_state
    {
        playing = 0, checkmate = 1,
        draw = 2, stalemate=2, insufficient_material=2,
        tablebase_win=3,
        tablebase_cursed_win=4,
        tablebase_draw=4,
        tablebase_blessed_loss=4,
        tablebase_loss=5,
    };




#ifdef USE_PRAGMA_PACK_1
#pragma pack(push, 1)
#endif


    //region Structs

    // Should be 15 bits
    struct move {
        uint8_t
        src:6,
        dst:6,
        promotion:4; // 1 unused bit

        move(uint8_t src, uint8_t dst,uint8_t promotion=PAWNS);
        move(std::string const& uci_move);
        move();
        move(const move&) = default;

        void flip();
        string to_uci_move() const;
        uint16_t to_policy_index() const;
        uint16_t to_flipped_policy_index() const;
        bool operator==(move const& other) const;
    };

    struct movegen_result
    {
        move moves[1024];
        uint16_t moves_count;

        move& operator[](uint16_t const& index)
        {
            return moves[index];
        }

        operator uint16_t()
        {
            return moves_count;
        }
    };


    struct disentangled_pieces
    {
        uint64_t pawns, knights, bishops, rooks, queens, kings;//,
        //current_player_pieces, enemy_pieces;
    };

    //endregion


    /*
     * 34 bytes, gets padded to 40 without pragma pack 1, but the padded struct is way faster
     */
    struct board {



        //region Data

        // See entangle_pieces and disentangle_pieces
        // 256-bits
        uint64_t current_player_pieces;
        uint64_t bishops_queens_kings;
        uint64_t rooks_queens_knights;
        uint64_t pawns_knights_kings;

        // 16-bits

        // the variable name is somewhat misleading, its actual purpose is to determine which player should play next
        // (white = false, black = true)
        bool flipped:1;
        uint8_t en_passant_x : 3;
        bool en_passant_possible : 1;
        uint8_t castling_rights : 4;
        uint8_t halfmove_clock:6;
        bool has_repeated:1; // This only serves to improve the accuracy of the transposition tables
        //uint8_t fullmove_number;

        //bool unused: 1;
        //endregion

        //region Core
        board();
        void clear();
        void set_default_position();
        game_state generate_moves(movegen_result& result) const;
        bool make_move(move const& m);
        bool operator==(const board& other) const;
        bool tfr_compare(const board& other) const;
        //endregion


        //region FEN
        bool from_fen(string const &fen);
        string to_fen() const;
        //endregion



        //region Utils
        string print() const;
        piece_type get_piece(int8_t x, int8_t y) const;
        piece_type get_piece(uint8_t position) const;

        void board_orientation_to_current_player();
        void flip_board();

        inline const bool castles_white_queenside() const
        {
            return castling_rights & WHITE_QUEEN_SIDE;
        }
        inline const bool castles_white_kingside() const
        {
            return castling_rights & WHITE_KING_SIDE;
        }
        inline const bool castles_black_queenside() const
        {
            return castling_rights & BLACK_QUEEN_SIDE;
        }
        inline const bool castles_black_kingside() const
        {
            return castling_rights & BLACK_KING_SIDE;
        }

        inline const int8_t get_mask_idx(uint64_t const& bit) const
        {
            return bit & pawns_knights_kings ?
                   (bit & rooks_queens_knights ? KNIGHTS :
                    bit & bishops_queens_kings ? KINGS : PAWNS) :
                   bit & rooks_queens_knights ?
                   (bit & bishops_queens_kings ? QUEENS : ROOKS) :
                   bit & bishops_queens_kings ? BISHOPS : NO_PIECE;

            /*
            if (bit & pawns_knights_kings)
            {
                if (bit & rooks_queens_knights)
                    return KNIGHTS;
                if (bit & bishops_queens_kings)
                    return KINGS;
                return PAWNS;
            }
            if (bit & rooks_queens_knights)
            {
                if (bit & bishops_queens_kings)
                    return QUEENS;
                return ROOKS;
            }
            if (bit & bishops_queens_kings)
                return BISHOPS;

            return NO_PIECE;
             */
        }

        //endregion

        //region Evaluation
        /*
        float nnue_eval() const;
        game_state tb_probe_wdl(disentangled_pieces const& pieces, uint64_t const& enemy_pieces) const;
        game_state tb_probe_wdl() const;
        chess::move tb_probe_dtz() const;
        */
         //endregion


        //region Entangled pieces interface

        inline uint64_t all_pieces() const
        {
            return pawns_knights_kings | rooks_queens_knights | bishops_queens_kings;
        }

        inline disentangled_pieces disentangle_pieces() const
        {

            disentangled_pieces pieces;
            pieces.kings = bishops_queens_kings & pawns_knights_kings;
            pieces.queens = rooks_queens_knights & bishops_queens_kings;
            pieces.knights = pawns_knights_kings & rooks_queens_knights;

            pieces.bishops = set_difference(bishops_queens_kings, (pieces.queens | pieces.kings));
            pieces.rooks = set_difference(rooks_queens_knights, (pieces.queens | pieces.knights));
            pieces.pawns = set_difference(pawns_knights_kings, (pieces.knights | pieces.kings));

            //pieces.current_player_pieces = current_player_pieces;
            //pieces.enemy_pieces = get_enemy_pieces();

            return pieces;
        }

        inline disentangled_pieces disentangle_pieces_flipped()
        {
            auto bqk = reverse_bytes(bishops_queens_kings);
            auto rqn = reverse_bytes(rooks_queens_knights);
            auto pnk = reverse_bytes(pawns_knights_kings);


            disentangled_pieces pieces;
            pieces.kings = bqk & pnk;
            pieces.queens = rqn & bqk;
            pieces.knights = pnk & rqn;

            pieces.bishops = set_difference(bqk, (pieces.queens | pieces.kings));
            pieces.rooks = set_difference(rqn, (pieces.queens | pieces.knights));
            pieces.pawns = set_difference(pnk, (pieces.knights | pieces.kings));

            //pieces.current_player_pieces = reverse_bytes(current_player_pieces);

            //pieces.enemy_pieces = reverse_bytes(get_enemy_pieces());

            return pieces;
        }

        inline void entangle_pieces(disentangled_pieces const& pieces)
        {
            bishops_queens_kings = pieces.bishops | pieces.queens | pieces.kings;
            pawns_knights_kings = pieces.pawns | pieces.knights | pieces.kings;
            rooks_queens_knights = pieces.rooks | pieces.queens | pieces.knights;
        }

        inline uint64_t get_enemy_pieces()  const
        {
            return set_difference(all_pieces(), current_player_pieces);
        }


        inline void flip_queens(uint64_t const& bit)
        {
            rooks_queens_knights ^= bit;
            bishops_queens_kings ^= bit;
        }

        inline void flip_kings(uint64_t const& bit)
        {
            pawns_knights_kings ^= bit;
            bishops_queens_kings ^= bit;
        }

        inline void flip_knights(uint64_t const& bit)
        {
            pawns_knights_kings ^= bit;
            rooks_queens_knights ^= bit;
        }

        inline void flip_pawns(uint64_t const& bit)
        {
            pawns_knights_kings ^= bit;
        }

        inline void flip_rooks(uint64_t const& bit)
        {
            rooks_queens_knights ^= bit;
        }

        inline void flip_bishops(uint64_t const& bit)
        {
            bishops_queens_kings ^= bit;
        }

        template<int PieceType>
        inline void flip_piece(uint64_t const& bit)
        {
            if constexpr (PieceType == PAWNS) flip_pawns(bit);
            else if constexpr(PieceType == KNIGHTS) flip_knights(bit);
            else if constexpr(PieceType == BISHOPS) flip_bishops(bit);
            else if constexpr(PieceType == ROOKS) flip_rooks(bit);
            else if constexpr(PieceType == QUEENS) flip_queens(bit);
            else if constexpr(PieceType == KINGS) flip_kings(bit);
            else if constexpr(PieceType == NO_PIECE) return;
            else throw std::invalid_argument("Invalid piece type.");
        }
        //endregion

        uint64_t hash() const;
        std::map<string, uint64_t> perft(int depth);

        string move_to_algebraic_notation(chess::move const&);

    private:
        uint64_t perft_internal(int depth);
    };
#ifdef USE_PRAGMA_PACK_1
#pragma pack(pop)
#endif
};
#endif //FIREFLY_BOARD_H
