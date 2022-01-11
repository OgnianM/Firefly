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

#include "board.h"
#include <cmath>
#include <cstring>
#include <sstream>
#include <string>
#include <utils/utils.h>
#include <immintrin.h>
//#include <engine/neural/utils/compressed_policy_map.h>
//#include <engine/neural/utils/uncompressed_policy_map.h>
#include <engine/neural/utils/uncompressed_policy_map.h>
using namespace chess;
using namespace std;

extern uint64_t cycle_counter;


const int chess::tb_maxcount = 6;

string chess::move::to_uci_move() const
{
    string res;
    res += (src & 7) + 'a';
    res += (src >> 3) + '1';
    res += (dst & 7) + 'a';
    res += (dst >> 3) + '1';

    switch(promotion)
    {
        case ROOKS:
            res += 'r';
            break;
        case BISHOPS:
            res += 'b';
            break;
        case KNIGHTS:
            res += 'n';
            break;
        case QUEENS:
            res += 'q';
            break;
    }

    return res;
}


chess::move::move(uint8_t src, uint8_t dst,uint8_t promotion) : src(src), dst(dst), promotion(promotion)
{}

chess::move::move(std::string const& uci_move)
{
    int srcx = uci_move[0] - 'a';
    int srcy = uci_move[1] - '1';

    int dstx = uci_move[2] - 'a';
    int dsty = uci_move[3] - '1';

    src = get_index(srcx, srcy);
    dst = get_index(dstx, dsty);

    if (uci_move.size() > 4)
    {
        switch (tolower(uci_move[4]))
        {
            case 'q':
                promotion = QUEENS;
                break;
            case 'r':
                promotion = ROOKS;
                break;
            case 'b':
                promotion = BISHOPS;
                break;
            case 'n':
                promotion = KNIGHTS;
                break;

            default:
                throw std::invalid_argument(string("Invalid promotion: ") + uci_move[4]);
        }
    }
    else promotion = PAWNS;
}

chess::move::move(){}

bool chess::move::operator==(const move &other) const {
    return src == other.src && dst == other.dst && promotion == other.promotion;
}


uint16_t chess::move::to_policy_index() const
{

    //Key: 3838590963604091776
    //Shift: 51  max idx = 8190
    //return policy_map[((src | (dst << 6) | (promotion << 12)) * 0x35456A24C7A57380) >> 51];

    uint16_t final_index;
    if (promotion)
    {
        //cout << (dst & 56) << endl;
        if ((dst & 56) == 0)
        {
            //cout << "Hi" << endl;
            // flip high 3 bits in src and dst
            auto flipped_src_y = 56 - (src & 56);
            auto flipped_dst_y = 56 - (dst & 56);



            final_index = ((src & 7) | flipped_src_y );
            final_index |= ((dst & 7) | flipped_dst_y) << 6;
            final_index |= (KNIGHTS == promotion) ? 0 : promotion << 12;

        }
        else
        {
            final_index = (src | (dst << 6)) | (KNIGHTS == promotion ? 0 : promotion << 12);
        }
    }
    else final_index = src | (dst << 6);

    final_index = (src | (dst << 6)) | (KNIGHTS == promotion ? 0 : promotion << 12);

    return uncompressed_policy_map[final_index];
    //return inverted_policy_map[to_uci_move()];
}



void chess::move::flip()
{
    auto flipped_src_y = 56 - (src & 56);
    auto flipped_dst_y = 56 - (dst & 56);

    src = (src & 7) | flipped_src_y;
    dst = (dst & 7) | flipped_dst_y;
}

uint16_t chess::move::to_flipped_policy_index() const
{
    auto flipped_src_y = 56 - (src & 56);
    auto flipped_dst_y = 56 - (dst & 56);



    auto final_index = ((src & 7) | flipped_src_y );
    final_index |= ((dst & 7) | flipped_dst_y) << 6;
    final_index |= (KNIGHTS == promotion) ? 0 : promotion << 12;

    return uncompressed_policy_map[final_index];
}

board::board()
{
    //clear();
    has_repeated = false;
}

void board::clear()
{
    memset(this, 0, sizeof(board));
}



void board::set_default_position()
{
    //clear();

    //from_fen(startpos_fen);

    this->en_passant_x = 0;
    this->en_passant_possible = false;
    this->halfmove_clock = 0;

    current_player_pieces = pow(2,16)-1;

    disentangled_pieces pieces;
    pieces.pawns = set_difference(current_player_pieces, 255);
    pieces.pawns |= pieces.pawns << 40;

    pieces.rooks = 129;
    pieces.rooks |= pieces.rooks << 56;

    pieces.knights = 66;
    pieces.knights |= pieces.knights << 56;

    pieces.bishops = 36;
    pieces.bishops |= pieces.bishops << 56;

    pieces.kings = 16;
    pieces.kings |= pieces.kings << 56;

    pieces.queens = 8;
    pieces.queens |= pieces.queens << 56;



    entangle_pieces(pieces);

    castling_rights = 0b1111;
}

game_state board::generate_moves(movegen_result& result) const
{
    //if (halfmove_clock > 50) [[unlikely]] return draw;
    result.moves_count = 0;
    auto add_move = [&result](move move)
    {
        result.moves[result.moves_count++] = move;
    };
    auto last_move = [&result]()
    {
        return result.moves + result.moves_count - 1;
    };


    uint64_t bit = 1;
    uint8_t i = 0;

    auto knights = rooks_queens_knights & pawns_knights_kings;
    auto kings = bishops_queens_kings & pawns_knights_kings;
    auto pawns = set_difference(pawns_knights_kings, (knights | kings));

    auto rooks_or_queens = set_difference(rooks_queens_knights, knights);
    auto bishops_or_queens = set_difference(bishops_queens_kings, kings);

    // Check for insufficient material
    if (!pawns && !rooks_or_queens && (std::popcount(bishops_or_queens) + std::popcount(knights)) < 2)
        return game_state::insufficient_material;

    auto iterate_bits = [&bit,&i](uint64_t bitboard, auto func)
    {
        while (bitboard)
        {
            bit = poplsb(bitboard);
            i = __builtin_ctzll(bit);
            func();
        }
    };

    uint64_t piece_union = all_pieces();

    uint64_t enemy_pieces = set_difference(piece_union, current_player_pieces);


    bool en_passant_possible = this->en_passant_possible;

    uint64_t empty_squares = ~piece_union;
    uint64_t pieces_complement = ~current_player_pieces, attacks;

    uint64_t king_square = current_player_pieces & kings;
    uint8_t king_pos = __builtin_ctzll(king_square);

    uint64_t enemy_attacked_squares = 0,
            union_excluding_king = piece_union ^ king_square;
    uint64_t non_attacked_squares;

    uint64_t king_attacker, king_attacker_attack_mask;
    uint8_t pieces_attacking_king = 0;
    bool attacker_is_slider = false;

    uint64_t pinned_pieces = 0;

    uint64_t pinned_pieces_move_mask[64];

    uint64_t bishop_pin_ray_mask[4];
    uint64_t rook_pin_ray_mask[4];


    for (int i = 0; i < 4; i++)
    {
        rook_pin_ray_mask[i]   = ~rook_pin_rays[king_pos][i] | king_square;
        bishop_pin_ray_mask[i] = ~bishop_pin_rays[king_pos][i] | king_square;
    }

    uint64_t enemy_rooks_or_queens = rooks_or_queens & enemy_pieces;
    uint64_t enemy_bishops_or_queens = bishops_or_queens & enemy_pieces;

    const uint64_t clip_right_side = ~0x101010101010101,
            clip_left_side = ~0x8080808080808080;

    auto calculate_pins_and_king_attacks_rooks_vert = [&](int&& z)
    {
        auto ray = rook_pin_rays[i][z];
        if (ray & king_square) {
            ray &= rook_pin_ray_mask[z];
            auto pieces_in_ray = ray & piece_union;

            auto count = popcount(pieces_in_ray);

            switch (count) {
                case 2: // Piece is pinned
                {
                    pieces_in_ray ^= king_square;
                    if (current_player_pieces & pieces_in_ray) {
                        pinned_pieces |= pieces_in_ray;
                        pinned_pieces_move_mask[__builtin_ctzll(pieces_in_ray)] = (ray | bit);
                    }
                    break;
                }
                case 1: // King is attacked
                {
                    king_attacker = bit;
                    pieces_attacking_king++;
                    king_attacker_attack_mask = ray;
                    attacker_is_slider = true;
                    break;
                }
            }
            return true;
        }
        return false;
    };

    auto calculate_pins_and_king_attacks_bishops = [&]()
    {
        for (int z = 0; z < 4; z++)
        {
            auto ray = bishop_pin_rays[i][z];
            if (ray & king_square)
            {
                ray &= bishop_pin_ray_mask[z];
                auto pieces_in_ray = ray & piece_union;

                auto count = popcount(pieces_in_ray);

                switch(count)
                {
                    case 2: // Piece is pinned
                    {
                        pieces_in_ray ^= king_square;
                        if (current_player_pieces & pieces_in_ray) {
                            pinned_pieces |= pieces_in_ray;
                            pinned_pieces_move_mask[__builtin_ctzll(pieces_in_ray)] = (ray | bit);
                        }
                    } break;
                    case 1: // King is attacked
                    {
                        king_attacker = bit;
                        pieces_attacking_king++;
                        king_attacker_attack_mask = ray;
                        attacker_is_slider = true;
                    } break;
                }
                return true;
            }
        }
        return false;
    };

    auto generate_moves_impl = [&]<int Color>()
    {
        auto calculate_pins_and_king_attacks_rooks_horz = [&](int z, int z_king)
        {
            auto ray = rook_pin_rays[king_pos][z_king];
            auto enemy_rq_xraying_or_attacking_king = ray & enemy_rooks_or_queens;


            if (!enemy_rq_xraying_or_attacking_king)
                return true;

            bit = poplsb(enemy_rq_xraying_or_attacking_king);

            if (enemy_rq_xraying_or_attacking_king)
            {
                if (king_square > enemy_rq_xraying_or_attacking_king)
                {
                    while (enemy_rq_xraying_or_attacking_king)
                        bit = poplsb(enemy_rq_xraying_or_attacking_king);
                }
            }


            i = __builtin_ctzll(bit);
            //bit = rook_bit;

            ray = rook_pin_rays[i][z];
            {
                ray &= rook_pin_ray_mask[z];
                auto pieces_in_ray = ray & piece_union;

                auto count = popcount(pieces_in_ray);

                switch (count) {
                    case 2: // Piece is pinned
                    {
                        pieces_in_ray ^= king_square;
                        if (current_player_pieces & pieces_in_ray) {
                            pinned_pieces |= pieces_in_ray;
                            pinned_pieces_move_mask[__builtin_ctzll(pieces_in_ray)] = (ray | bit);
                        }
                        break;
                    }
                    case 1:  // King is attacked
                    {
                        king_attacker = bit;
                        pieces_attacking_king++;
                        king_attacker_attack_mask = ray;
                        attacker_is_slider = true;
                        break;
                    }

                    case 3: {
                        /*
                            If there are two pawns horizontally interposed between a rook (or queen)
                            and the king, en passant is impossible, because it would expose the king.

                            en_passant_possible will only be set to true if there's a pawn that can
                            be taken en passant AND an enemy pawn that can take that pawn en passant.

                            Therefore, if there are three pieces in a rook's attack ray and one of
                            them is a king and en passant is possible and the rook is on the rank
                            where en passant would normally happen (rank 4 for white, rank 3 for black) then the two
                            remaining pieces in the rook's attack mask are necessarily the two pawns
                            that may participate in an en passant capture, in this specific case,
                            set en_passant_possible to false.

                            Once we get to this point, we already know that there are 3 pieces in a
                            rook's attack mask and that one of them is a king, so we just need to
                            check if the rook is on the proper rank and set en_passant_possible
                            to false if it is.

                            If en_passant_possible was already false, nothing changes.
                         */
                        en_passant_possible &= bool((i >> 3) != (4 - (Color == C_BLACK)));
                        break;
                    }
                }
                return true;
            }
            return false;
        };





        // region Enemy pawn calculations
        /*
         * Calculate all squares attacked by enemy pawns
         * as well as the check caused by an enemy pawn if there is one.
         */
        auto enemy_pawns = enemy_pieces & pawns;

        auto enemy_pawn_attacks_right = Color == C_BLACK ?
                ((enemy_pawns & clip_right_side) << 7) :
                ((enemy_pawns & clip_left_side) >> 7);

        auto enemy_pawn_attacks_left = Color == C_BLACK ?
                                       ((enemy_pawns & clip_left_side) << 9) :
                                       ((enemy_pawns & clip_right_side) >> 9);


        auto attacked_king_square = enemy_pawn_attacks_right & king_square;
        pieces_attacking_king = bool(attacked_king_square);
        king_attacker = Color == C_WHITE ? attacked_king_square << 7 : attacked_king_square >> 7;


        attacked_king_square = enemy_pawn_attacks_left & king_square;
        pieces_attacking_king |= bool(attacked_king_square);
        king_attacker |= Color == C_WHITE ? attacked_king_square << 9 : attacked_king_square >> 9;

        enemy_attacked_squares |= enemy_pawn_attacks_right | enemy_pawn_attacks_left;

        //endregion






        iterate_bits(enemy_rooks_or_queens, [&](){
            enemy_attacked_squares |= get_rook_attacks(union_excluding_king, i);
        });
        iterate_bits(enemy_bishops_or_queens, [&](){
            enemy_attacked_squares |= get_bishop_attacks(union_excluding_king, i);
            calculate_pins_and_king_attacks_bishops();
        });
        iterate_bits(enemy_pieces & knights, [&]()
        {
            auto attacks = knight_attacks[i];
            if (attacks & king_square)
            {
                pieces_attacking_king++;
                king_attacker = bit;
                king_attacker_attack_mask = attacks;
                attacker_is_slider = false;
            }
            enemy_attacked_squares |= attacks;
        });


        calculate_pins_and_king_attacks_rooks_horz(0,1);
        calculate_pins_and_king_attacks_rooks_horz(1,0);
        calculate_pins_and_king_attacks_rooks_horz(2,3);
        calculate_pins_and_king_attacks_rooks_horz(3,2);

        enemy_attacked_squares |= king_moves[__builtin_ctzll(enemy_pieces & kings)];

        non_attacked_squares = ~enemy_attacked_squares;

        auto create_moves = [&]<bool MaybePromotion = false>(uint8_t const& i, uint64_t moves)
        {

            if (pinned_pieces & bit) [[unlikely]]{
                moves &= pinned_pieces_move_mask[i];
            }

            uint64_t lsb;

            for (lsb = poplsb(moves); lsb; lsb = poplsb(moves))
            {
                add_move(move(i,__builtin_ctzll(lsb)));


                if constexpr (MaybePromotion)
                {
                    if (lsb & pawn_promotion_squares) [[unlikely]]
                    {
                        last_move()->promotion = QUEENS;
                        add_move(*last_move());
                        last_move()->promotion = ROOKS;
                        add_move(*last_move());
                        last_move()->promotion = BISHOPS;
                        add_move(*last_move());
                        last_move()->promotion = KNIGHTS;
                    }
                }
            }
        };


        /*
         * Generates the moves for queens, rooks, bishops and knights
         * If the king is in check, the move filter should be the squares
         * where a piece could move to block the check, along with the square
         * of the king attacker.
         * If the king is not in check, the move filter should contain all empty
         * squares and all enemy pieces.
         */
        auto generate_moves_qrbn = [&](uint64_t const& move_filter)
        {
            iterate_bits(current_player_pieces & rooks_or_queens, [&]() {
                attacks = get_rook_attacks(piece_union, i) & move_filter;
                create_moves(i, attacks);
            });
            iterate_bits(current_player_pieces & bishops_or_queens, [&]() {
                attacks = get_bishop_attacks(piece_union, i) & move_filter;
                create_moves(i, attacks);
            });
            iterate_bits(current_player_pieces & knights, [&]() {
                attacks = knight_attacks[i] & move_filter;
                create_moves(i, attacks);
            });
            /*
            iterate_bits(current_player_pieces & queens, [&]() {
                attacks = get_rook_attacks(piece_union, i) | get_bishop_attacks(piece_union, i);
                create_moves(i, attacks & move_filter);
            });
             */
        };

        switch (pieces_attacking_king)
        {
            case 2: [[unlikely]] // Double check
            {
                auto evasions = king_moves[king_pos] & pieces_complement & non_attacked_squares;

                if (!evasions) return game_state::checkmate;

                create_moves(king_pos, evasions);
                return game_state::playing;
            }
            case 1: [[unlikely]] // Single check
            {
                auto evasions = king_moves[king_pos] & pieces_complement & non_attacked_squares;

                create_moves(king_pos, evasions);

                uint64_t interceptions_and_captures = king_attacker;
                if (attacker_is_slider)
                {
                    interceptions_and_captures |=
                            (get_bishop_attacks(piece_union, king_pos) & king_attacker_attack_mask) |
                            (get_rook_attacks(piece_union, king_pos) & king_attacker_attack_mask);
                }

                uint64_t pieces_compliment_and_interception_and_captures =
                        pieces_complement & interceptions_and_captures;

                iterate_bits(current_player_pieces & pawns, [&]() {

                    uint64_t pushes = 0;

                    if constexpr (Color == C_WHITE) {
                        pushes = bit << 8;//get_bit(i + 8);
                        pushes &= empty_squares;

                        if (pushes & 16711680) [[unlikely]] {
                            pushes |= (pushes << 8) ;
                            pushes &= empty_squares;
                        }
                    } else {
                        pushes = bit >> 8;//get_bit(i - 8);
                        pushes &= empty_squares;


                        if (pushes & 280375465082880) [[unlikely]] //(i >= 48 && i < 56)
                        {
                            pushes |= (pushes >> 8);
                            pushes &= empty_squares;
                        }
                    }

                    create_moves.template operator()<true>(i, (pushes |
                                                              (pawn_attacks[Color == C_BLACK][i] & enemy_pieces)) &
                                                              interceptions_and_captures);
                });

                generate_moves_qrbn(pieces_compliment_and_interception_and_captures);


                if (en_passant_possible)
                {
                    const auto ep_y = 4-(Color == C_BLACK);
                    auto ep_bit = get_bit(en_passant_x, ep_y);

                    if (ep_bit == king_attacker) [[unlikely]]
                    {
                        if (en_passant_x > 0 && ((ep_bit >> 1) & current_player_pieces & pawns))
                        {
                            bit = get_bit(en_passant_x-1, ep_y);

                            create_moves(get_index(en_passant_x-1, ep_y),
                                         get_bit(en_passant_x, Color == C_BLACK ? 2 : 5));
                            //add_move(chess::move(get_index(en_passant_x-1, ep_y),
                            //                     get_index(en_passant_x, Color == C_BLACK ? 2 : 5)));
                        }
                        if (en_passant_x < 7 && ((ep_bit << 1) & current_player_pieces & pawns))
                        {
                            bit = get_bit(en_passant_x+1, ep_y);

                            create_moves(get_index(en_passant_x+1, ep_y),
                                         get_bit(en_passant_x, Color == C_BLACK ? 2 : 5));

                            //add_move(chess::move(get_index(en_passant_x+1, ep_y),
                            //                     get_index(en_passant_x, Color == C_BLACK ? 2 : 5)));
                        }
                    }
                }

                if (result.moves_count)
                {
                    return game_state::playing;
                }

                // If no moves were generated, protecting the king is impossible.
                return game_state::checkmate;
            }
            case 0: [[likely]] // No check
            {
                auto our_pawns = current_player_pieces & pawns;

                auto non_pinned_pawns = our_pawns &~ pinned_pieces;
                uint64_t pushes, double_pushes, left_attacks, right_attacks;
                if constexpr (Color == C_WHITE)
                {
                    pushes = non_pinned_pawns << 8;
                    pushes &= empty_squares;

                    double_pushes = (pushes & 16711680) << 8;
                    double_pushes &= empty_squares;

                    left_attacks = ((non_pinned_pawns & clip_left_side) << 9) & enemy_pieces;
                    right_attacks = ((non_pinned_pawns & clip_right_side) << 7) & enemy_pieces;
                }
                else
                {
                    pushes = non_pinned_pawns >> 8;
                    pushes &= empty_squares;

                    double_pushes = (pushes & 280375465082880) >> 8;
                    double_pushes &= empty_squares;

                    left_attacks = ((non_pinned_pawns & clip_right_side) >> 9) & enemy_pieces;
                    right_attacks = ((non_pinned_pawns & clip_left_side) >> 7) & enemy_pieces;
                }

                iterate_bits(left_attacks, [&]() {
                    auto src = i + (Color == C_WHITE ? -9 : 9);
                    add_move(chess::move(src, i));

                    if (bit & pawn_promotion_squares) [[unlikely]] {
                        last_move()->promotion = QUEENS;
                        add_move(*last_move());
                        last_move()->promotion = ROOKS;
                        add_move(*last_move());
                        last_move()->promotion = BISHOPS;
                        add_move(*last_move());
                        last_move()->promotion = KNIGHTS;
                    }
                });

                iterate_bits(right_attacks, [&]() {
                    auto src = i + (Color == C_WHITE ? -7 : 7);
                    add_move(chess::move(src, i));

                    if (bit & pawn_promotion_squares) [[unlikely]] {
                        last_move()->promotion = QUEENS;
                        add_move(*last_move());
                        last_move()->promotion = ROOKS;
                        add_move(*last_move());
                        last_move()->promotion = BISHOPS;
                        add_move(*last_move());
                        last_move()->promotion = KNIGHTS;
                    }
                });

                iterate_bits(pushes, [&]() {
                    auto src = i + (Color == C_WHITE ? -8 : 8);
                    add_move(chess::move(src, i));

                    if (bit & pawn_promotion_squares) [[unlikely]] {
                        last_move()->promotion = QUEENS;
                        add_move(*last_move());
                        last_move()->promotion = ROOKS;
                        add_move(*last_move());
                        last_move()->promotion = BISHOPS;
                        add_move(*last_move());
                        last_move()->promotion = KNIGHTS;
                    }
                });

                iterate_bits(double_pushes, [&]()
                {
                    add_move(chess::move(i + (Color == C_WHITE ? -16 : 16), i));
                });


                iterate_bits(our_pawns & pinned_pieces, [&]() {

                    uint64_t pushes = 0;


                    if constexpr (Color == C_WHITE) {
                        pushes = bit << 8;//get_bit(i + 8);
                        pushes &= empty_squares;

                        if (pushes & 16711680) [[unlikely]]
                        {
                            pushes |= (pushes << 8) ;
                            pushes &= empty_squares;
                        }
                    } else {
                        pushes = bit >> 8;//get_bit(i - 8);
                        pushes &= empty_squares;


                        if (pushes & 280375465082880) [[unlikely]] //(i >= 48 && i < 56)
                        {
                            pushes |= (pushes >> 8);
                            pushes &= empty_squares;
                        }
                    }


                    create_moves.template operator()<true>(i, pushes |
                                                              (pawn_attacks[Color == C_BLACK][i] & enemy_pieces));

                });



                generate_moves_qrbn(pieces_complement);

                {
                    bit = king_square;
                    //attacks = (king_moves[king_pos] & pieces_complement) & non_attacked_squares;

                    create_moves(king_pos, (king_moves[king_pos] & pieces_complement) & non_attacked_squares);
                    if constexpr (Color == C_WHITE) {

                            if (castling_rights & WHITE_KING_SIDE) {
                                auto m = (white_king_side_castles_mask & piece_union) |
                                         (white_king_side_castles_mask & enemy_attacked_squares);
                                if (!m)
                                    add_move(chess::move(king_pos, (uint8_t) square_index::g1));
                            }
                            if (castling_rights & WHITE_QUEEN_SIDE) {
                                auto m = ((white_queen_side_castles_mask | b1) & piece_union) |
                                         (white_queen_side_castles_mask & enemy_attacked_squares);
                                if (!m)
                                    add_move(chess::move(king_pos, (uint8_t) square_index::c1));
                            }
                    }
                    else
                    {
                        if (castling_rights & BLACK_KING_SIDE) {
                            auto m = (black_king_side_castles_mask & piece_union) |
                                     (black_king_side_castles_mask & enemy_attacked_squares);
                            if (!m)
                                add_move(chess::move(king_pos, (uint8_t) square_index::g8));
                        }
                        if (castling_rights & BLACK_QUEEN_SIDE) {
                            auto m = ((black_queen_side_castles_mask | b8) & piece_union) |
                                     (black_queen_side_castles_mask & enemy_attacked_squares);
                            if (!m)
                                add_move(chess::move(king_pos, (uint8_t) square_index::c8));
                        }
                    }
                }


                if (en_passant_possible) [[unlikely]]
                {
                    auto ep_y = 4 - (Color == C_BLACK);
                    auto ep_bit = get_bit(en_passant_x, ep_y);

                    if (en_passant_x > 0 && ((ep_bit >> 1) & current_player_pieces & pawns)) {

                        bit = get_bit(en_passant_x-1, ep_y);
                        create_moves(get_index(en_passant_x-1, ep_y),
                                     get_bit(en_passant_x, Color == C_BLACK ? 2 : 5));

//                        add_move(chess::move(get_index(en_passant_x - 1, ep_y),
  //                                           get_index(en_passant_x, Color == C_BLACK ? 2 : 5)));
                    }
                    if (en_passant_x < 7 && ((ep_bit << 1) & current_player_pieces & pawns)) {

                        bit = get_bit(en_passant_x+1, ep_y);
                        create_moves(get_index(en_passant_x+1, ep_y),
                                     get_bit(en_passant_x, Color == C_BLACK ? 2 : 5));

                        //add_move(chess::move(get_index(en_passant_x + 1, ep_y),
                        //                     get_index(en_passant_x, Color == C_BLACK ? 2 : 5)));
                    }
                }


                return result.moves_count ? game_state::playing : game_state::draw;
            }
            default:
                throw std::logic_error("Invalid number of king attackers: " + to_string(pieces_attacking_king));
        }




    };


    if (flipped)
        return generate_moves_impl.operator()<C_BLACK>();
    else return generate_moves_impl.operator()<C_WHITE>();
}


map<string, uint64_t> board::perft(int depth) {

    map<string, uint64_t> result;

    movegen_result moves;
    generate_moves(moves);

    board temp;
    temp = *this;

    for (int i = 0; i < moves.moves_count; i++)
    {
        temp.make_move(moves.moves[i]);
        result[moves.moves[i].to_uci_move()] = temp.perft_internal(depth - 1);

        //temp.unmake_move(moves.moves[i], original_castling_rights);

        temp = *this;
        //nodes += temp.perft(depth-1);
    }

    return result;
}

uint64_t board::perft_internal(int depth) {
    if (depth == 0) return 1;
    uint64_t nodes = 0;

    movegen_result moves;

    generate_moves(moves);

    board temp;
    temp = *this;

    for (int i = 0; i < moves.moves_count; i++)
    {
        temp.make_move(moves.moves[i]);
        nodes += temp.perft_internal(depth - 1);
        temp = *this;
        //temp.unmake_move(moves.moves[i], orig_castling_rights);
    }

    return nodes;
}
#include <unordered_map>
string board::print() const
{
    uint64_t bit = 1;

    static  unordered_map<char, string> unicode_pieces {{'K', "\u2654"}, {'Q', "\u2655"}, {'R', "\u2656"},
                                        {'B', "\u2657"}, {'N', "\u2658"},{'P', "\u2659"},
                                        {'k', "\u265A"}, {'q', "\u265B"}, {'r', "\u265C"},
                                        {'b', "\u265D"}, {'n', "\u265E"}, {'p', "\u265F"}, {'.', "."}};
    string res;
    res.reserve(200);

    //std::map<char, wchar_t> p2u8p = {'k', L'U+2654'};

    for (int i = 0; i < 64; i++, bit <<= 1)
    {
        if (i % 8 == 0 && i != 0) res += '\n';

        res += ' ';
        res += unicode_pieces[get_piece(i)];
        res += ' ';
    }

    return res;
}

string board::to_fen() const {
    string fen;

    uint64_t black_pieces, white_pieces;

    if (flipped)
    {
        white_pieces = all_pieces() &~ current_player_pieces;
    }
    else
    {
        white_pieces = current_player_pieces;
    }

    char empty_squares = 0;

    auto add = [&](uint64_t const& bit, char piece)
    {
        if (empty_squares != 0)
        {
            fen += empty_squares;
            empty_squares = 0;
        }
        if (white_pieces & bit) fen += toupper(piece);
        else fen += piece;
    };

    for (int y = 7; y >= 0; y--)
    {
        for (int x = 0; x < 8; x++)
        {
            auto bit = get_bit(x,y);

            auto mask = get_mask_idx(bit);

            switch (mask)
            {
                case PAWNS: add(bit, 'p'); break;
                case KNIGHTS: add(bit, 'n'); break;
                case BISHOPS: add(bit, 'b'); break;
                case ROOKS: add(bit, 'r'); break;
                case QUEENS: add(bit, 'q'); break;
                case KINGS: add(bit, 'k'); break;

                default:
                    if (empty_squares == 0)
                        empty_squares = '1';
                    else empty_squares++;
                    break;
            }
        }


        if (empty_squares != 0)
        {
            fen += empty_squares;
            empty_squares = 0;
        }
        fen += '/';
    }

    fen.pop_back();

    fen += ' ';
    fen += flipped ? 'b' : 'w';
    fen += ' ';

    if (castling_rights & WHITE_KING_SIDE) fen += 'K';
    if (castling_rights & WHITE_QUEEN_SIDE) fen += 'Q';
    if (castling_rights & BLACK_KING_SIDE) fen += 'k';
    if (castling_rights & BLACK_QUEEN_SIDE) fen += 'q';

    if (fen.back() == ' ') fen += '-';

    fen += ' ';
    if (en_passant_possible)
    {
        fen += ('a' + en_passant_x);
        fen += ('1' + 4 - flipped);
    }
    else fen += '-';
    fen += ' ';

    fen += to_string(halfmove_clock);
    fen += ' ';
    fen += to_string(0);//fullmove_number);

    return fen;
}

bool board::from_fen(string const& fen)
{
    clear();
    stringstream ss(fen);

    string piece_positions, color_to_play, castling_rights, en_passant_target, halfmove_clock, fullmove_number;
    if (!getline(ss, piece_positions, ' ')) return false;
    if (!getline(ss, color_to_play, ' ')) return false;
    if (!getline(ss, castling_rights, ' ')) return false;
    if (!getline(ss, en_passant_target, ' ')) return false;

    if (!getline(ss, halfmove_clock, ' ')) return false;
    if (!getline(ss, fullmove_number, ' ')) return false;


    if (en_passant_target != "-")
    {
        en_passant_possible = true;
        en_passant_x = en_passant_target[0]-'a';
    }

    flipped = color_to_play[0] == 'b';

    this->halfmove_clock = stoi(halfmove_clock);
    //this->fullmove_number = stoi(fullmove_number);

    for (auto& i : castling_rights)
    {
        switch (i)
        {
            case 'K':
                this->castling_rights |= WHITE_KING_SIDE;
                break;
            case 'Q':
                this->castling_rights |= WHITE_QUEEN_SIDE;
                break;
            case 'k':
                this->castling_rights |= BLACK_KING_SIDE;
                break;
            case 'q':
                this->castling_rights |= BLACK_QUEEN_SIDE;
                break;

            case '-':
                break;

            default:
                throw std::invalid_argument("Invalid castling rights in FEN: " + castling_rights);
        }
    }


    uint64_t white_pieces = 0, black_pieces = 0;


    int fen_idx = 0, board_x = 0, board_y = 7;

    for (; fen_idx < piece_positions.size(); fen_idx++)
    {
        if (piece_positions[fen_idx] >= '0' && piece_positions[fen_idx] <= '9')
        {
            board_x += piece_positions[fen_idx]- '0';
            if (board_x > 7)
            {
                board_x &= 7;
                board_y--;
            }
            continue;
        }
        else if (piece_positions[fen_idx] == ' ')  break;
        else
        {
            if (piece_positions[fen_idx] >= 'a' && piece_positions[fen_idx] <= 'z')
                set_bit(black_pieces, board_x, board_y);
            else if (piece_positions[fen_idx] != '/')
            {
                set_bit(white_pieces, board_x, board_y);
            }
            else
            {
                continue;
            }
            switch (tolower(piece_positions[fen_idx]))
            {
                case 'p':
                    flip_pawns(get_bit(board_x, board_y));
                    break;
                case 'n':
                    flip_knights(get_bit(board_x, board_y));
                    break;
                case 'r':
                    flip_rooks(get_bit(board_x, board_y));
                    break;
                case 'b':
                    flip_bishops(get_bit(board_x, board_y));
                    break;
                case 'k':
                    flip_kings(get_bit(board_x, board_y));

                    if (white_pieces & get_bit(board_x,board_y))
                    {
                        if (board_x != 4 || board_y != 0)
                            this->castling_rights &= ~(WHITE_KING_SIDE | WHITE_QUEEN_SIDE);
                    }
                    else
                    {
                        if (board_x != 4 || board_y != 7)
                            this->castling_rights &= ~(BLACK_KING_SIDE | BLACK_QUEEN_SIDE);
                    }

                    break;
                case 'q':
                    flip_queens(get_bit(board_x, board_y));
                    break;
                default:
                    return false;
            }

            if (++board_x == 8)
            {
                board_x = 0;
                board_y--;
            }
        }
    }

    if (flipped)
    {
        current_player_pieces = black_pieces;
        //enemy_pieces = white_pieces;
    }
    else
    {
        current_player_pieces = white_pieces;
        //enemy_pieces = black_pieces;
    }

    return true;
}


piece_type board::get_piece(int8_t x, int8_t y) const {
    return get_piece(get_index(x,y));
}
piece_type board::get_piece(uint8_t position) const {

    auto bit = get_bit(position);

    static const piece_type piece_map[6] = {'p', 'r', 'b', 'n','k','q'};
    auto index = get_mask_idx(bit);
    if (index == NO_PIECE) return '.';

    auto piece = piece_map[index];

    return bit & current_player_pieces ? toupper(piece) : piece;
}


bool board::make_move(move const& m) {

    auto src_bit = get_bit(m.src);

    if (!(src_bit & current_player_pieces)) [[unlikely]] {

        if (src_bit & pawns_knights_kings ||
            src_bit & rooks_queens_knights ||
            src_bit & bishops_queens_kings)
            throw std::invalid_argument("Tried moving an enemy piece: " + to_fen() + " " + m.to_uci_move());
        else throw std::invalid_argument("Tried moving an empty square: " + to_fen() + " " + m.to_uci_move());
    }

    auto dst_bit = get_bit(m.dst);

    auto src_mask = get_mask_idx(src_bit);
    auto dst_mask = get_mask_idx(dst_bit);


    auto local_orig_castling_rights = castling_rights;
    auto resolve_castling_rights_for_rook_state_change = [&](uint8_t const& rook_position)
    {
        switch (rook_position)
        {
            case int(square_index::a1):
                castling_rights &= ~WHITE_QUEEN_SIDE;
                break;
            case int(square_index::h1):
                castling_rights &= ~WHITE_KING_SIDE;
                break;
            case int(square_index::a8):
                castling_rights &= ~BLACK_QUEEN_SIDE;
                break;
            case int(square_index::h8):
                castling_rights &= ~BLACK_KING_SIDE;
                break;
        }
    };

    bool en_passant_temp = en_passant_possible;
    en_passant_possible = false;


    auto enemy_pieces = set_difference(all_pieces(), current_player_pieces);


    /*
    auto make_move_impl_2 = [&]<int SrcMask, int DstMask>() {

        if constexpr (SrcMask == PAWNS && DstMask == NO_PIECE) {
            if (en_passant_temp) {

                if ((m.src & 7) != (m.dst & 7)) {
                    auto ep_bit = get_bit(en_passant_x, Color == C_WHITE ? 4 : 3);
                    enemy_pieces ^= ep_bit;
                    flip_pawns(ep_bit);
                }
                en_passant_x = 0;
            }
            if ((m.src + (Color == C_WHITE ? 16 : -16)) == m.dst) {

                auto pawns = set_difference(pawns_knights_kings, ((pawns_knights_kings & rooks_queens_knights) |
                                                                  (pawns_knights_kings & bishops_queens_kings)));

                auto file = m.dst & 7;

                //auto left_passant = (file && (pawns & get_bit(m.dst - 1) & enemy_pieces);
                //auto right_passant = (file != 7) && (pawns & get_bit(m.dst + 1) & enemy_pieces);

                if ((file && (pawns & get_bit(m.dst - 1) & enemy_pieces)) ||
                    ((file != 7) && (pawns & get_bit(m.dst + 1) & enemy_pieces))
                        ) {
                    en_passant_possible = true;
                    en_passant_x = file;
                    //cout << "En passant: " << (int)en_passant_x << endl;
                }
            }
        }
        else if constexpr (SrcMask == KINGS)
        {
            if constexpr(Color == C_WHITE)
                castling_rights &= ~(WHITE_KING_SIDE | WHITE_QUEEN_SIDE);
            else
                castling_rights &= ~(BLACK_QUEEN_SIDE | BLACK_KING_SIDE);


            if (dst_bit == (src_bit << 2))
            {
                 if constexpr (Color == C_WHITE)
                 {
                     flip_rooks(h1);
                     flip_rooks(f1);
                 } else
                 {
                     flip_rooks(h8);
                     flip_rooks(f8);
                 }
            }
            else if (dst_bit == (src_bit >> 2))
            {
                if constexpr (Color == C_WHITE)
                {
                    flip_rooks(a1);
                    flip_rooks(d1);
                } else
                {
                    flip_rooks(a8);
                    flip_rooks(d8);
                }
            }
        }

        if constexpr (DstMask != NO_PIECE) // Capturing move
        {
            flip_piece<DstMask>(dst_bit);
            enemy_pieces ^= dst_bit;
            halfmove_clock = 0;
        }
        else if constexpr(SrcMask != PAWNS) // Non-pawn non-capturing move
        {
            halfmove_clock = (castling_rights == local_orig_castling_rights) ? halfmove_clock + 1 : 0;
        }
        else halfmove_clock = 0; // Pawn move

        if constexpr (SrcMask == PAWNS) {
            //if (dst_bit & pawn_promotion_squares)
            {
                switch(m.promotion)
                {
                    case QUEENS: [[likely]]
                        flip_queens(dst_bit);
                        break;
                    case ROOKS:
                        flip_rooks(dst_bit);
                        break;
                    case BISHOPS:
                        flip_bishops(dst_bit);
                        break;
                    case KNIGHTS:
                        flip_knights(dst_bit);
                        break;

                    case PAWNS: [[likely]]
                        flip_piece<SrcMask>(dst_bit);
                        break;
                        //throw std::invalid_argument("Tried promoting pawn to pawn.");
                    case KINGS: [[unlikely]]
                        throw std::invalid_argument("Tried promoting pawn to king.");
                    default: [[unlikely]]
                        throw std::invalid_argument("Invalid promotion piece type: " + to_string(m.promotion));
                }
            }
            //else flip_piece<SrcMask>(dst_bit);
        }
        else flip_piece<SrcMask>(dst_bit);
    };
*/

    auto do_capture = [&]<bool Symmetric = true>()
    {
        switch (dst_mask)
        {
            case KINGS: throw std::invalid_argument("King take attempted: " + to_fen());
            case QUEENS:
                enemy_pieces ^= dst_bit;
                flip_queens(dst_bit);

                if constexpr (Symmetric)
                    halfmove_clock = 0;
                else return true;
                break;
            case ROOKS:
                resolve_castling_rights_for_rook_state_change(m.dst);
                enemy_pieces ^= dst_bit;
                flip_rooks(dst_bit);

                if constexpr (Symmetric)
                    halfmove_clock = 0;
                else return true;

                break;
            case BISHOPS:
                enemy_pieces ^= dst_bit;
                flip_bishops(dst_bit);

                if constexpr (Symmetric)
                    halfmove_clock = 0;
                else return true;

                break;
            case KNIGHTS:
                enemy_pieces ^= dst_bit;
                flip_knights(dst_bit);
                if constexpr (Symmetric)
                    halfmove_clock = 0;
                else return true;

            break;
            case PAWNS:
                enemy_pieces ^= dst_bit;
                flip_pawns(dst_bit);
                if constexpr (Symmetric)
                    halfmove_clock = 0;
                else return true;
                break;
            default:
                if constexpr (Symmetric)
                    halfmove_clock++;
                else return false;
            break;
        }
        return false;
    };


    switch (src_mask)
    {
        case KINGS:
            flip_kings(src_bit);
            if (!flipped )
                castling_rights &= ~(WHITE_KING_SIDE | WHITE_QUEEN_SIDE);
            else
                castling_rights &= ~(BLACK_QUEEN_SIDE | BLACK_KING_SIDE);

            if (do_capture.template operator()<false>())
            {
                halfmove_clock = 0;
            } else
            {


                if (dst_bit == (src_bit << 2))
                {
                    if (!flipped)
                    {
                        flip_rooks(h1);
                        flip_rooks(f1);
                    } else
                    {
                        flip_rooks(h8);
                        flip_rooks(f8);
                    }
                }
                else if (dst_bit == (src_bit >> 2))
                {
                    if (!flipped)
                    {
                        flip_rooks(a1);
                        flip_rooks(d1);
                    } else
                    {
                        flip_rooks(a8);
                        flip_rooks(d8);
                    }
                }


                if (castling_rights != local_orig_castling_rights)
                    halfmove_clock = 0;
                else halfmove_clock++;
            }

            flip_kings(dst_bit);
            break;
        case QUEENS:
            flip_queens(src_bit);
            do_capture();
            flip_queens(dst_bit);
            break;
        case ROOKS:
            flip_rooks(src_bit);
            do_capture();
            flip_rooks(dst_bit);
            resolve_castling_rights_for_rook_state_change(m.src);
            break;
        case BISHOPS:
            flip_bishops(src_bit);
            do_capture();
            flip_bishops(dst_bit);
            break;
        case KNIGHTS:
            flip_knights(src_bit);
            do_capture();
            flip_knights(dst_bit);
            break;
        case PAWNS:
            flip_pawns(src_bit);
            halfmove_clock = 0;

            if (!do_capture.template operator()<false>())
            {
                if (en_passant_temp) {

                    if ((m.src & 7) != (m.dst & 7)) {
                        auto ep_bit = get_bit(en_passant_x, flipped == C_WHITE ? 4 : 3);
                        enemy_pieces ^= ep_bit;
                        flip_pawns(ep_bit);
                    }
                    en_passant_x = 0;
                }
                if ((m.src + (flipped == C_WHITE ? 16 : -16)) == m.dst) {

                    auto pawns = set_difference(pawns_knights_kings, ((pawns_knights_kings & rooks_queens_knights) |
                                                                      (pawns_knights_kings & bishops_queens_kings)));

                    auto file = m.dst & 7;

                    //auto left_passant = (file && (pawns & get_bit(m.dst - 1) & enemy_pieces);
                    //auto right_passant = (file != 7) && (pawns & get_bit(m.dst + 1) & enemy_pieces);

                    if ((file && (pawns & get_bit(m.dst - 1) & enemy_pieces)) ||
                        ((file != 7) && (pawns & get_bit(m.dst + 1) & enemy_pieces))
                            ) {
                        en_passant_possible = true;
                        en_passant_x = file;
                        //cout << "En passant: " << (int)en_passant_x << endl;
                    }
                }

            }

            switch(m.promotion)
            {
                case QUEENS:
                    flip_queens(dst_bit);
                    break;
                case ROOKS:
                    flip_rooks(dst_bit);
                    break;
                case BISHOPS:
                    flip_bishops(dst_bit);
                    break;
                case KNIGHTS:
                    flip_knights(dst_bit);
                    break;

                case PAWNS: [[likely]]
                    flip_pawns(dst_bit);
                    break;
                    //throw std::invalid_argument("Tried promoting pawn to pawn.");
                case KINGS: [[unlikely]]
                            throw std::invalid_argument("Tried promoting pawn to king.");
                default: [[unlikely]]
                            throw std::invalid_argument("Invalid promotion piece type: " + to_string(m.promotion));
            }

            break;
        default: throw std::invalid_argument("Invalid source square.");
    }


    //fullmove_number += flipped;
    current_player_pieces = enemy_pieces;
    flipped = !flipped;

    return true;
}


// TODO: These comparisons are probably inefficient
bool board::operator==(const board &other) const {

    return this->halfmove_clock == other.halfmove_clock && this->has_repeated == other.has_repeated && tfr_compare(other);
}

bool board::tfr_compare(const board &other) const {

    return this->current_player_pieces == other.current_player_pieces &&
           this->rooks_queens_knights == other.rooks_queens_knights &&
           this->bishops_queens_kings == other.bishops_queens_kings &&
           this->pawns_knights_kings == other.pawns_knights_kings &&
           this->en_passant_possible == other.en_passant_possible &&
           this->en_passant_x == other.en_passant_x &&
           this->castling_rights == other.castling_rights &&
           this->flipped == other.flipped;
}

/**
* Evaluation subroutine suitable for lchess engines.
* -------------------------------------------------
* Piece codes are
*     wking=1, wqueen=2, wrook=3, wbishop= 4, wknight= 5, wpawn= 6,
*     bking=7, bqueen=8, brook=9, bbishop=10, bknight=11, bpawn=12,
* Square are
*     A1=0, B1=1 ... H8=63
* Input format:
*     piece[0] is white king, square[0] is its location
*     piece[1] is black king, square[1] is its location
*     ..
*     piece[x], square[x] can be in any order
*     ..
*     piece[n+1] is set to 0 to represent end of array
*/

#ifdef WITH_NNUE
float board::nnue_eval() const
{
    int pieces[33];
    int squares[33];

    auto pieces_src = disentangle_pieces();

    //cout << to_fen() << endl;
    auto enemy_pieces = get_enemy_pieces();
    //memset(pieces, 0, sizeof(int) * 33);
    //memset(squares, 0, sizeof(int) * 33);
    uint64_t white_pieces, black_pieces;
    if (flipped)
    {
        white_pieces = enemy_pieces;
        black_pieces = current_player_pieces;
    }
    else
    {
        white_pieces = current_player_pieces;
        black_pieces = enemy_pieces;
    }

    int pindex = 2;

    auto iterate_bits = [&](uint64_t bitmask, int value)
    {
        for (auto bit = poplsb(bitmask); bit; bit = poplsb(bitmask))
        {
            pieces[pindex] = value;
            squares[pindex++] = __builtin_ctzll(bit);
        }
    };


    pieces[0] = 1;
    pieces[1] = 7;
    squares[0] = __builtin_ctzll(pieces_src.kings & white_pieces);
    squares[1] = __builtin_ctzll(pieces_src.kings & black_pieces);

    iterate_bits(pieces_src.queens & white_pieces, 2);
    iterate_bits(pieces_src.rooks & white_pieces, 3);
    iterate_bits(pieces_src.bishops & white_pieces, 4);
    iterate_bits(pieces_src.knights & white_pieces, 5);
    iterate_bits(pieces_src.pawns & white_pieces, 6);


    iterate_bits(pieces_src.queens & black_pieces, 8);
    iterate_bits(pieces_src.rooks & black_pieces, 9);
    iterate_bits(pieces_src.bishops & black_pieces, 10);
    iterate_bits(pieces_src.knights & black_pieces, 11);
    iterate_bits(pieces_src.pawns & black_pieces, 12);

    if (pindex > 32)
    {
        throw std::logic_error("Board has too many pieces for NNUE evaluation: " + to_fen());
    }

    pieces[pindex] = 0;

    return nnue_evaluate(flipped, pieces, squares);
}

#endif

void chess::board::board_orientation_to_current_player()
{
    if (!flipped) return;

    flip_board();
}

void chess::board::flip_board()
{
    current_player_pieces = reverse_bytes(current_player_pieces);
    bishops_queens_kings = reverse_bytes(bishops_queens_kings);
    rooks_queens_knights = reverse_bytes(rooks_queens_knights);
    pawns_knights_kings = reverse_bytes(pawns_knights_kings);

    castling_rights = castling_rights << 2 | castling_rights >> 2;
    flipped = !flipped;
};


chess::move board::tb_probe_dtz() const
{
    // TODO:
    throw std::logic_error("tb_probe_dtz not implemented.");
    /*
    auto piece_union = all_pieces();

    uint64_t white, black;

    flipped ?
    (black = current_player_pieces, white = set_difference(piece_union, black)) :
    (white = current_player_pieces, black = set_difference(piece_union, white));


    auto pieces = disentangle_pieces();

    ::tb_probe_root_dtz(white, black, pieces.kings, pieces.queens, pieces.rooks, pieces.bishops, pieces.knights,
                        pieces.pawns, halfmove_clock, castling_rights, )
    */
}



game_state board::tb_probe_wdl() const {

    // TODO: Re-merge this into generate_moves as doing it separately is somewhat inefficient.
    auto piece_union = all_pieces();
    if (popcount(piece_union) <= tb_maxcount)
    {
        auto pieces = disentangle_pieces();
        auto enemy_pieces = set_difference(piece_union, current_player_pieces);
        return tb_probe_wdl(pieces, enemy_pieces);
    }

    return game_state::playing;
}


game_state board::tb_probe_wdl(disentangled_pieces const& pieces, uint64_t const& enemy_pieces) const
{
    //return playing;

    int tb_value = (!flipped) ?
                   ::tb_probe_wdl(current_player_pieces, enemy_pieces, pieces.kings, pieces.queens,
                                  pieces.rooks, pieces.bishops, pieces.knights, pieces.pawns,
                                  halfmove_clock, castling_rights,  en_passant_possible ?
                                                                    get_index(en_passant_x, 4) : 0, true)
                                      :
                   ::tb_probe_wdl(enemy_pieces, current_player_pieces, pieces.kings, pieces.queens,
                                  pieces.rooks, pieces.bishops, pieces.knights,pieces.pawns,
                                  halfmove_clock, castling_rights, en_passant_possible ?
                                                                   get_index(en_passant_x, 3) : 0, false);

    switch(tb_value)
    {
        case TB_LOSS: return tablebase_loss;
        case TB_WIN: return tablebase_win;

        case TB_DRAW:
            return tablebase_draw;
        case TB_BLESSED_LOSS:
            return tablebase_blessed_loss;
        case TB_CURSED_WIN:
            return tablebase_cursed_win;

        default:
            return game_state::playing;
            cout << to_fen() << endl;
            throw std::logic_error("Tablebase lookup returned an unrecognized value.");
    }
}

uint64_t chess::board::hash() const
{
    return XXH64(this, sizeof(chess::board), 3498508396312845423);
}


string chess::board::move_to_algebraic_notation(chess::move const& move)
{
    auto src_piece = get_piece(move.src);
    auto dst_piece = get_piece(move.dst);

    if (src_piece == '.')
        throw std::logic_error("Invalid move.");

    string result;
    result += src_piece;

    if (dst_piece != '.') result += 'x';

    char file = 'a' + (move.dst & 7);
    char rank = '1' + (move.dst >> 3);

    result += file;
    result += rank;

    if (move.promotion != PAWNS)
    {
        result += " = ";
        result += move.promotion == QUEENS ? 'Q' : move.promotion == ROOKS ? 'R' : move.promotion == BISHOPS ? 'B' : 'N';
    }
    return result;
}

