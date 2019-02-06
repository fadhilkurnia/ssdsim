#include "initialize.h"

#define RAID_REQUEST_QUEUE_CAPACITY 10
#define RAID_0 0
#define RAID_5 5
#define NDISK 4

struct raid_info* initialize_raid(struct raid_info*, struct user_args*);
void free_raid_ssd(struct raid_info*);

int simulate_raid(struct user_args *);
struct raid_info* simulate_raid0(struct raid_info*);
struct raid_info* simulate_raid5(struct raid_info*);

struct raid_info {
    unsigned int raid_type;
    unsigned int num_disk;

    char tracefilename[80];
    FILE * tracefile;

    struct request *request_queue;
    struct request *request_tail;

    struct ssd_info **connected_ssd;
};