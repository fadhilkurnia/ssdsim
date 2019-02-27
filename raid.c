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
    if(raid->tracefile == NULL) {
        printf("the tracefile can't be opened\n");
        exit(1);
    }

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
    raid_subreq->disk_id = disk_id;
    raid_subreq->stripe_id = stripe_id;
    raid_subreq->strip_id = strip_id;
    raid_subreq->strip_offset = strip_offset;
    raid_subreq->begin_time = raid_req->begin_time;
    raid_subreq->current_state = R_SR_PENDING;
    raid_subreq->lsn = lsn;
    raid_subreq->size = size;
    raid_subreq->operation = operation;
    raid_subreq->next_node = NULL;
    if (raid_req->subs == NULL) {
        raid_req->subs = raid_subreq;
    } else {
        raid_req->subs->next_node = raid_subreq;
        raid_req->subs = raid_subreq;
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
    int req_device_id, req_lsn, req_size, req_operation, flag, err;

    while (flag != RAID_SIMULATION_FINISH) {

        // If we reach the end of the tracefile, stop the simulation
        if (feof(raid->tracefile)) {
            flag = RAID_SIMULATION_FINISH;
        }
        
        // Read a request from tracefile
        filepoint = ftell(raid->tracefile);
        fgets (buffer, 200, raid->tracefile);
        sscanf (buffer,"%lld %d %d %d %d", &req_incoming_time, &req_device_id, &req_lsn, &req_size, &req_operation);

        // Validating incoming request
        if (req_device_id < 0 || req_lsn < 0 || req_size < 0 || !(req_operation == 0 || req_operation == 1)) {
            printf("Error! wrong io request from tracefile (%lld %d %d %d %d)\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
            exit(1);
        }
        if (req_incoming_time < 0) {
            printf("Error! wrong incoming time! (%lld %d %d %d %d)\n", req_incoming_time, req_device_id, req_lsn, req_size, req_operation);
            exit(1);
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
        
        // insert request to raid rquest queue
        // a single request can be forwarder to multiple disk
        err = raid_distribute_request(raid, req_incoming_time, req_lsn, req_size, req_operation);
        if (err == R_DIST_ERR) {
            fseek(raid->tracefile,filepoint,0);
            continue;
        }

        // simulate all the ssd in the raid
        for(int i = 0; i < raid->num_disk; i++) {
            printf("simulate disk %d\n", i);
            raid_simulate_ssd(raid, i);
            printf("end simulate disk %d\n", i);
        }
        
        // remove processed request from raid queue
        raid_clear_completed_request(raid);


    }

    return raid;
}

// raid_distribute_request will distribute single IO request in raid level
// to IO request in disk level. A single IO request can be multiple IO request
// in disk level. This function return R_DIST_SUCCESS (0) if success and R_DIST_ERR (1) if not.
int raid_distribute_request(struct raid_info* raid, int64_t req_incoming_time, unsigned int req_lsn, unsigned int req_size, unsigned int req_operation) {
    unsigned int disk_id, strip_id, stripe_id, stripe_offset, strip_offset, disk_req_lsn, disk_req_size;
    int req_size_block = req_size;
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

        while(req_size_block > 0){
            stripe_id = req_lsn / raid->stripe_size_block;
            stripe_offset = req_lsn - (stripe_id * raid->stripe_size_block);
            strip_id = stripe_offset / raid->strip_size_block;
            strip_offset = stripe_offset % raid->strip_size_block;
            disk_id = strip_id;
            disk_req_lsn = (stripe_id * raid->strip_size_block) + strip_offset;
            disk_req_size = (raid->strip_size_block - strip_offset >= req_size) ? raid->strip_size_block - strip_offset : req_size;

            // add sub_request to request
            printf("--> %u %u %u %u %u\n", disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn);
            raid_subreq = (struct raid_sub_request*)malloc(sizeof(struct raid_sub_request));
            alloc_assert(raid_subreq, "raid_sub_request");
            memset(raid_subreq,0,sizeof(struct raid_sub_request));
            initialize_raid_sub_request(raid_subreq, raid_req, disk_id, stripe_id, strip_id, strip_offset, disk_req_lsn, disk_req_size, req_operation);
            
            req_size_block = req_size_block - (raid->strip_size_block - strip_offset);
            if (req_size_block > 0) {
                req_size = req_size_block;
                req_lsn = req_lsn + req_size;
            }
        }

        // add request to raid request queue
        if (raid->request_queue == NULL) {
            raid->request_queue = raid_req;
            raid->request_tail = raid->request_queue;
            raid->request_queue_length++;
        } else {
            raid_req->prev_node = raid->request_tail;
            raid->request_tail->next_node = raid_req;
            raid->request_tail = raid_req;
            raid->request_queue_length++;
        }
    }

    return R_DIST_SUCCESS;
}

int raid_clear_completed_request(struct raid_info* raid) {
    struct raid_request* req_pointer, *temp_r;
    struct raid_sub_request* subreq_pointer, *temp_sr;
    int is_all_completed = 1;

    req_pointer = raid->request_queue;
    while (req_pointer != NULL){
        is_all_completed = 1;
        subreq_pointer = req_pointer->subs;
        while (subreq_pointer != NULL){
            if (subreq_pointer->current_state != R_SR_COMPLETE) is_all_completed = 0;
            subreq_pointer = subreq_pointer->next_node;
        }

        if (is_all_completed) {
            subreq_pointer = req_pointer->subs;
            while (subreq_pointer != NULL){
                temp_sr = subreq_pointer;
                subreq_pointer = temp_sr->next_node;
                free(temp_sr);
            }

            if (raid->request_queue == req_pointer) {  // first element in queue
                raid->request_queue = NULL;
                raid->request_tail = NULL;
                free(req_pointer);
                req_pointer = raid->request_queue;
            } else {
                temp_r = req_pointer->prev_node;
                (req_pointer->prev_node)->next_node = req_pointer->next_node;
                free(req_pointer);
                req_pointer = temp_r;
            }

            raid->request_queue_length -= 1;
        }

        if (req_pointer != NULL)
            req_pointer = req_pointer->next_node;
    }
    return 0;
}

int raid_simulate_ssd(struct raid_info* raid, int disk_id) {
    // iterate raid request queue, find any request for this disk
    struct ssd_info* ssd;
    struct raid_request* req_pointer;
    struct raid_sub_request* subreq_pointer;
    int flag = 0, err;

    ssd = raid->connected_ssd[disk_id];
    req_pointer = raid->request_queue;
    while(req_pointer != NULL){
        subreq_pointer = req_pointer->subs;
        while(subreq_pointer != NULL){
            if (subreq_pointer->disk_id == disk_id && subreq_pointer->current_state == R_SR_PENDING) {
                // insert to ssd queue
                err = raid_ssd_interface(ssd, subreq_pointer);
                if (err != ERROR) subreq_pointer->current_state = R_SR_PROCESS;
            }
            subreq_pointer = subreq_pointer->next_node;
        }
        req_pointer = req_pointer->next_node;
    }
    
    if (err != ERROR) {
        if (ssd->parameter->dram_capacity!=0) {
            buffer_management(ssd);
            distribute(ssd);
        } else {
            no_buffer_distribute(ssd);
        }
    }

    process(ssd);
    raid_ssd_trace_output(ssd);

    return 0;
}

int raid_ssd_interface(struct ssd_info* ssd, struct raid_sub_request *subreq) {
    int large_lsn;
    int64_t nearest_event_time;
    struct request *ssd_request;
    
    large_lsn = (int)((ssd->parameter->subpage_page*ssd->parameter->page_block*ssd->parameter->block_plane*ssd->parameter->plane_die*ssd->parameter->die_chip*ssd->parameter->chip_num)*(1-ssd->parameter->overprovide));
    subreq->lsn = subreq->lsn % large_lsn;

    nearest_event_time = find_nearest_event(ssd);
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
            latency = req->response_time - req->time;
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
                    sub = sub->next_node;
                } else {
                    is_all_sub_completed = 0;
                    break;
                }
            }
            
            if (is_all_sub_completed) {
                latency = end_time-req->time;
                
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
                    free(tmp);
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

    if (req == ssd->request_queue) {
        (req->subreq_on_raid)->current_state = R_SR_COMPLETE;
        (req->subreq_on_raid)->complete_time = req->response_time;
        req->subreq_on_raid = NULL;
        ssd->request_queue = req->next_node;

        if (req->need_distr_flag != NULL) free(req->need_distr_flag);
        free(req);

        ssd->request_queue=NULL;
        ssd->request_tail=NULL;
        ssd->request_queue_length--;
        return;
    }

    temp = ssd->request_queue;
    while (temp != NULL && temp != req) { 
        prev = temp; 
        temp = temp->next_node; 
    } 

    if (temp == NULL) return;

    prev->next_node = temp->next_node;
    (req->subreq_on_raid)->current_state = R_SR_COMPLETE;
    (req->subreq_on_raid)->complete_time = req->response_time;
    if (req->need_distr_flag != NULL) free(req->need_distr_flag);
    free(req);
    ssd->request_queue_length--;
    return;
}

struct raid_info* simulate_raid5(struct raid_info* raid) {
    printf("Work in progress for simualting raid 5 ...\n");

    // controller's request queue
    // get_raid_request(raid)
    return raid;
}