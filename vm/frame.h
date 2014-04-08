#include <hash.h>
#include <debug.h>
#include <stdint.h>

struct frame
{
	struct thread *owner;
	uint8_t *pte;
	void *frame;
	struct hash_elem hash_elem;
};

/* Hash Functions for Frame Tables */
unsigned frame_hash(const struct hash_elem *, void * UNUSED);
bool frame_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);

/* Frame Table functions */
void frame_init(void);
void * obtain_frame (uint8_t *);
void free_frame (uint8_t *);
struct frame * lookup_frame (uint8_t *);
void evict_frame (void);
