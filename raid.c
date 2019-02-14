#include <stdio.h>
#include <unistd.h>
#include "pagemap.h"
#include "raid.h"
#include "ssd.h"

// initialize_raid function initializes raid struct and all ssd connected to the struct
struct raid_info* initialize_raid(struct raid_info* raid, struct user_args* uargs) {
    struct ssd_info *ssd_pointer;
    unsigned int max_lsn_per_disk;

    raid->raid_type = uargs->raid_type;
    raid->num_disk = uargs->num_disk;
    strcpy(raid->tracefilename, uargs->trace_filename);
    raid->tracefile = fopen(raid->tracefilename, "r");

    raid->connected_ssd=malloc(sizeof(struct ssd_info) * raid->num_disk);
    alloc_assert(raid->connected_ssd, "connected ssd");
    for (int i = 0; i < raid->num_disk; i++) {
        ssd_pointer=(struct ssd_info*) malloc(sizeof(struct ssd_info));
        alloc_assert(ssd_pointer, "one of connected ssd");
        memset(ssd_pointer,0,sizeof(struct ssd_info));
        raid->connected_ssd[i] = ssd_pointer;
    }

    // initialize each ssd in raid
    for (int i = 0; i < raid->num_disk; i++) {
        printf("Initializing disk%d\n", i);

        raid->connected_ssd[i] = initialize_ssd(raid->connected_ssd[i], uargs);
        raid->connected_ssd[i] = initiation(raid->connected_ssd[i]);
        raid->connected_ssd[i] = make_aged(raid->connected_ssd[i]);
        raid->connected_ssd[i] = pre_process_page(raid->connected_ssd[i]);
    }

    // set raid block size and stripe size
    raid->block_size = ssd_pointer->parameter->subpage_capacity;
    raid->stripe_size = RAID_STRIPE_SIZE_BYTE;
    raid->stripe_size_block = raid->stripe_size / raid->block_size;
    raid->strip_size_block = raid->stripe_size_block / raid->num_disk;
    if (raid->stripe_size_block == 0 || raid->strip_size_block == 0) {
        printf("--> %u %u %u %u\n", raid->block_size, raid->stripe_size, raid->stripe_size_block, raid->strip_size_block);
        printf("Error! wrong stripe size or strip size!\n");
        exit(1);
    }

    // calculating the maximum lsn for the raid
    max_lsn_per_disk = (unsigned int) ssd_pointer->parameter->subpage_page * ssd_pointer->parameter->page_block * ssd_pointer->parameter->block_plane * ssd_pointer->parameter->plane_die * ssd_pointer->parameter->die_chip * ssd_pointer->parameter->chip_num * (1 - ssd_pointer->parameter->overprovide );
    raid->max_lsn = max_lsn_per_disk * raid->num_disk;
    if (raid->raid_type == RAID_5)
        raid->max_lsn -= max_lsn_per_disk;

    return raid;
}

int simulate_raid(struct user_args* uargs) {
    struct raid_info *raid;

    raid=(struct raid_info*)malloc(sizeof(struct raid_info));
    alloc_assert(raid, "raid");
    memset(raid,0,sizeof(struct raid_info));

    raid = initialize_raid(raid, uargs);

    if (raid->raid_type == RAID_0) {
        simulate_raid0(raid);
    } else if (raid->raid_type == RAID_5) {
        simulate_raid5(raid);
    } else {
        printf("Error! unknown raid version\n");
        exit(1);
    }
    
    free_raid_ssd_and_tracefile(raid);
    free(raid);
    return 0;
}

void free_raid_ssd_and_tracefile(struct raid_info* raid) {
    for (int i = 0; i < raid->num_disk; i++) {
        free(raid->connected_ssd[i]);
    }
    fclose(raid->tracefile);
}

int64_t raid_find_nearest_event(struct raid_info* raid) {
    int64_t nearest_time = MAX_INT64, temp;

    for (int i = 0; i < raid->num_disk; i++) {
        temp = find_nearest_event(raid->connected_ssd[i]);
        if (temp < nearest_time) nearest_time = temp;
    }

    return nearest_time;
}

struct raid_info* simulate_raid0(struct raid_info* raid) {
    char buffer[200];
    long filepoint;
    int64_t req_incoming_time, nearest_event_time;
    int req_device_id, req_lsn, req_size, req_operation, flag;

    while (flag != 100) {

        // If we reach the end of the tracefile, stop the simulation
        if (feof(raid->tracefile)) {
            flag = 100;
        }
        
        // Read a request from tracefile
        filepoint = ftell(raid->tracefile);
        fgets (buffer, 200, raid->tracefile);
        sscanf (buffer,"%lld %d %d %d %d", &req_incoming_time, &req_device_id, &req_lsn, &req_size, &req_operation);

        // Validating incoming request
        if (req_device_id < 0 || req_lsn < 0 || req_size < 0 || !(req_operation == 0 || req_operation == 1)) {
            printf("Error! wrong io request from trace file\n");
            continue;
        }
        if (req_incoming_time < 0) {
            printf("Error! wrong incoming time!\n");
            while(1){}
        }
        req_lsn = req_lsn%raid->max_lsn;
        
        // Set raid current_time
        nearest_event_time = raid_find_nearest_event(raid);
        if (nearest_event_time == MAX_INT64) {
            raid->current_time = req_incoming_time;    
        } else if (nearest_event_time < req_incoming_time){
            fseek(raid->tracefile,filepoint,0);
            raid->current_time = nearest_event_time;
            continue;
        } else {
            if (raid->request_queue_length >= RAID_REQUEST_QUEUE_CAPACITY) {
                fseek(raid->tracefile,filepoint,0);
                raid->current_time = nearest_event_time;
                continue;
            } else {
                raid->current_time = req_incoming_time;
            }
        }

        printf("%lld %d %d %d %d\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
        
        // insert request to specific disk
        // a single request can be inserted to various disk too ._.
        raid_distrubute_request(raid, req_incoming_time, req_lsn, req_size, req_operation);


    }

    return raid;
}

// raid_distribute_request will distribute single IO request in raid level
// to IO request in disk level. A single IO request can be multiple IO request
// in disk level. This function return 0 if success and -1 if not.
int raid_distrubute_request(struct raid_info* raid, int64_t req_incoming_time, unsigned int req_lsn, unsigned int req_size, unsigned int req_operation) {
    unsigned int disk_id, strip_id, stripe_id, stripe_offset, strip_offset;
    int req_size_block = req_size;

    if (raid->raid_type == RAID_0) {
        while(req_size_block > 0){
            stripe_id = req_lsn / raid->stripe_size_block;
            stripe_offset = req_lsn - (stripe_id * raid->stripe_size_block);
            strip_id = stripe_offset / raid->strip_size_block;
            strip_offset = stripe_offset % raid->strip_size_block;
            disk_id = strip_id;

            // TODO: add this request to ssd queue
            printf("--> %u %u %u %u\n", disk_id, stripe_id, strip_id, strip_offset);
            
            req_size_block = req_size_block - (raid->strip_size_block - strip_offset);
            if (req_size_block > 0) {
                req_size = req_size_block;
                req_lsn = req_lsn + req_size;
            }
        }
    }

    return 0;
}


struct raid_info* simulate_raid5(struct raid_info* raid) {
    printf("Work in progress for simualting raid 5 ...\n");

    // controller's request queue
    // get_raid_request(raid)
    return raid;
}