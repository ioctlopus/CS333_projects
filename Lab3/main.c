#include "primesMT.h"


//Bits per block:
#ifndef BPB
# define BPB 32
#endif // BPB

//Using globals since the entire program is only a few files and I'd rather not take a trillion args for every function:
static uint16_t num_threads = 1;
static uint64_t upper_num = 10240;
static bool is_verbose = false;
BitBlock_t* bitarr = NULL;

//Main is...well, main():
int
main(int argc, char** argv){
    int opt = 1;
    const char* usage = "Usage: ./primesMT [OPTIONS]\n\n -t\t\tSpecify number of threads\n -u\t\tSpecify upper limit of numbers to check\n -v\t\tVerbose mode\n -h\t\tDisplay this message and exit\n";
    pthread_t* thrds = NULL;
    long tid = 0;

    //Parse command line arguments and set values accordingly:
    while((opt = getopt(argc, argv, "t:u:h:v")) != -1){
        switch(opt){
            case 't':
                num_threads = atoi(optarg);
                break;
            case 'u':
                upper_num = atoi(optarg);
                break;
            case 'h':
                printf("%s", usage);
                break;
            case 'v':
                is_verbose = true;
                break;
            default:
                printf("%s", usage);
                break;
        }
    }

    thrds = malloc(sizeof(pthread_t) * num_threads);
    bitarr = alloc_arr(upper_num, BPB);

    for(size_t i = 0; i < num_threads; ++i, tid++){
        pthread_create(&thrds[i], NULL, thread_func, (void*)tid);
    }

    for(size_t i = 0; i < num_threads; ++i){
        pthread_join(thrds[i], NULL);
    }
    
    printf("2\n");    
    for(uint64_t i = 3; i <= upper_num; i += 2){
        if((bitarr[i / BPB].bits & ((uint32_t)0x1 << (i % 32))) == 0){
            printf("%lu\n", i);
        }

    }

    free(thrds);
    free_arr(bitarr, upper_num / BPB + 1);

    return 0;
}

//Return a pointer to a BitBlock array with all bits initialized to 0. Size is determined based on the
//size of the bits member and the upper number:
static inline BitBlock_t*
alloc_arr(uint64_t upnum, uint16_t bitsperblock){
    BitBlock_t* ret = NULL;

    uint64_t how_many = upnum / bitsperblock + 1;

    ret = malloc(how_many * sizeof(BitBlock_t));

    if(is_verbose){
        fprintf(stderr, "Allocating and initializing %lu blocks...\n", how_many);
    }

    for(int i = 0; i < how_many; ++i){
        ret[i].bits = 0x0;
        pthread_mutex_init(&ret[i].mutex, NULL);
    }

    return ret;
}

//De-allocate the array of bitblocks:
static inline void
free_arr(BitBlock_t* ar, uint64_t size){

    for(int i = 0; i < size; ++i){
        pthread_mutex_destroy(&ar[i].mutex);
    }
    free(ar);

    if(is_verbose){
        fprintf(stderr, "Freeing array...\n");
    }
}

//Perform the sieve:
static inline void 
sieve(BitBlock_t* array, uint64_t size, long tid){
    if(is_verbose){
        fprintf(stderr, "Initializing sieve with %hu thread(s)...\n", num_threads);
    }
    
    //First assign threads to rows:
    for(uint64_t i = 3 + tid * 2; i <= (uint64_t)sqrt(upper_num); i += 2 * num_threads){
        if(is_verbose)
            fprintf(stderr, "Thread %ld Candidate %lu\n", tid, i);
        //Determine primality and set multiples, locking the setting of the bits:
        if(((bitarr[i / BPB].bits & ((uint32_t)0x1 << (i % 32))) == 0)){
            for(uint64_t j = 2 * i; j <= upper_num; j += i){
                pthread_mutex_lock(&array[j / BPB].mutex);
                array[j / BPB].bits |= ((uint32_t)0x1 << (j % 32));
                pthread_mutex_unlock(&array[j / BPB].mutex);
            }
        }
    }
    
}

//Driver function to pass to pthread_create():
void*
thread_func(void* arg){
    long tid = (long)arg;
    if(is_verbose)
        fprintf(stderr, "This is thread %ld\n", tid);
    sieve(bitarr, upper_num / BPB + 1, tid);
    pthread_exit(EXIT_SUCCESS);
}
