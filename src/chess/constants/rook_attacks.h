#ifndef rook_ATTACKS_HEADER
#define rook_ATTACKS_HEADER
extern const uint64_t _rook_masks[64];
extern const uint64_t rook_magics[64];
extern const uint64_t rook_shifts[64];
extern const uint64_t* rook_tables[64];

uint64_t get_rook_attacks(uint64_t position, int piece_idx);
#endif
