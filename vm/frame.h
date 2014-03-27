#include <hash.h>
#include <debug.h>
#include <stdint.h>

unsigned frame_hash(const struct hash_elem *, void * UNUSED);
bool frame_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);
void frame_init(void);
