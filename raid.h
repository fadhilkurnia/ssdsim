#include "initialize.h"

#define RAID_REQUEST_QUEUE_CAPACITY 10
#define RAID_0 0
#define RAID_5 5
#define NDISK 4

struct raid_info* initialize_raid(struct raid_info*, struct user_args*);
void free_raid_ssd_and_tracefile(struct raid_info*);
int64_t raid_find_nearest_event(struct raid_info*);

int simulate_raid(struct user_args *);
struct raid_info* simulate_raid0(struct raid_info*);
struct raid_info* simulate_raid5(struct raid_info*);

struct raid_info {
    unsigned int raid_type;
    unsigned int num_disk;

    char tracefilename[80];
    FILE * tracefile;

    int64_t current_time;
    unsigned int max_lsn;
    struct request *request_queue;
    struct request *request_tail;
    int request_queue_length;

    struct ssd_info **connected_ssd;
};