// Ben Rimedio (Template from Jesse Chaney)
// brimedio@pdx.edu

#include "vikalloc.h"

#define BLOCK_SIZE (sizeof(mem_block_t))
#define BLOCK_DATA(__curr) (((void *) __curr) + (BLOCK_SIZE))
#define DATA_BLOCK(__data) ((mem_block_t *) (__data - BLOCK_SIZE))

#define IS_FREE(__curr) ((__curr -> size) == 0)

#define PTR "0x%07lx"
#define PTR_T PTR "\t"

static mem_block_t *block_list_head = NULL;
static mem_block_t *block_list_tail = NULL;
static void *low_water_mark = NULL;
static void *high_water_mark = NULL;
// only used in next-fit algorithm
static mem_block_t *prev_fit = NULL;

static uint8_t isVerbose = FALSE;
static vikalloc_fit_algorithm_t fit_algorithm = FIRST_FIT;
static FILE *vikalloc_log_stream = NULL;

static void init_streams(void) __attribute__((constructor));

static size_t min_sbrk_size = MIN_SBRK_SIZE;

static void 
init_streams(void)
{
    vikalloc_log_stream = stderr;
}

size_t
vikalloc_set_min(size_t size)
{
    if (0 == size) {
        // just return the current value
        return min_sbrk_size;
    }
    if (size < (BLOCK_SIZE + BLOCK_SIZE)) {
        // In the event that it is set to something silly small.
        size = MAX(BLOCK_SIZE + BLOCK_SIZE, SILLY_SBRK_SIZE);
    }
    min_sbrk_size = size;

    return min_sbrk_size;
}

void 
vikalloc_set_algorithm(vikalloc_fit_algorithm_t algorithm)
{
    fit_algorithm = algorithm;
    if (isVerbose) {
        switch (algorithm) {
        case FIRST_FIT:
            fprintf(vikalloc_log_stream, "** First fit selected\n");
            break;
        case BEST_FIT:
            fprintf(vikalloc_log_stream, "** Best fit selected\n");
            break;
        case WORST_FIT:
            fprintf(vikalloc_log_stream, "** Worst fit selected\n");
            break;
        case NEXT_FIT:
            fprintf(vikalloc_log_stream, "** Next fit selected\n");
            break;
        default:
            fprintf(vikalloc_log_stream, "** Algorithm not recognized %d\n"
                    , algorithm);
            fit_algorithm = FIRST_FIT;
            break;
        }
    }
}

void
vikalloc_set_verbose(uint8_t verbosity)
{
    isVerbose = verbosity;
    if (isVerbose) {
        fprintf(vikalloc_log_stream, "Verbose enabled\n");
    }
}

void
vikalloc_set_log(FILE *stream)
{
    vikalloc_log_stream = stream;
}


void *
vikalloc(size_t size)
{
    mem_block_t *curr = NULL;
    size_t sbrk_size = ((((BLOCK_SIZE + size) / min_sbrk_size)) + 1) * min_sbrk_size;

    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry: size = %lu\n"
                , __LINE__, __FUNCTION__, size);
    }
    //Size 0:
    if(!size)
        return NULL;
    //List empty:
    if(!block_list_head){
        curr = block_list_head = low_water_mark = block_list_tail = (mem_block_t*)sbrk(sbrk_size);
        high_water_mark = low_water_mark + sbrk_size;
        block_list_head->prev = block_list_head->next = NULL;
        curr->size = size;
        curr->capacity = sbrk_size - BLOCK_SIZE;
    }
    //List not empty:
    else{
        curr = block_list_head;
        //Iterate through the free list looking for available blocks:
        while(curr){
            //A free block is available for reuse:
            if(IS_FREE(curr) && curr->capacity >= size){
                curr->size = size;
                break;
            }
            //A block of sufficient size is available. Split:
            else if(curr->capacity - curr->size >= size + BLOCK_SIZE){
                mem_block_t *new_block = (void*)curr + curr->size + BLOCK_SIZE;
                new_block->size = size;
                new_block->capacity = curr->capacity - curr->size - BLOCK_SIZE;
                curr->capacity = curr->size;
                //We're in the middle of the list:
                if(curr->next){
                    new_block->next = curr->next;
                    new_block->prev = curr;
                    curr->next->prev = new_block;
                    curr->next = new_block;
                    curr = new_block;
                }
                //We're at the end of the list:
                else{
                    new_block->prev = curr;
                    new_block->next = NULL;
                    curr->next = new_block;
                    block_list_tail = new_block;
                    curr = new_block;
                }
                break;
            }
            //Keep going:
            else{
                curr = curr->next;
            }
        }
        //If we reached the end of the free list, grow the heap:
        if(!curr){
            curr = (mem_block_t*)sbrk(sbrk_size);
            block_list_tail->next = curr;
            curr->size = size;
            curr->capacity = sbrk_size - BLOCK_SIZE;
            curr->next = NULL;
            curr->prev = block_list_tail;
            block_list_tail = curr;
            high_water_mark += sbrk_size;
        }

    }
        
    return BLOCK_DATA(curr);
}

void 
vikfree(void *ptr)
{
    mem_block_t *block;
    
    if(isVerbose){
        fprintf(vikalloc_log_stream, "Block is already free: ptr = " PTR "\n"
                , (long)(ptr - low_water_mark));
    }
/*    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry\n"
                , __LINE__, __FUNCTION__);
    }
 */
    //Check for NULL and then find the block of interest, setting its size to 0:
    if(!ptr)
        return;
    block = (void*)ptr - BLOCK_SIZE;
    block->size = 0;    

    //Coalesce up:
    if(block && block->next){
        if(IS_FREE(block->next)){
            block->capacity = BLOCK_SIZE + block->capacity + block->next->capacity;
            if(!block->next->next){
                block->next = NULL;
                block_list_tail = block;
            }
            else{
                block->next->next->prev = block;
                block->next = block->next->next;
            }
        }
    }

    //Coalesce down:
    if(block && block->prev && IS_FREE(block->prev)){
        vikfree((void*)block->prev + BLOCK_SIZE);
    }

    return;
}

///////////////

void 
vikalloc_reset(void)
{
    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry\n"
                , __LINE__, __FUNCTION__);
    }

    if (low_water_mark != NULL) {
        if (isVerbose) {
            fprintf(vikalloc_log_stream, "*** Resetting all vikalloc space ***\n");
        }

    }
    
    //Return the heap and free list to their original states.    
    brk(low_water_mark);
    high_water_mark = low_water_mark;
    block_list_head = block_list_tail = NULL;
}

void *
vikcalloc(size_t nmemb, size_t size)
{
    void *ptr = NULL;
    
    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry\n"
                , __LINE__, __FUNCTION__);
    }
    ptr = vikalloc(nmemb * size);
    ptr = memset(ptr, 0, nmemb * size);

    return ptr;
}

void *
vikrealloc(void *ptr, size_t size)
{
    mem_block_t* block = (mem_block_t*)(ptr - BLOCK_SIZE);

    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry\n"
                , __LINE__, __FUNCTION__);
    }
    //NULL check:
    if(!ptr)
        return vikalloc(size);

    //Is the current block capable of handling the new size? If so, adjust size:
    if(block->capacity >= size){
        block->size = size;
	    return BLOCK_DATA(block);
    }

    //Otherwise, we have to allocate a new chunk and copy the data:
    else{
        void* new_block = vikalloc(size);
	    memcpy(new_block, ptr, block->size);
    	vikfree(ptr);
	    return BLOCK_DATA(new_block);
    }

}

void *
vikstrdup(const char *s)
{
    void *ptr = NULL;
    size_t length = 0;
    void* begin = NULL;

    if (isVerbose) {
        fprintf(vikalloc_log_stream, ">> %d: %s entry\n"
                , __LINE__, __FUNCTION__);
    }

    //Really dumb strlen() replacement:
    for(char* p = (char*)s; p && *p != '\0'; ++p)
        ++length;
    ++length;

    //Allocate and copy:
    begin = vikalloc(length);
    ptr = memcpy(begin, s, length);

    return ptr;
}

#include "vikalloc_dump.c"
