#define RAID_REQUEST_QUEUE_CAPACITY 20
#define RAID_STRIPE_SIZE_BYTE 65536
#define RAID_BLOCK_SIZE_BYTE 512
#define RAID_0 0
#define RAID_5 5
#define NDISK 3

#define SSD_SIMULATION_STOP 0
#define SSD_SIMULATION_CONTINUE 1
#define RAID_SIMULATION_ERROR 100
#define RAID_SIMULATION_FINISH 200
#define R_DIST_ERR 1
#define R_DIST_SUCCESS 0

#define R_SR_WAIT_PARITY 0
#define R_SR_PENDING 1
#define R_SR_PROCESS 2
#define R_SR_COMPLETE 3
// #define DEBUGRAID

#define RAID_SSD_LATENCY_NS 300000 // 300us
#define RAID5_PARITY_CALC_TIME_NS 80000 // 80us
#define WRITE_RAID 3

struct raid_info* initialize_raid(struct raid_info*, struct user_args*);
struct raid_request* initialize_raid_request(struct raid_request* raid_req, int64_t req_incoming_time, unsigned int req_lsn, unsigned int req_size, unsigned int req_operation);
struct raid_sub_request* initialize_raid_sub_request(struct raid_sub_request* raid_subreq, struct raid_request* raid_req, unsigned int disk_id, unsigned int stripe_id, unsigned int strip_id, unsigned int strip_offset, unsigned int lsn, unsigned int size, unsigned int operation);
void free_raid_ssd_and_tracefile(struct raid_info*);
int64_t raid_find_nearest_event(struct raid_info*);

int simulate_raid(struct user_args *);
struct raid_info* simulate_raid0(struct raid_info*);
struct raid_info* simulate_raid5(struct raid_info*);
void raid_simulate_ssd(struct raid_info*, int);
int raid_ssd_interface(struct ssd_info*, struct raid_sub_request*);
int raid_ssd_get_requests(int disk_id, struct ssd_info *ssd, struct raid_info *raid);
void raid_ssd_trace_output(struct ssd_info*);

int raid_distribute_request(struct raid_info*, int64_t, unsigned int, unsigned int, unsigned int);
int raid_clear_completed_request(struct raid_info*);

void raid5_finish_parity_calculation(struct raid_info*);
void ssd_delete_request_from_queue(struct ssd_info*, struct request*);
void raid_print_req_queue(struct raid_info*);

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
    char logfilename[80];
    FILE * tracefile;
    FILE * logfile;

    int64_t current_time;
    unsigned int max_lsn;
    struct raid_request *request_queue;
    struct raid_request *request_tail;
    unsigned int request_queue_length;

    struct ssd_info **connected_ssd;

    // additional var for gc scheduling
    struct gclock_raid_info *gclock;
};

struct gclock_raid_info {
    int64_t begin_time;
    int64_t end_time;
    int is_available;
    int holder_id;
    int gc_count;
};

// Request in RAID level
struct raid_request {
    unsigned int lsn;
    unsigned int size;
    unsigned int operation;

    int64_t begin_time;
    int64_t response_time;

    struct raid_sub_request *subs;
    struct raid_request *prev_node;
    struct raid_request *next_node;
};

// Request in SSD level, one raid_request can be separated into multiple raid_sub_request
struct raid_sub_request {
    unsigned int disk_id;
    unsigned int stripe_id;
    unsigned int strip_id;
    unsigned int strip_offset;

    int64_t begin_time;
    int64_t complete_time;
    unsigned int current_state;

    unsigned int lsn;
    unsigned int size;
    unsigned int operation;

    struct raid_sub_request *next_node;
};