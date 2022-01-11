#ifndef bishop_ATTACKS_HEADER
#define bishop_ATTACKS_HEADER

extern uint64_t _bishop_masks[64];
extern const uint64_t bishop_magics[64];
extern const uint64_t bishop_shifts[64];
extern const uint64_t* bishop_tables[64];

uint64_t get_bishop_attacks(uint64_t position, int piece_idx);
#endif
