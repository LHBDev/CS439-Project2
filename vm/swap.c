#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "devices/block.h"
#include "threads/synch.h"
#include "threads/vaddr.h"

struct bitmap *swap_table;
struct block *swap_block;
struct lock swap_lock;

//Header code and above code driven by both Ruben and Siva

//P3 Added Code - Initialize the swap block, the swap table
//and the swap lock

//Siva started driving
void
swap_init (void)
{
	swap_block = block_get_role(BLOCK_SWAP);
	swap_table = bitmap_create(block_size(swap_block));
	lock_init(&swap_lock);
}
//Siva stopped driving

//P3 Added Code - Find the first available swap index in the swap table
//and write to that spot in the table. The write buffer is simply obtained
//from the frame addr stored in the frame entry.

//Ruben started driving
void
swap_out (void *vaddr)
{
	int i, swp_ind;
	struct frame *frame = lookup_frame(vaddr);
	struct page *spt_entry = lookup_page(vaddr, frame->owner);
	void *frame_addr = frame->frame_addr;

	swp_ind = bitmap_scan_and_flip(swap_table, 0, PGSIZE / BLOCK_SECTOR_SIZE, false);
	spt_entry->swp = swp_ind;
	
	lock_acquire(&swap_lock);
	for(i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
		block_write(swap_block, swp_ind + i, frame_addr + (BLOCK_SECTOR_SIZE * i));
	lock_release(&swap_lock);
}
//Ruben stopped driving

//P3 Added Code - Given the swap index, we read from that location of swap
//and put the data into the buffer (indicated by kpage)

//Siva started driving
void *
swap_in (void *vaddr, int swp_ind)
{
	void *kpage;
	int i;

	lock_acquire(&swap_lock);
	kpage = obtain_frame(vaddr, true);
	bitmap_reset(swap_table, swp_ind);
	for(i = 0; i < PGSIZE / BLOCK_SECTOR_SIZE; i++)
		block_read(swap_block, swp_ind + i, kpage + (BLOCK_SECTOR_SIZE * i));
	bitmap_set_multiple (swap_table, swp_ind, PGSIZE / BLOCK_SECTOR_SIZE, false);
	lock_release(&swap_lock);

	return kpage;
}
//Siva stopped driving
