#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

struct bitmap *swap_table;
struct block *swap_block;
struct lock swap_lock;

void
swap_init (void)
{
	swap_block = block_get_role(BLOCK_SWAP);
	swap_table = bitmap_create(block_size(swap_block));
	lock_init(&swap_lock);
}

void
swap_out (uint8_t *vaddr)
{
	int i, swp_ind;
	struct frame *frame = lookup_frame(vaddr);
	struct page *spt_entry = lookup_page(vaddr);

	swp_ind = bitmap_scan_and_flip(swap_table, 0, PGSIZE / BLOCK_SECTOR_SIZE, false);
	spt_entry->swp = swp_ind;
	
	lock_acquire(&swap_lock);
	for(i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
		block_write(swap_block, swp_ind + i, frame + (BLOCK_SECTOR_SIZE * i));
	lock_release(&swap_lock);
}

void *
swap_in (uint8_t *vaddr, int swp_ind)
{
	void *palloc_frame;
	int i;

	lock_acquire(&swap_lock);
	palloc_frame = obtain_frame(vaddr, true);
	bitmap_reset(swap_table, swp_ind);
	for(i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
		block_read(swap_block, swp_ind + i, palloc_frame + (BLOCK_SECTOR_SIZE * i));
	lock_release(&swap_lock);

	return palloc_frame;
}
