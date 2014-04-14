#include <hash.h>
#include <debug.h>
#include <stdint.h>

//A frame entry in the frame table. Item descriptions in Siva's design doc
struct frame
{
	struct thread *owner;
	void *pte;
	void *frame_addr;
	bool remember_dirty;
	bool pinned;
	struct hash_elem hash_elem;
};

//A lock for frame table operations
struct lock ft_lock;

/* Hash Functions for Frame Tables */
unsigned frame_hash(const struct hash_elem *, void * UNUSED);
bool frame_less(const struct hash_elem *, const struct hash_elem *,
           void * UNUSED);

/* Frame Table functions */
void frame_init(void);
void * obtain_frame (void *, bool);
void free_frame (void *, void *);
struct frame * lookup_frame (void *);
void * evict_frame (bool);
void free_eviction (struct frame *, void *);
void unpin(struct frame *);
void pin(struct frame *);
