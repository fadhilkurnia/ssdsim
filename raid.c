#include <stdio.h>
#include <unistd.h>
#include "pagemap.h"
#include "raid.h"
#include "ssd.h"

// initialize_raid function initializes raid struct and all ssd connected to the struct
struct raid_info* initialize_raid(struct raid_info* raid, struct user_args* uargs) {
    struct ssd_info *ssd_pointer;
    unsigned int max_lsn_per_disk;
    char *current_time;
    char logfilename[80];

    raid->raid_type = uargs->raid_type;
    raid->num_disk = uargs->num_disk;
    
    // try to access tracefile
    strcpy(raid->tracefilename, uargs->trace_filename);
    raid->tracefile = fopen(raid->tracefilename, "r");
    if(raid->tracefile == NULL) {
        printf("the tracefile can't be opened\n");
        exit(1);
    }

    // prepare raid logfile
    current_time = (char*) malloc(sizeof(char)*16);
    get_current_time(current_time);
    strcpy(logfilename, "raw/raid_"); strcat(logfilename, current_time); strcat(logfilename, ".log");
    strcpy(raid->logfilename, logfilename);
    raid->logfile = fopen(raid->logfilename, "w");
    if (raid->logfile == NULL) {
        printf("Error: can't create logfile for this raid simulation\n");
        exit(1);
    }

    // prepare ssds struct
    raid->connected_ssd=malloc(sizeof(struct ssd_info) * raid->num_disk);
    alloc_assert(raid->connected_ssd, "connected ssd");
    for (int i = 0; i < raid->num_disk; i++) {
        ssd_pointer=(struct ssd_info*) malloc(sizeof(struct ssd_info));
        alloc_assert(ssd_pointer, "one of connected ssd");
        memset(ssd_pointer,0,sizeof(struct ssd_info));
        raid->connected_ssd[i] = ssd_pointer;
    }

    // prepare gc scheduling algorithm
    if (uargs->is_gclock) {
        raid->gclock = (struct gclock_raid_info*) malloc(sizeof(struct gclock_raid_info));
        alloc_assert(raid->gclock, "gclock_info");
        memset(raid->gclock,0,sizeof(struct ssd_info));
        raid->gclock->is_available = 1;
        raid->gclock->gc_count = 0;
        raid->gclock->holder_id = -1;
    }

    // initialize each ssd in raid
    for (int i = 0; i < raid->num_disk; i++) {
        printf("\nInitializing disk%d\n", i);

        get_current_time(current_time);
        strcpy(uargs->simulation_timestamp, current_time);
        uargs->diskid = i;

        raid->connected_ssd[i] = initialize_ssd(raid->connected_ssd[i], uargs);
        raid->connected_ssd[i] = initiation(raid->connected_ssd[i]);
        raid->connected_ssd[i] = make_aged(raid->connected_ssd[i]);
        raid->connected_ssd[i] = pre_process_page(raid->connected_ssd[i]);
        raid->connected_ssd[i]->tracefile = NULL;
        raid->connected_ssd[i]->diskid = i;

        if (uargs->is_gclock) {
            raid->connected_ssd[i]->gclock_pointer = raid->gclock;
        }

        fprintf(raid->logfile, "raw/%s/\n", current_time);
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

    free(current_time);
    fclose(raid->logfile);
    printf("RAID initialized!\n");

    return raid;
}

struct raid_request* initialize_raid_request(struct raid_request* raid_req, int64_t req_incoming_time, unsigned int req_lsn, unsigned int req_size, unsigned int req_operation) {
    raid_req->begin_time = req_incoming_time;
    raid_req->lsn = req_lsn;
    raid_req->size = req_size;
    raid_req->operation = req_operation;
    raid_req->prev_node = NULL;
    raid_req->next_node = NULL;
    return raid_req;
}

struct raid_sub_request* initialize_raid_sub_request(struct raid_sub_request* raid_subreq, struct raid_request* raid_req, unsigned int disk_id, unsigned int stripe_id, unsigned int strip_id, unsigned int strip_offset, unsigned int lsn, unsigned int size, unsigned int operation) {
    struct raid_sub_request *ptr = NULL;
    unsigned int state = R_SR_PENDING;

    if (operation == WRITE_RAID) {
        operation = WRITE;
        state = R_SR_WAIT_PARITY;
    }

    raid_subreq->disk_id = disk_id;
    raid_subreq->stripe_id = stripe_id;
    raid_subreq->strip_id = strip_id;
    raid_subreq->strip_offset = strip_offset;
    raid_subreq->begin_time = raid_req->begin_time+RAID_SSD_LATENCY_NS;
    raid_subreq->operation = operation;
    raid_subreq->current_state = state;
    raid_subreq->lsn = lsn;
    raid_subreq->size = size;
    raid_subreq->next_node = NULL;
    if (raid_req->subs == NULL) {
        raid_req->subs = raid_subreq;
    } else {
        ptr = raid_req->subs;
        while(ptr->next_node != NULL) {
            ptr = ptr->next_node;
        }
        ptr->next_node = raid_subreq;
    }
    return raid_subreq;
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
    struct ssd_info *ssd;

    for (int i = 0; i < raid->num_disk; i++) {
        ssd = raid->connected_ssd[i];
        statistic_output(ssd);
        close_file(ssd);
        free(ssd);
    }
    if (raid->gclock != NULL) {
        free(raid->gclock);
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

// raid_distribute_request will distribute single IO request in raid level
// to IO request in disk level. A single IO request can be splitted into multiple IO request
// in disk level. This function return R_DIST_SUCCESS (0) if success and R_DIST_ERR (1) if not.
int raid_distribute_request(struct raid_info* raid, int64_t req_incoming_time, unsigned int req_lsn, unsigned int req_size, unsigned int req_operation) {
    unsigned int disk_id, strip_id, stripe_id, stripe_offset, strip_offset, disk_req_lsn, disk_req_size;
    int req_size_block = req_size, parity_strip_id;
    struct raid_request* raid_req;
    struct raid_sub_request* raid_subreq;

    if (raid->raid_type == RAID_0) {

        if (raid->request_queue_length == RAID_REQUEST_QUEUE_CAPACITY) {
            return R_DIST_ERR;
        }

        raid_req = (struct raid_request*)malloc(sizeof(struct raid_request));
        alloc_assert(raid_req, "raid_request");
        memset(raid_req,0,sizeof(struct raid_request));
        initialize_raid_request(raid_req, req_incoming_time, req_lsn, req_size, req_operation);

        while(req_size_block > 0) {
            stripe_id = req_lsn / raid->stripe_size_block;
            stripe_offset = req_lsn - (stripe_id * raid->stripe_size_block);
            strip_id = stripe_offset / raid->strip_size_block;
            strip_offset = stripe_offset % raid->strip_size_block;
            disk_id = strip_id;
            disk_req_lsn = (stripe_id * raid->strip_size_block) + strip_offset;
            disk_req_size = (raid->strip_size_block - strip_offset >= req_size) ? req_size : raid->strip_size_block - strip_offset;

            // add sub_request to request
            #ifdef DEBUG
            printf("--> req distributed to ssd: %u %u %u %u %u\n", disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn);
            #endif
            raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
            alloc_assert(raid_subreq, "raid_sub_request");
            memset(raid_subreq,0,sizeof(struct raid_sub_request));
            initialize_raid_sub_request(raid_subreq, raid_req, disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn, disk_req_size, req_operation);
            
            req_size_block = req_size_block - (raid->strip_size_block - strip_offset);
            if (req_size_block > 0) {
                req_size = req_size_block;
                req_lsn = req_lsn + disk_req_size;
            }
        }

        // add request to raid request queue
        if (raid->request_queue == NULL) {
            raid->request_queue = raid_req;
            raid->request_tail = raid->request_queue;
        } else {
            raid_req->prev_node = raid->request_tail;
            raid->request_tail->next_node = raid_req;
            raid->request_tail = raid_req;
        }
        raid->request_queue_length = raid->request_queue_length + 1;
    
    } else if (raid->raid_type==RAID_5) {

        if (raid->request_queue_length == RAID_REQUEST_QUEUE_CAPACITY) {
            return R_DIST_ERR;
        }

        raid_req = (struct raid_request*)malloc(sizeof(struct raid_request));
        alloc_assert(raid_req, "raid_request");
        memset(raid_req,0,sizeof(struct raid_request));
        initialize_raid_request(raid_req, req_incoming_time, req_lsn, req_size, req_operation);

        // handle read request
        if (raid_req->operation == READ) {
            while (req_size_block > 0) {
                stripe_id = req_lsn / (raid->strip_size_block*(raid->num_disk-1)); // not include parity strip
                stripe_offset = req_lsn - (raid->strip_size_block * (raid->num_disk-1) * stripe_id);
                strip_id = stripe_offset / raid->strip_size_block;
                parity_strip_id = stripe_id % raid->num_disk;
                if (parity_strip_id <= strip_id) strip_id++;
                strip_offset = stripe_offset % raid->strip_size_block;
                disk_id = strip_id;
                disk_req_lsn = (stripe_id * raid->strip_size_block) + strip_offset;
                disk_req_size = (raid->strip_size_block - strip_offset >= req_size_block) ? req_size_block : raid->strip_size_block - strip_offset;

                // add sub_request to request
                raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
                alloc_assert(raid_subreq, "raid_sub_request");
                memset(raid_subreq,0,sizeof(struct raid_sub_request));
                initialize_raid_sub_request(raid_subreq, raid_req, disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn, disk_req_size, req_operation);

                req_size_block = req_size_block - disk_req_size;
                if (req_size_block > 0) {
                    req_lsn = req_lsn + disk_req_size;
                }
            }
        }

        // handle write request, parity calculation
        if (raid_req->operation == WRITE) {
            while (req_size_block > 0) {
                #ifdef DEBUGRAID
                printf(">> read old data %lld %d\n", req_incoming_time, req_size_block);
                #endif
                // reading old data
                stripe_id = req_lsn / (raid->strip_size_block*(raid->num_disk-1)); // not include parity strip
                stripe_offset = req_lsn - (raid->strip_size_block * (raid->num_disk-1) * stripe_id);
                strip_id = stripe_offset / raid->strip_size_block;
                parity_strip_id = stripe_id % raid->num_disk;
                if (parity_strip_id <= strip_id) strip_id++;
                strip_offset = stripe_offset % raid->strip_size_block;
                disk_id = strip_id;
                disk_req_lsn = (stripe_id * raid->strip_size_block) + strip_offset;
                disk_req_size = (raid->strip_size_block - strip_offset >= req_size_block) ? req_size_block : raid->strip_size_block - strip_offset;

                raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
                alloc_assert(raid_subreq, "raid_sub_request");
                memset(raid_subreq,0,sizeof(struct raid_sub_request));
                initialize_raid_sub_request(raid_subreq, raid_req, disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn, disk_req_size, READ);

                req_size_block = req_size_block - disk_req_size;
                if (req_size_block > 0) {
                    req_lsn = req_lsn + disk_req_size;
                }
                
                // reading old parity
                if (req_size_block <= 0 || disk_id == raid->num_disk-2) {
                    raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
                    alloc_assert(raid_subreq, "raid_sub_request");
                    memset(raid_subreq,0,sizeof(struct raid_sub_request));
                    initialize_raid_sub_request(raid_subreq, raid_req, parity_strip_id, stripe_id, parity_strip_id, 0, disk_req_lsn, raid->strip_size_block, READ);
                }
            }

            // calculating new parity will be handled on raid5_finish_parity_calculation();
            req_size_block = req_size;
            while (req_size_block > 0) {
                // writing new data
                stripe_id = req_lsn / (raid->strip_size_block*(raid->num_disk-1)); // not include parity strip
                stripe_offset = req_lsn - (raid->strip_size_block * (raid->num_disk-1) * stripe_id);
                strip_id = stripe_offset / raid->strip_size_block;
                parity_strip_id = stripe_id % raid->num_disk;
                if (parity_strip_id <= strip_id) strip_id++;
                strip_offset = stripe_offset % raid->strip_size_block;
                disk_id = strip_id;
                disk_req_lsn = (stripe_id * raid->strip_size_block) + strip_offset;
                disk_req_size = (raid->strip_size_block - strip_offset >= req_size_block) ? req_size_block : raid->strip_size_block - strip_offset;

                raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
                alloc_assert(raid_subreq, "raid_sub_request");
                memset(raid_subreq,0,sizeof(struct raid_sub_request));
                initialize_raid_sub_request(raid_subreq, raid_req, disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn, disk_req_size, WRITE_RAID);

                req_size_block = req_size_block - disk_req_size;
                if (req_size_block > 0) {
                    req_lsn = req_lsn + disk_req_size;
                }
                
                // writing new parity
                if (req_size_block <= 0 || disk_id == raid->num_disk-2) {
                    raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
                    alloc_assert(raid_subreq, "raid_sub_request");
                    memset(raid_subreq,0,sizeof(struct raid_sub_request));
                    initialize_raid_sub_request(raid_subreq, raid_req, parity_strip_id, stripe_id, parity_strip_id, 0, disk_req_lsn, raid->strip_size_block, WRITE_RAID);
                }
            }

        }

        // add request to raid request queue
        if (raid->request_queue == NULL) {
            raid->request_queue = raid_req;
            raid->request_tail = raid->request_queue;
        } else {
            raid_req->prev_node = raid->request_tail;
            raid->request_tail->next_node = raid_req;
            raid->request_tail = raid_req;
        }
        raid->request_queue_length = raid->request_queue_length + 1;

    } else {
        printf("Error: unknown RAID type!\n");
        exit(100);
    }

    return R_DIST_SUCCESS;
}

int raid_clear_completed_request(struct raid_info* raid) {
    struct raid_request* req_pointer, *temp_r;
    struct raid_sub_request* subreq_pointer, *temp_sr;
    int is_all_completed = 1, is_need_move_forward = 1;

    req_pointer = raid->request_queue;
    while (req_pointer != NULL){
        is_all_completed = is_need_move_forward = 1;
        subreq_pointer = req_pointer->subs;
        while (subreq_pointer != NULL){
            if (subreq_pointer->current_state != R_SR_COMPLETE) {
                is_all_completed = 0;
                break;
            }
            subreq_pointer = subreq_pointer->next_node;
        }

        if (is_all_completed) {
            subreq_pointer = req_pointer->subs;
            while (subreq_pointer != NULL){
                temp_sr = subreq_pointer;
                subreq_pointer = temp_sr->next_node;
                free(temp_sr);
            }
            req_pointer->subs = NULL;

            if (raid->request_queue_length == 1) {              // the only element in queue
                free((void *)req_pointer);
                req_pointer = NULL;
                raid->request_queue = raid->request_tail = NULL;
                is_need_move_forward = 0;
            } else if (raid->request_queue == req_pointer) {    // first element in queue with multiple elements
                raid->request_queue = req_pointer->next_node;
                raid->request_queue->prev_node = NULL;
                free((void *)req_pointer);
                req_pointer = raid->request_queue;
                is_need_move_forward = 0;
            } else {                                            // middle or last element in queue
                temp_r = req_pointer->prev_node;
                temp_r->next_node = req_pointer->next_node;
                if (raid->request_tail == req_pointer) {
                    raid->request_tail = req_pointer->prev_node;
                } else {
                    (req_pointer->next_node)->prev_node = temp_r;
                }
                free((void *)req_pointer);
                req_pointer = temp_r;
            }

            raid->request_queue_length = raid->request_queue_length - 1;
        }

        if (req_pointer != NULL && is_need_move_forward)
            req_pointer = req_pointer->next_node;
    }
    return 0;
}

void raid_simulate_ssd(struct raid_info* raid, int disk_id) {
    // iterate raid request queue, find first incoming request for this disk
    struct ssd_info* ssd;
    int interface_flag = 0;

    ssd = raid->connected_ssd[disk_id];

    // Interface layer
    interface_flag = raid_ssd_get_requests(disk_id, ssd, raid);

    // Buffer layer
    if(interface_flag == 1) {
        if (ssd->parameter->dram_capacity!=0) {
            buffer_management(ssd);
            distribute(ssd); 
            } else {
            no_buffer_distribute(ssd);
        }
    }

    // FTL+FCL+Flash layer
    process(ssd);
    raid_ssd_trace_output(ssd);

}

// raid_ssd_get_requests will try to insert request from raid request queue to
// ssd request queue. Return    1: request added to ssd request queue
//                             -1: no request added to ssd request queue
//                              0: no request in raid req queue for this disk
int raid_ssd_get_requests(int disk_id, struct ssd_info *ssd, struct raid_info *raid) {
    struct raid_request *rreq;
    struct raid_sub_request *rsreq;
    struct request *ssd_request;
    int is_found = 0;

    unsigned int req_lsn=0;
    int req_device, req_size, req_ope, large_lsn;
    int64_t req_time = 0;
    int64_t nearest_event_time;

    rreq = raid->request_queue;

    // get first unprocessed request for this disk from raid req queue
    while (rreq != NULL && !is_found) {
        rsreq = rreq->subs;
        while (rsreq != NULL) {
            if (rsreq->disk_id == disk_id && rsreq->current_state == R_SR_PENDING) {
                is_found = 1;
                break;
            }
            rsreq = rsreq->next_node;
        }
        rreq = rreq->next_node;
    }

    nearest_event_time = raid_find_nearest_event(raid);

    // no request for this disk
    if (rsreq == NULL || !is_found) {
        if (nearest_event_time != MAX_INT64)
            ssd->current_time=nearest_event_time;
        return 0;
    }

    // insert this request to ssd's request queue
    req_device = disk_id;
    req_time = rsreq->begin_time;
    req_lsn = rsreq->lsn;
    req_size = rsreq->size;
    req_ope = rsreq->operation;

    if (req_device < 0 || req_size < 0 || req_lsn < 0 || !(req_ope == WRITE || req_ope == READ)) {
        printf("Error! wrong io request from raid controller\n");
        exit(100);
    }

    large_lsn=(int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
    req_lsn = req_lsn%large_lsn;

    if (nearest_event_time==MAX_INT64) {
        ssd->current_time = req_time;
    } else {
        if (nearest_event_time < req_time) {
            if (ssd->current_time <= nearest_event_time) ssd->current_time = nearest_event_time;
            return -1;
        } else {
            if (ssd->request_queue_length>=ssd->parameter->queue_length) {
                ssd->current_time = nearest_event_time;
                return -1;
            } else {
                ssd->current_time = req_time;
            }
        }
    }

    if (req_time < 0) {
        printf("Error! wrong io request's incoming time\n");
        exit(100);
    }

    // set this raid's sub request state to R_SR_PROCESS
    rsreq->current_state = R_SR_PROCESS;

    ssd_request = (struct request*) malloc(sizeof(struct request));
    alloc_assert(ssd_request, "ssd_request");
    memset(ssd_request, 0, sizeof(struct request));

    ssd_request->time = req_time;
    ssd_request->lsn = req_lsn;
    ssd_request->size = req_size;
    ssd_request->operation = req_ope;
    ssd_request->begin_time = req_time;
    ssd_request->response_time = 0;	
    ssd_request->energy_consumption = 0;	
    ssd_request->next_node = NULL;
    ssd_request->distri_flag = 0;
    ssd_request->subs = NULL;
    ssd_request->need_distr_flag = NULL;
    ssd_request->complete_lsn_count=0;
    ssd_request->subreq_on_raid = rsreq;

    // add to ssd queue
    if (ssd->request_queue == NULL) {
        ssd->request_queue = ssd_request;
    } else {
        (ssd->request_tail)->next_node = ssd_request;
    }
    ssd->request_tail = ssd_request;
    ssd->request_queue_length++;

    // update ssd statistics
    if (ssd_request->lsn > ssd->max_lsn) ssd->max_lsn = ssd_request->lsn;
    if (ssd_request->lsn < ssd->min_lsn) ssd->min_lsn = ssd_request->lsn;
    if (ssd_request->operation == WRITE) ssd->ave_write_size=(ssd->ave_write_size*ssd->write_request_count+ssd_request->size)/(ssd->write_request_count+1);
    if (ssd_request->operation == READ) ssd->ave_read_size=(ssd->ave_read_size*ssd->read_request_count+ssd_request->size)/(ssd->read_request_count+1);

    return 1;
}

int raid_ssd_interface(struct ssd_info* ssd, struct raid_sub_request *subreq) {
    int large_lsn;
    int64_t nearest_event_time;
    struct request *ssd_request;
    
    nearest_event_time = find_nearest_event(ssd);
    
    // only update ssd->current time if subreq is null
    // this is used only after the "tracefile" is eof
    // until the request queue in this ssd is empty
    if (subreq == NULL) {
        ssd->current_time = nearest_event_time;
        return SUCCESS;
    }

    large_lsn = (int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
    subreq->lsn = subreq->lsn % large_lsn;

    if (nearest_event_time == MAX_INT64) {
        ssd->current_time = subreq->begin_time;
    } else {
        if (nearest_event_time < subreq->begin_time) {
            if (ssd->current_time <= nearest_event_time) ssd->current_time = nearest_event_time;
            return ERROR;
        } else {
            if (ssd->request_queue_length >= ssd->parameter->queue_length) {
                return ERROR;
            } else {
                ssd->current_time = subreq->begin_time;
            }
        }
    }

    ssd_request = (struct request*) malloc(sizeof(struct request));
    alloc_assert(ssd_request, "ssd_request");
    memset(ssd_request, 0, sizeof(struct request));

    ssd_request->time = subreq->begin_time;
    ssd_request->lsn = subreq->lsn;
    ssd_request->size = subreq->size;
    ssd_request->operation = subreq->operation;
    ssd_request->begin_time = subreq->begin_time;
    ssd_request->response_time = 0;	
    ssd_request->energy_consumption = 0;	
    ssd_request->next_node = NULL;
    ssd_request->distri_flag = 0;
    ssd_request->subs = NULL;
    ssd_request->need_distr_flag = NULL;
    ssd_request->complete_lsn_count=0;
    ssd_request->subreq_on_raid = subreq;

    // add to ssd queue
    if (ssd->request_queue == NULL) {
        ssd->request_queue = ssd_request;
    } else {
        (ssd->request_tail)->next_node = ssd_request;
    }
    ssd->request_tail = ssd_request;
    ssd->request_queue_length++;

    // update ssd statistics
    if (ssd_request->lsn > ssd->max_lsn) ssd->max_lsn = ssd_request->lsn;
    if (ssd_request->lsn < ssd->min_lsn) ssd->min_lsn = ssd_request->lsn;
    if (ssd_request->operation == WRITE) ssd->ave_write_size=(ssd->ave_write_size*ssd->write_request_count+ssd_request->size)/(ssd->write_request_count+1);
    if (ssd_request->operation == READ) ssd->ave_read_size=(ssd->ave_read_size*ssd->read_request_count+ssd_request->size)/(ssd->read_request_count+1);

    return SUCCESS;
}

void raid_ssd_trace_output(struct ssd_info* ssd) {
    struct request *req, *req_temp;
    struct sub_request *sub, *tmp;
    int64_t start_time, end_time, latency;
    int is_all_sub_completed;

    req_temp = NULL;
    req = ssd->request_queue;
    if (req == NULL)
        return;

    while (req != NULL){
        latency = 0;
        if (req->response_time != 0) {
            latency = req->response_time-req->time;
            #ifdef DEBUG
            printf("%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
            #endif
            fprintf(ssd->outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time); fflush(ssd->outputfile);
            fprintf(ssd->outfile_io,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time); fflush(ssd->outfile_io);
            if (req->operation == WRITE) {
                fprintf(ssd->outfile_io_write,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                fflush(ssd->outfile_io_write);
                ssd->write_request_count++;
                ssd->write_avg=ssd->write_avg+(req->response_time-req->time);
            } else if (req->operation == READ){
                fprintf(ssd->outfile_io_read,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                fflush(ssd->outfile_io_read);
                ssd->read_request_count++;
                ssd->read_avg=ssd->read_avg+(req->response_time-req->time);
            }
            
            if(req->response_time-req->begin_time==0) {
                printf("the response time is 0?? \n");
                exit(1);
            }

            req_temp = req;
            req = req->next_node;
            ssd_delete_request_from_queue(ssd, req_temp);

        } else {
            sub = req->subs;
            is_all_sub_completed = 1;
            start_time = end_time = 0;

            // if any sub-request is not completed, the request is not completed
            while (sub != NULL){
                if (start_time == 0) start_time = sub->begin_time;
                if (start_time > sub->begin_time) start_time = sub->begin_time;
                if (end_time < sub->complete_time) end_time = sub->complete_time;

                if((sub->current_state == SR_COMPLETE)||((sub->next_state==SR_COMPLETE)&&(sub->next_state_predict_time<=ssd->current_time))) {
                    sub = sub->next_subs;
                } else {
                    is_all_sub_completed = 0;
                    break;
                }
            }
            
            if (is_all_sub_completed) {
                latency = end_time-req->time;
                
                #ifdef DEBUG
                printf("%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                #endif
                fprintf(ssd->outputfile,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                fflush(ssd->outputfile);
                fprintf(ssd->outfile_io,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                fflush(ssd->outfile_io);

                if (req->operation == WRITE) {
                    fprintf(ssd->outfile_io_write,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                    fflush(ssd->outfile_io_write);
                    ssd->write_request_count++;
                    ssd->write_avg=ssd->write_avg+(end_time-req->time);
                } else if (req->operation == READ){
                    fprintf(ssd->outfile_io_read,"%16lld %10d %6d %2d %16lld %16lld %10lld %2d %10lld\n",req->time,req->lsn, req->size, req->operation, req->begin_time, req->response_time, latency, req->meet_gc_flag, req->meet_gc_remaining_time);
                    fflush(ssd->outfile_io_read);
                    ssd->read_request_count++;
                    ssd->read_avg=ssd->read_avg+(end_time-req->time);
                }

                if(end_time-start_time==0) {
                    printf("the response time is 0?? \n");
                    exit(1);
                } else {
                    req->response_time=end_time;
                }

                // empty the sub on the req
                while (req->subs!=NULL){
                    tmp = req->subs;
                    req->subs = tmp->next_subs;
                    if (tmp->update!=NULL) {
                        free(tmp->update->location);
                        tmp->update->location=NULL;
                        free(tmp->update);
                        tmp->update=NULL;
                    }
                    free(tmp->location);
                    tmp->location=NULL;
                    free((void *)tmp);
                    tmp=NULL;
                }
                
                req_temp = req;
                req = req->next_node;
                ssd_delete_request_from_queue(ssd, req_temp);

            } else {
                req = req->next_node;
            }

        }
    }

    return;
}

void ssd_delete_request_from_queue(struct ssd_info* ssd, struct request *req) {
    struct request *temp, *prev;

    // update raid subrequest
    (req->subreq_on_raid)->current_state = R_SR_COMPLETE;
    (req->subreq_on_raid)->complete_time = req->response_time+RAID_SSD_LATENCY_NS;

    // req is first element of queue
    if (req == ssd->request_queue) {
        req->subreq_on_raid = NULL;
        ssd->request_queue = req->next_node;

        free(req->need_distr_flag);
        req->need_distr_flag=NULL;

        // req is alone in the queue
        if (ssd->request_tail == req) {
            ssd->request_tail=NULL;
        }
        
        free((void *)req);
        ssd->request_queue_length--;
        return;
    }

    // find the prev req before req
    temp = ssd->request_queue;
    while (temp != NULL && temp != req) { 
        prev = temp; 
        temp = temp->next_node; 
    } 

    if (temp == NULL || prev == NULL) {
        printf("Error! can't find request that need to be deleted\n");
        exit(100);
    }

    prev->next_node = req->next_node;
    if (ssd->request_tail == req) ssd->request_tail = prev;
    req->next_node = NULL;
    free(req->need_distr_flag);
    req->need_distr_flag = NULL;
    free((void *)req);
    ssd->request_queue_length--;
    return;
}

// raid5_finish_parity_calculation will change the state of write request
void raid5_finish_parity_calculation(struct raid_info* raid) {
    struct raid_request* rreq;
    struct raid_sub_request *srreq;
    int is_all_read_finish, is_processing_write;
    int64_t read_finish_time;

    rreq = raid->request_queue;
    while(rreq!=NULL){
        if (rreq->operation==WRITE) {
            is_all_read_finish = 1; is_processing_write = 0; read_finish_time = 0;
            srreq = rreq->subs;

            while(srreq!=NULL){
                if (srreq->operation==READ && srreq->current_state!=R_SR_COMPLETE) {
                    is_all_read_finish = 0;
                    break;
                }
                if (srreq->operation==WRITE && srreq->current_state!=R_SR_WAIT_PARITY) {
                    is_processing_write = 1;
                    break;
                }
                if (srreq->operation==READ && srreq->current_state==R_SR_COMPLETE && read_finish_time<srreq->complete_time) {
                    read_finish_time = srreq->complete_time;
                }
                srreq = srreq->next_node;
            }

            // if all read req is finished, we can continue our write req
            if (is_all_read_finish && !is_processing_write) {
                srreq = rreq->subs;
                while(srreq!=NULL){
                    if (srreq->operation==WRITE) {
                        srreq->current_state=R_SR_PENDING;
                        srreq->begin_time=read_finish_time+RAID5_PARITY_CALC_TIME_NS+RAID_SSD_LATENCY_NS;
                    }
                    srreq = srreq->next_node;
                }
            }
        }
        rreq = rreq->next_node;
    }
    
}

void raid_print_req_queue(struct raid_info* raid) {
    struct raid_request* rreq;
    struct raid_sub_request *srreq;
    int rreq_id = 0;

    printf(" ============ RAID REQUEST QUEUE ============ \n");
    rreq = raid->request_queue;
    while (rreq != NULL) {
        printf(" [%d] %lld %lld %u %u %u\n", rreq_id, rreq->begin_time, rreq->response_time, rreq->lsn, rreq->size, rreq->operation);
        srreq = rreq->subs;
        while (srreq != NULL) {
            printf("    %u %u %lld %lld\n", srreq->disk_id, srreq->current_state, srreq->begin_time, srreq->complete_time);
            srreq = srreq->next_node;
        }
        rreq = rreq->next_node;
        rreq_id++;
    }
    printf(" ============ RAID REQUEST QUEUE ============ \n");

}

struct raid_info* simulate_raid0(struct raid_info* raid) {
    int req_device_id, req_size, req_operation, flag, err, is_accept_req, interface_flag;
    int64_t req_incoming_time, nearest_event_time, req_lsn;
    struct ssd_info *ssd;
    char buffer[200];
    long filepoint;

    // Run the RAID0 simulation untill all the request is tracefile is processed
    while (flag != RAID_SIMULATION_FINISH) {

        // Stop the simulation, if we reach the end of the tracefile and request queue is empty
        if (feof(raid->tracefile) && raid->request_queue_length==0) {
            flag = RAID_SIMULATION_FINISH;
        }
        
        // Trying to get a request from tracefile
        if (!feof(raid->tracefile)) {

            // Read a request from tracefile
            filepoint = ftell(raid->tracefile);
            fgets (buffer, 200, raid->tracefile);
            sscanf (buffer,"%lld %d %lld %d %d", &req_incoming_time, &req_device_id, &req_lsn, &req_size, &req_operation);
            is_accept_req = 1;

            // Validating incoming request
            if (req_device_id < 0 || req_lsn < 0 || req_size < 0 || !(req_operation == 0 || req_operation == 1)) {
                printf("Error! wrong io request from tracefile (%lld %d %lld %d %d)\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
                exit(1);
            }
            if (req_incoming_time < 0) {
                printf("Error! wrong incoming time! (%lld %d %lld %d %d)\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
                exit(1);
            }
            req_lsn = req_lsn%raid->max_lsn;
            
            // Check whether we can process this request or not
            nearest_event_time = raid_find_nearest_event(raid);
            #ifdef DEBUG
            printf(" nearest time %lld %lld %lld %d\n", nearest_event_time, req_incoming_time, raid->current_time, raid->request_queue_length);
            #endif
            if (raid->request_queue_length >= RAID_REQUEST_QUEUE_CAPACITY) {
                fseek(raid->tracefile,filepoint,0);
                is_accept_req = 0;
            }
            if (nearest_event_time != MAX_INT64) raid->current_time = nearest_event_time;

            if (is_accept_req) {
                #ifdef DEBUG
                printf("req inserted: %lld %d %d %d %d [%d]\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation, raid->request_queue_length);
                #endif
            
                // insert request to raid rquest queue
                // a single request can be forwarder to multiple disk
                err = raid_distribute_request(raid, req_incoming_time, req_lsn, req_size, req_operation);
                if (err == R_DIST_ERR) {
                    fseek(raid->tracefile,filepoint,0);
                    printf("Error! Distributing raid request failed!\n");
                    // getchar();
                    continue;
                }
            }
        }

        // simulate all the ssd in the raid, 
        // this is corresponding to simulate(ssd_info *ssd) function in ssd.c
        for(int i = 0; i < raid->num_disk; i++) {
            raid_simulate_ssd(raid, i);
        }
        
        // remove processed request from raid queue
        raid_clear_completed_request(raid);

    }

    return raid;
}

struct raid_info* simulate_raid5(struct raid_info* raid) {
    int req_device_id, req_size, req_operation, flag, err, is_accept_req, interface_flag;
    int64_t req_incoming_time, nearest_event_time, req_lsn;
    struct ssd_info *ssd;
    char buffer[200];
    long filepoint;
    
    // Run the RAID5 simulation untill all the request is tracefile is processed
    while (flag != RAID_SIMULATION_FINISH) {
        
        // Stop the simulation, if we reach the end of the tracefile and request queue is empty
        if (feof(raid->tracefile) && raid->request_queue_length==0) {
            flag = RAID_SIMULATION_FINISH;
        }

        // Trying to get a request from tracefile
        if (!feof(raid->tracefile)) {

            // Read a request from tracefile
            filepoint = ftell(raid->tracefile);
            fgets (buffer, 200, raid->tracefile);
            sscanf (buffer,"%lld %d %lld %d %d", &req_incoming_time, &req_device_id, &req_lsn, &req_size, &req_operation);
            is_accept_req = 1;

            // Validating incoming request
            if (req_device_id < 0 || req_lsn < 0 || req_size < 0 || !(req_operation == 0 || req_operation == 1)) {
                printf("Error! wrong io request from tracefile (%lld %d %lld %d %d)\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
                exit(1);
            }
            if (req_incoming_time < 0) {
                printf("Error! wrong incoming time! (%lld %d %lld %d %d)\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
                exit(1);
            }
            req_lsn = req_lsn%raid->max_lsn;
            
            // Check whether we can process this request or not
            nearest_event_time = raid_find_nearest_event(raid);
            #ifdef DEBUGRAID
            printf(" nearest time %lld %lld %lld %d\n", nearest_event_time, req_incoming_time, raid->current_time, raid->request_queue_length);
            #endif
            if (raid->request_queue_length >= RAID_REQUEST_QUEUE_CAPACITY) {
                fseek(raid->tracefile,filepoint,0);
                is_accept_req = 0;
            }
            if (nearest_event_time != MAX_INT64) raid->current_time = nearest_event_time;

            if (is_accept_req) {
                #ifdef DEBUGRAID
                printf("req inserted: %lld %d %d %d %d [%d]\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation, raid->request_queue_length);
                #endif
            
                // insert request to raid rquest queue
                // a single request can be forwarder to multiple disk
                err = raid_distribute_request(raid, req_incoming_time, req_lsn, req_size, req_operation);
                if (err == R_DIST_ERR) {
                    fseek(raid->tracefile,filepoint,0);
                    printf("Error! Distributing raid request failed!\n");
                    // getchar();
                    continue;
                }
            }
        }

        // simulate all the ssd in the raid, 
        // this is corresponding to simulate(ssd_info *ssd) function in ssd.c
        for(int i = 0; i < raid->num_disk; i++) {
            raid_simulate_ssd(raid, i);
        }

        // check parity calculation process in write req
        raid5_finish_parity_calculation(raid);
        
        // remove processed request from raid queue
        raid_clear_completed_request(raid);
        
    }

    return raid;
}