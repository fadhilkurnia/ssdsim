#include "initialize.h"

#define RAID_REQUEST_QUEUE_CAPACITY 10
#define RAID_STRIPE_SIZE_BYTE 65536
#define RAID_BLOCK_SIZE_BYTE 512
#define RAID_0 0
#define RAID_5 5
#define NDISK 3

struct raid_info* initialize_raid(struct raid_info*, struct user_args*);
void free_raid_ssd_and_tracefile(struct raid_info*);
int64_t raid_find_nearest_event(struct raid_info*);

int simulate_raid(struct user_args *);
struct raid_info* simulate_raid0(struct raid_info*);
struct raid_info* simulate_raid5(struct raid_info*);

int raid_distrubute_request(struct raid_info*, int64_t, unsigned int, unsigned int, unsigned int);

// reference: https://www.snia.org/sites/default/files/SNIA_DDF_Technical_Position_v2.0.pdf
// block and sector is interchangable here
struct raid_info {
    unsigned int raid_type;
    unsigned int num_disk;
    unsigned int block_size;        // block size in bytes
    unsigned int stripe_size;       // stripe size in bytes
    unsigned int stripe_size_block; // stripe size in block
    unsigned int strip_size_block;  // strip size in block

    char tracefilename[80];
    FILE * tracefile;

    int64_t current_time;
    unsigned int max_lsn;
    struct request *request_queue;
    struct request *request_tail;
    unsigned int request_queue_length;

    struct ssd_info **connected_ssd;
};