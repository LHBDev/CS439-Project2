#include <hash.h>
#include <debug.h>
#include <stdint.h>

struct frame
{
	struct hash_elem hash_elem;
	struct thread *owner;
	uint32_t *pte;
	uint32_t frame_addr;
};

/* Hash Functions for Frame Tables */
unsigned frame_hash(const struct hash_elem *, void * UNUSED);
bool frame_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);

/* Frame Table functions */
void frame_init(void);
void add_frame (uint32_t, uint32_t);
struct frame * lookup_frame (uint32_t);
void evict_frame (void);
