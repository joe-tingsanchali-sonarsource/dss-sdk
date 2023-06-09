/**
 # The Clear BSD License
 #
 # Copyright (c) 2022 Samsung Electronics Co., Ltd.
 # All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted (subject to the limitations in the disclaimer
 # below) provided that the following conditions are met:
 #
 # * Redistributions of source code must retain the above copyright notice, 
 #   this list of conditions and the following disclaimer.
 # * Redistributions in binary form must reproduce the above copyright notice,
 #   this list of conditions and the following disclaimer in the documentation
 #   and/or other materials provided with the distribution.
 # * Neither the name of Samsung Electronics Co., Ltd. nor the names of its
 #   contributors may be used to endorse or promote products derived from this
 #   software without specific prior written permission.
 # NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
 # THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
 # CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
 # NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 # PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 # OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 # WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 # OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 # ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 */

#ifndef NKV_FRAMEWORK_H
#define NKV_FRAMEWORK_H

/*#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <list>
#include <string>
#include <atomic>
#include <mutex>
#include <functional>*/

#include <thread>
#include "kvs_api.h"
#include "nkv_utils.h"
#include "auto_discovery.h"
#include "nkv_stats.h"
//#include "rdd_cl.h"
#include <condition_variable>
#include <pthread.h>
#include <mutex>
#include <string.h>

  #define SLEEP_FOR_MICRO_SEC 100
  #define NKV_STORE_OP      0
  #define NKV_RETRIEVE_OP   1
  #define NKV_DELETE_OP     2
  #define MAX_PATH_PER_SUBSYS 4
  #define NKV_RETRIEVE_OP_RDD 5
  #define NKV_STORE_OP_RDD 6

  #define AUTOTHROTTLING 0.7
  #define NKV_LIST_OP       3
  #define NKV_LOCK_OP       3
  #define NKV_UNLOCK_OP     4
  extern std::atomic<bool> nkv_stopping;
  extern std::atomic<uint64_t> nkv_pending_calls;
  extern std::atomic<bool> is_kvs_initialized;
  extern std::atomic<uint32_t> nkv_async_path_max_qd;
  extern std::atomic<uint64_t> nkv_num_async_submission;
  extern std::atomic<uint64_t> nkv_num_async_completion;
  extern std::atomic<uint64_t> nkv_num_read_cache_miss;
  
  //extern c_smglogger* logger;
  class NKVContainerList;
  extern NKVContainerList* nkv_cnt_list;
  extern int32_t core_pinning_required;
  extern int32_t queue_depth_monitor_required;
  extern int32_t queue_depth_threshold;
  extern int32_t listing_with_cached_keys;
  extern std::string iter_prefix;
  extern std::string transient_prefix;
  extern int32_t num_path_per_container_to_iterate;
  extern int32_t nkv_is_on_local_kv;
  extern std::string key_default_delimiter;
  extern int32_t MAX_DIR_ENTRIES;
  extern int32_t nkv_listing_wait_till_cache_init;
  extern int32_t nkv_listing_need_cache_stat;
  extern int32_t nkv_listing_cache_num_shards;
  extern int32_t nkv_dynamic_logging;
  extern std::atomic<int32_t> path_stat_collection;
  extern std::atomic<int32_t> path_stat_detailed;
  extern int32_t nkv_use_read_cache;
  extern int32_t nkv_read_cache_size;
  extern int32_t nkv_read_cache_shard_size;
  extern int32_t nkv_data_cache_size_threshold;
  extern int32_t nkv_use_data_cache;
  extern int32_t nkv_remote_listing;
  extern uint32_t nkv_max_key_length;
  extern uint32_t nkv_max_value_length;
  extern int32_t nkv_in_memory_exec;
  extern std::atomic<uint32_t> nic_load_balance;
  extern std::atomic<uint32_t> nic_load_balance_policy;
  extern int32_t nkv_device_support_iter;

  // Global lock used for boost read_json and cv_global
  extern std::mutex mtx_global;

  int32_t get_path_stat_collection();
  void set_path_stat_collection(int32_t path_stat);
 
  int32_t get_path_stat_detailed();
  void set_path_stat_detailed(int32_t path_stat);

  typedef struct iterator_info {
    std::unordered_set<uint64_t> visited_path;
    std::unordered_set<std::string> excess_keys;
    std::unordered_set<std::string> dir_entries_added;
    kvs_iterator_handle iter_handle;
    kvs_iterator_list iter_list;
    #ifndef SAMSUNG_API
      kvs_option_iterator g_iter_mode;
    #endif
    uint64_t network_path_hash_iterating;
    int32_t all_done;
    //For cached based listing
    std::unordered_set<std::string>* dup_chached_key_set;
    //std::unordered_set<std::string>::const_iterator cached_key_iter;
    //std::list<std::string>::const_iterator cached_key_iter;
    //std::vector<std::string>::const_iterator cached_key_iter;
    std::set<std::string>::const_iterator cached_key_iter;
    std::size_t key_prefix_hash;
    std::string key_to_start_iter;
    iterator_info():network_path_hash_iterating(0), all_done(0), dup_chached_key_set(NULL) {
      excess_keys.clear();
      dir_entries_added.clear();
      visited_path.clear();
      key_prefix_hash = 0;
    }

    ~iterator_info() {
      if (dup_chached_key_set) {
        delete dup_chached_key_set;
        dup_chached_key_set = NULL;
      }
    }
  } iterator_info;

  struct nkv_value_wrapper {
    void *value;
    uint64_t length;
    uint64_t actual_length;
    nkv_value_wrapper(void* pval, uint64_t plength, uint64_t pactual_length) : value(pval),
                                                                               length(plength),
                                                                               actual_length(pactual_length) {}

    ~nkv_value_wrapper() {
      if (value) {
        free (value);
        value = NULL;
      }
      length = 0;
      actual_length = 0;
    }

    nkv_value_wrapper(const nkv_value_wrapper& other) {
      //std::cout <<"In copy constructor.." << std::endl;
      value = malloc (other.length);
      memcpy(value, other.value, other.length);
      length = other.length;
      actual_length = other.actual_length;
    }


    nkv_value_wrapper (nkv_value_wrapper&& other) : value (other.value), length(other.length), actual_length(other.actual_length) {
      other.value = NULL;
      other.length = 0;
      other.actual_length = 0;
    }

    nkv_value_wrapper& operator=(nkv_value_wrapper&& other) {
      if (this != &other) {
        value = other.value;
        length = other.length;
        actual_length = other.actual_length;
        other.value = NULL;
        other.length = 0;
        other.actual_length = 0;
      }
      return *this;
    }


  };

  //Aio call back function
  void nkv_aio_completion (nkv_aio_construct* ops, int32_t num_op);

  class NKVTargetPath {
    kvs_device_handle path_handle;
    #ifdef SAMSUNG_API
      kvs_container_context path_cont_ctx;
      kvs_container_handle path_cont_handle;
    #else
      kvs_key_space_name   path_ks_name;
      kvs_key_space_handle path_ks_handle;
    #endif
    std::thread path_thread_iter;
    std::thread path_thread_cache;

	//multiple_path Load Balance use only
  public:
    std::string path_cont_name;
    std::string dev_path;
    std::string path_ip;
    std::string path_nqn;
    int32_t path_port;
    int32_t addr_family;
    int32_t path_speed;
    std::atomic<int32_t> path_status;
    int32_t path_numa_aligned;
    int32_t path_type;
    int32_t path_id;
    uint64_t path_hash;
    int32_t core_to_pin;
    int32_t path_numa_node;
    std::atomic<uint32_t> nkv_async_path_cur_qd;
    std::unordered_map<std::size_t, std::set<std::string> > *listing_keys; 
    std::unordered_map<std::string, nkv_value_wrapper*> *data_cache; 
    std::atomic<uint32_t> nkv_outstanding_iter_on_path;
    std::atomic<uint32_t> nkv_path_stopping;
    std::atomic<uint64_t> nkv_num_key_prefixes;
    std::atomic<uint32_t> nkv_num_keys;
    std::mutex cache_mtx;
    pthread_rwlock_t* cache_rw_lock_list;
    pthread_rwlock_t* data_rw_lock_list;
    std::mutex iter_mtx;
    std::vector<std::string> path_vec;
    std::atomic<uint32_t> pending_io_number;
    std::atomic<uint64_t> pending_io_size;
    std::atomic<uint64_t> pending_io_value;
    std::condition_variable cv_path;
    nkv_lruCache<std::string, nkv_value_wrapper> *cnt_cache;
    pthread_rwlock_t lru_rw_lock;
    std::mutex lru_lock;
    std::atomic<uint64_t> nkv_num_dc_keys;
    stat_io_t* device_stat;
    vector<stat_io_t*> cpu_stats;
    std::atomic<bool> cpu_stat_initialized;
    //rdd_cl_conn_ctx_t *path_rdd_conn;
    void *path_rdd_conn;
    int32_t path_enable_rdd;
  public:
    NKVTargetPath (uint64_t p_hash, int32_t p_id, std::string& p_ip, int32_t port, int32_t fam, int32_t p_speed, 
		  int32_t p_stat, int32_t numa_aligned, int32_t p_type):
                  path_ip(p_ip), path_port(port), addr_family(fam), path_speed(p_speed), 
				  path_status(p_stat), path_numa_aligned(numa_aligned), path_type(p_type), 
				  path_id(p_id), path_hash(p_hash),pending_io_number(0), pending_io_size(0), 
				  pending_io_value(0),device_stat(NULL){

      nkv_async_path_cur_qd = 0;
      core_to_pin = -1;
      path_numa_node = -1;

      nkv_outstanding_iter_on_path = 0;
      nkv_path_stopping = 0;
      nkv_num_key_prefixes = 0;
      nkv_num_keys = 0;
      cache_rw_lock_list = new pthread_rwlock_t[nkv_listing_cache_num_shards];
     
      for (int iter = 0; iter < nkv_listing_cache_num_shards; iter++) {
        pthread_rwlock_init(&cache_rw_lock_list[iter], NULL);
      }
      data_rw_lock_list = new pthread_rwlock_t[nkv_listing_cache_num_shards];

      for (int iter = 0; iter < nkv_listing_cache_num_shards; iter++) {
        pthread_rwlock_init(&data_rw_lock_list[iter], NULL);
      }

      listing_keys = new std::unordered_map<std::size_t, std::set<std::string> > [nkv_listing_cache_num_shards];
      if (nkv_in_memory_exec) {
        data_cache = new std::unordered_map<std::string, nkv_value_wrapper*> [nkv_listing_cache_num_shards];
      }
      cnt_cache = new nkv_lruCache<std::string, nkv_value_wrapper> [nkv_read_cache_shard_size](nkv_read_cache_size);
      nkv_num_dc_keys = 0;

      // ustats
      //string device_name = (dev_path.substr(5, dev_path.size()));
      //nkv_init_path_io_stats(device_stat, )
      cpu_stat_initialized = false;
      pthread_rwlock_init(&lru_rw_lock, NULL);
      path_rdd_conn = NULL;
      path_enable_rdd = 0;
    }

    ~NKVTargetPath();

    void add_device_path (std::string& p_dev_path) {
      dev_path = p_dev_path;
    }
    
    int32_t get_target_path_status() {
      return path_status.load(std::memory_order_relaxed);
    }

    void set_target_path_status(int32_t status) {
      path_status.store(status, std::memory_order_relaxed);
    }

    void reset_path_stat();
    
    bool get_cpu_stat_initialized(void) {
      return cpu_stat_initialized.load(std::memory_order_relaxed);
    }
    void set_cpu_stat_initialized(bool is_initialized) {
      cpu_stat_initialized.store(is_initialized, std::memory_order_relaxed);
    }

    bool open_path(const std::string& app_name) {
      if (nkv_in_memory_exec)
        return true;
      kvs_result ret = kvs_open_device((char*)dev_path.c_str(), &path_handle);
      if(ret != KVS_SUCCESS) {
        #ifdef SAMSUNG_API
          smg_error(logger, "Path open failed, path = %s, error = %s", dev_path.c_str(), kvs_errstr(ret));
        #else
          smg_error(logger, "Path open failed, path = %s, error = %d", dev_path.c_str(), ret);
        #endif
        return false;
      }
      smg_info(logger,"** Path open successful for path = %s **", dev_path.c_str());
      //For now device supports single container only..
      path_cont_name = "nkv_" + app_name;
      #ifdef SAMSUNG_API 
        ret = kvs_create_container(path_handle, path_cont_name.c_str(), 0, &path_cont_ctx);      
        if (ret != KVS_SUCCESS) {
          smg_error(logger, "Path container creation failed, path = %s, container name = %s, error = %s", 
                   dev_path.c_str(), path_cont_name.c_str(), kvs_errstr(ret));
          return false;
        }
      #else
        kvs_option_key_space option = {KVS_KEY_ORDER_ASCEND};
        path_ks_name.name = (char*)path_cont_name.c_str();
        path_ks_name.name_len = path_cont_name.length();
  
        kvs_create_key_space(path_handle, &path_ks_name, 0, option);

      #endif
      smg_info(logger,"** Path container creation successful for path = %s, container name = %s **", 
              dev_path.c_str(), path_cont_name.c_str());

      #ifdef SAMSUNG_API
        ret = kvs_open_container(path_handle, path_cont_name.c_str(), &path_cont_handle);
        if(ret != KVS_SUCCESS) {
          smg_error(logger, "Path open container failed, path = %s, container name = %s, error = %s",
                   dev_path.c_str(), path_cont_name.c_str(), kvs_errstr(ret));
          return false;
        }
      #else
        ret = kvs_open_key_space(path_handle, (char*)path_cont_name.c_str(), &path_ks_handle);
        if(ret != KVS_SUCCESS) {
          smg_error(logger, "Path open key_space failed, path = %s, container name = %s, error = %d",
                   dev_path.c_str(), path_cont_name.c_str(), ret);
          return false;
        }

      #endif
      smg_info(logger,"** Path open container successful for path = %s, container name = %s **",
              dev_path.c_str(), path_cont_name.c_str());


      return true;
    }
 
    /* Function Name: close_path
     * Input Args   : None
     * Return       : <bool> = Path close success/failure
     * Description  : Close the kv device path opened through openMPDK driver.
     */ 
    bool close_path(){
      if (nkv_in_memory_exec)
        return true;
      smg_warn(logger, "Closing kvs_device_path %s", dev_path.c_str());
      kvs_result ret;
      #ifdef SAMSUNG_API
        ret = kvs_close_container(path_cont_handle);
        if ( ret != KVS_SUCCESS ) {
          smg_error(logger, "KVS container close failed for %s", dev_path.c_str());
        }
      #else
        kvs_close_key_space(path_ks_handle);
        kvs_delete_key_space(path_handle, &path_ks_name);
      #endif

      ret = kvs_close_device(path_handle);
      if ( ret != KVS_SUCCESS ) {
        smg_error(logger, "KVS path close failed for %s", dev_path.c_str());
        return false;     
      }
      return true;
    }
 
    nkv_result map_kvs_err_code_to_nkv_err_code (int32_t kvs_code);
    nkv_result do_store_io_to_path (const nkv_key* key, const nkv_store_option* opt, nkv_value* value, nkv_postprocess_function* post_fn,
                                    uint32_t client_rdma_key, uint16_t client_rdma_qhandle); 
    nkv_result do_retrieve_io_from_path (const nkv_key* key, const nkv_retrieve_option* opt, nkv_value* value, nkv_postprocess_function* post_fn,
                                         uint32_t client_rdma_key, uint16_t client_rdma_qhandle); 
    nkv_result do_delete_io_from_path (const nkv_key* key, nkv_postprocess_function* post_fn); 
    nkv_result do_lock_io_from_path (const nkv_key* key,
		const nkv_lock_option *opt, nkv_postprocess_function* post_fn); 
    nkv_result do_unlock_io_from_path (const nkv_key* key,
		const nkv_unlock_option *opt, nkv_postprocess_function* post_fn); 
    nkv_result do_list_keys_from_path(uint32_t* num_keys_iterted, iterator_info*& iter_info, uint32_t* max_keys, nkv_key* keys, const char* prefix,
                                     const char* delimiter, const char* start_after); 
    nkv_result find_keys_from_path(uint32_t* max_keys, nkv_key* keys, iterator_info*& iter_info, uint32_t* num_keys_iterted, const char* prefix,
                                      const char* delimiter); 
    void filter_and_populate_keys_from_path(uint32_t* max_keys, nkv_key* keys, char* disk_key, uint32_t key_size, uint32_t* num_keys_iterted,
                                            const char* prefix, const char* delimiter, iterator_info*& iter_info, bool cached_keys = false);
    nkv_result perform_remote_listing(const char* key_prefix_iter, const char* start_after, uint32_t* max_keys, 
                                      nkv_key* keys, iterator_info*& iter_info, uint32_t* num_keys_iterted);
    int32_t parse_delimiter_entries(std::string& key, const char* delimiter, std::vector<std::string>& dirs,
                                    std::vector<std::string>& prefixes, std::string& f_name);
    void populate_iter_cache(std::string& key_prefix, std::string& key_prefix_val, bool need_lock = true);
    void populate_value_cache(std::string& key_str, nkv_value* n_value);
    void delete_from_value_cache(std::string& key_str);
    nkv_result retrieve_from_value_cache(std::string& key_str, nkv_value* n_value);
    bool remove_from_iter_cache(std::string& key_prefix, std::string& key_prefix_val, bool root_prefix = false);
    void nkv_path_thread_func(int32_t what_work);
    void nkv_path_thread_init(int32_t what_work);

    void start_thread(int32_t what_work = 0) {
      if (nkv_in_memory_exec)
        return;
      path_thread_iter = std::thread(&NKVTargetPath::nkv_path_thread_func, this, what_work);
      path_thread_cache = std::thread(&NKVTargetPath::nkv_path_thread_init, this, what_work);
      
    }
    void wait_for_thread_completion() {
      if (nkv_in_memory_exec)
        return;
      try {
        if (path_thread_iter.joinable()) {
          path_thread_iter.join();
        }
        nkv_path_stopping.fetch_add(1, std::memory_order_relaxed);
        if (path_thread_cache.joinable()) {
          cv_path.notify_one();
          path_thread_cache.join();
        }

      } catch(...) {
        smg_warn(logger,"Exception during pthread_join() on dev_path = %s, ip = %s, may be thread is not running ?",
                 dev_path.c_str(), path_ip.c_str());
      }
    }
    void detach_thread_completion() {
      if (nkv_in_memory_exec)
        return;
      path_thread_iter.detach();
      path_thread_cache.detach();
    }
    int32_t initialize_iter_cache (iterator_info*& iter_info);
	
    void load_balance_update_path_parameter(uint64_t size, int is_completed) {
      if (is_completed == 0) {
	this->pending_io_number.fetch_add(1, std::memory_order_relaxed);
	this->pending_io_size.fetch_add(size, std::memory_order_relaxed);
      } else {
	this->pending_io_number.fetch_sub(1, std::memory_order_relaxed);
	this->pending_io_size.fetch_sub(size, std::memory_order_relaxed);
      }
    }
  };

  class NKVTarget {
  protected:	
    //Multiple Load Balance use only
    uint64_t verify_path;
    uint64_t preferred_path_hash;
    uint64_t path_array[MAX_PATH_PER_SUBSYS];
    std::atomic<uint64_t> lb_rr_count;
    int path_count;
  public:
    //<ip_address hash, path> pair
    std::unordered_map<uint64_t, NKVTargetPath*> pathMap;
    //Incremental Id
    uint32_t t_id;
    //Could be Hash of target_node_name:target_container_name if not coming from FM
    std::string target_uuid;
    std::string target_node_name;
    std::string target_container_name;
    int32_t ss_status;
    float ss_space_avail_percent;
    //Generated from target_uuid and passed to the app
    uint64_t target_hash;
    NKVTarget(uint32_t p_id, const std::string& puuid, const std::string& tgtNodeName, const std::string& tgtCntName, uint64_t t_hash) : 
             verify_path(0), preferred_path_hash(0), lb_rr_count(0), path_count(0), t_id(p_id), 
	     target_uuid(puuid), target_node_name(tgtNodeName), target_container_name(tgtCntName), 
	     target_hash(t_hash) {
	       for(int i = 0; i < MAX_PATH_PER_SUBSYS; i++) {
		 path_array[i] = 0;
	       }
	     }

    ~NKVTarget() {
      for (auto m_iter = pathMap.begin(); m_iter != pathMap.end(); m_iter++) {
        delete(m_iter->second);
      }
      pathMap.clear();  
    }

    std::unordered_map<uint64_t, NKVTargetPath*>& get_path_map() {
        return pathMap;
    }

    void set_ss_status (int32_t p_status) {
      ss_status = p_status;
    }

    int32_t get_ss_status () {
      return ss_status;
    } 
    
    void set_space_avail_percent (float p_space) {
      ss_space_avail_percent = p_space;
    } 

    nkv_result  send_io_to_path(uint64_t container_path_hash, const nkv_key* key, 
                                void* opt, nkv_value* value, int32_t which_op, nkv_postprocess_function* post_fn,
                                uint32_t client_rdma_key, uint16_t client_rdma_qhandle) {
      nkv_result status = NKV_SUCCESS; 
      NKVTargetPath* one_p = NULL;
      std::unordered_set<uint64_t> visited_path;
      uint32_t max_retry_count = path_count;
      int stop_flag = 0;

      if (nic_load_balance && container_path_hash != verify_path) {
	smg_error(logger, "The container path hash %u not exists in the path)", container_path_hash);
	return NKV_ERR_CNT_VERIFY_FAILED;
      }

      do {
        if(nic_load_balance) {
	  container_path_hash = this->load_balance_get_path(visited_path, stop_flag);
	  if(stop_flag == 1) {
            smg_error(logger,"Load Balancer cannot get a valid path for container name = %s,"
		 "target node = %s, nkvtarget hash %u", target_container_name.c_str(), 
		 target_node_name.c_str(), this->target_hash);
	      return NKV_ERR_NO_CNT_PATH_FOUND;
	  }
        }  
      	
        auto p_iter = pathMap.find(container_path_hash);
        if ( !nic_load_balance && p_iter == pathMap.end()) {
          smg_error(logger,"Load Balancer cannot get a valid path for container name = %s, container hash = %u,"
		 "target node = %s", target_container_name.c_str(), container_path_hash, 
		target_node_name.c_str());
       	  return NKV_ERR_NO_CNT_PATH_FOUND;
        }
      
        one_p = p_iter->second;

        // Fix checking path status before sending IO to that path
        if (one_p) {
          if (! one_p->get_target_path_status() ) {
            smg_error(logger, "Container Path down !!, Path ip=%s dev_path=%s status = %d", one_p->path_ip.c_str(),
                      one_p->dev_path.c_str(), one_p->get_target_path_status());
            status = NKV_ERR_CNT_PATH_DOWN;
            continue;
          }
          if (!post_fn) {
            smg_info(logger, "Sending IO to dev mount = %s, container name = %s, target node = %s, path ip = %s,"
		  "path port = %d, key = %s, key_length = %u, op = %d, cur_qd = %u", 
                  one_p->dev_path.c_str(), target_container_name.c_str(), target_node_name.c_str(), one_p->path_ip.c_str(), 
                  one_p->path_port, (char*)key->key, key->length, which_op, one_p->nkv_async_path_cur_qd.load());
          } else {
            smg_info(logger, "Sending IO to dev mount = %s, container name = %s, target node = %s, path ip = %s," 
		  "path port = %d, key = %s, key_length = %u, op = %d, cur_path_qd = %u, max_path_qd = %u",
                  one_p->dev_path.c_str(), target_container_name.c_str(), target_node_name.c_str(), one_p->path_ip.c_str(),
                  one_p->path_port, (char*)key->key, key->length, which_op, one_p->nkv_async_path_cur_qd.load(), nkv_async_path_max_qd.load());
            /*smg_warn(logger, "Sending IO to dev mount = %s, key = %s, op = %d, cur_qd = %u, cb_address = 0x%x, post_fn = 0x%x, pvt_1 = 0x%x",
                  one_p->dev_path.c_str(), (char*)key->key, which_op, one_p->nkv_async_path_cur_qd.load(), post_fn->nkv_aio_cb, post_fn, post_fn->private_data_1);*/
          }

 	  if (nic_load_balance) {
	    if (which_op == NKV_STORE_OP || which_op == NKV_RETRIEVE_OP){
	      one_p->load_balance_update_path_parameter(value->length, 0);
	    } else if (which_op == NKV_DELETE_OP || which_op == NKV_LOCK_OP || which_op == NKV_UNLOCK_OP) {
	      one_p->load_balance_update_path_parameter(0, 0);	
	    }
	  }

          int core = -1;
          uint64_t v_len = 0;
          switch(which_op) {
            case NKV_STORE_OP:
            case NKV_STORE_OP_RDD:
              if (!post_fn && get_path_stat_collection()) {
                v_len = value->length;
                core = sched_getcpu();
                assert(core >= 0);
                stat_io_t* cpu_stat = NULL;  // stat_io_t
                if( one_p->get_cpu_stat_initialized() ) {
                  cpu_stat = one_p->cpu_stats[core];  // stat_io_t
                }
                if (value->length <= 4096) {
                  nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->put_less_4KB);
                  nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->put_less_4KB);
                } else if (value->length > 4096 && value->length <= 65536) {
                  nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->put_4KB_64KB);
                  nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->put_4KB_64KB);
                } else {
                  nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->put_64KB_2MB);
                  nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->put_64KB_2MB);
                }
                
                nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->put);
                nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->put);
              }
              status = one_p->do_store_io_to_path(key, (const nkv_store_option*)opt, value, post_fn,
                                                        client_rdma_key, client_rdma_qhandle);
              break;
            case NKV_RETRIEVE_OP:
            case NKV_RETRIEVE_OP_RDD:

              if (!post_fn && get_path_stat_collection()) {
                v_len = value->length;
                core = sched_getcpu();
                assert(core >= 0);
                stat_io_t* cpu_stat = NULL;  // stat_io_t
                if( one_p->get_cpu_stat_initialized() ) {
                  cpu_stat = one_p->cpu_stats[core];  // stat_io_t
                }
                if (value->length <= 4096) {
                  if( cpu_stat ) {
                    nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->get_less_4KB);
                  }
                  nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->get_less_4KB);
                } else if (value->length > 4096 && value->length <= 65536) {
                  if( cpu_stat ) {
                    nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->get_4KB_64KB);
                  }
                  nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->get_4KB_64KB);
                } else {
                  if( cpu_stat) {
                    nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->get_64KB_2MB);
                  }
                  nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->get_64KB_2MB);
                }
                if( cpu_stat){
                  nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->get);
                } 
                nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->get);
              }
              status = one_p->do_retrieve_io_from_path(key, (const nkv_retrieve_option*)opt, value, post_fn, 
                                                             client_rdma_key, client_rdma_qhandle);
              break;
            case NKV_DELETE_OP:
              if (!post_fn && get_path_stat_collection()) {
                core = sched_getcpu();
                assert(core >= 0);
                if( one_p->get_cpu_stat_initialized() ) {
                  stat_io_t* cpu_stat = one_p->cpu_stats[core];  // stat_io_t
                  nkv_ustat_atomic_inc_u64(cpu_stat, &cpu_stat->del);
                }
                nkv_ustat_atomic_inc_u64(one_p->device_stat, &one_p->device_stat->del);
              }
              status = one_p->do_delete_io_from_path(key, post_fn);
              break;
            case NKV_LOCK_OP:
              status = one_p->do_lock_io_from_path(key, \
						(const nkv_lock_option*)opt, post_fn);
              break;
            case NKV_UNLOCK_OP:
              status = one_p->do_unlock_io_from_path(key, \
						(const nkv_unlock_option*)opt, post_fn);
              break;
            default:
              smg_error(logger, "Unknown op, op = %d", which_op);
              return NKV_ERR_WRONG_INPUT;
          }  

	  if (!post_fn && nic_load_balance) {
	    if (which_op == NKV_STORE_OP || which_op == NKV_RETRIEVE_OP){
	      one_p->load_balance_update_path_parameter(value->length, 1);
	    } else if (which_op == NKV_DELETE_OP || which_op == NKV_LOCK_OP || which_op == NKV_UNLOCK_OP) {
	      one_p->load_balance_update_path_parameter(0, 1);	
	    }
	  }

          if (!post_fn && get_path_stat_collection()) {
            stat_io_t* cpu_stat = NULL;
            core = sched_getcpu();
            assert(core >= 0);
            if( one_p->get_cpu_stat_initialized() ) {
              cpu_stat = one_p->cpu_stats[core];  // stat_io_t
            }

            switch (which_op) {
              case NKV_STORE_OP:
                //core = sched_getcpu();   
                //assert(core >= 0);
                if (v_len <= 4096) {
                  nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->put_less_4KB);
                  nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->put_less_4KB);
                } else if (v_len > 4096 && v_len <= 65536) {
                  nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->put_4KB_64KB);
                  nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->put_4KB_64KB);
                } else {
                  nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->put_64KB_2MB);
                  nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->put_64KB_2MB);
                }

                nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->put);
                nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->put);
                break;
              case NKV_RETRIEVE_OP:
                //core = sched_getcpu();
                //assert(core >= 0);
                if (v_len <= 4096) {
                  nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->get_less_4KB);
                  nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->get_less_4KB);
                } else if (v_len > 4096 && v_len <= 65536) {
                  nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->get_4KB_64KB);
                  nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->get_4KB_64KB);
                } else {
                  nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->get_64KB_2MB);
                  nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->get_64KB_2MB);
                }
                nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->get);
                nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->get);
                break;

              case NKV_DELETE_OP:
                //core = sched_getcpu();
                //assert(core >= 0);
                nkv_ustat_atomic_dec_u64(cpu_stat, &cpu_stat->del);
                nkv_ustat_atomic_dec_u64(one_p->device_stat, &one_p->device_stat->del);

            }
          }
        } else {
          smg_error(logger, "NULL Container path found for hash = %u!!", container_path_hash);
          return NKV_ERR_NO_CNT_PATH_FOUND;
        }
        /*if (status == NKV_SUCCESS && post_fn)
          nkv_num_async_submission.fetch_add(1, std::memory_order_relaxed);*/

      } while ( nic_load_balance && !post_fn && status == NKV_ERR_IO && visited_path.size() < max_retry_count);
      
      return status;
    }
    
    nkv_result get_path_mount_point (uint64_t container_path_hash, std::string& p_mount) {

      nkv_result status = NKV_SUCCESS;
      auto p_iter = pathMap.find(container_path_hash);
      if (p_iter == pathMap.end()) {
        smg_error(logger,"No Container path found for hash = %u", container_path_hash);
        return NKV_ERR_NO_CNT_PATH_FOUND;
      }
      NKVTargetPath* one_p = p_iter->second;
      if (one_p) {
        p_mount = one_p->dev_path;
      } else {
        smg_error(logger, "NULL Container path found for hash = %u!!", container_path_hash);
        return NKV_ERR_NO_CNT_PATH_FOUND;
      }
      return status;
    }

    nkv_result list_keys_from_path (uint64_t container_path_hash, uint32_t* max_keys, nkv_key* keys, void*& iter_context, const char* prefix,
                                    const char* delimiter, const char* start_after) {

      nkv_result stat = NKV_SUCCESS;
      uint64_t current_path_hash = 0;
      uint32_t num_keys_iterted = 0;

      iterator_info* iter_info = NULL;
      if (iter_context) {
        iter_info = (iterator_info*) iter_context;
        current_path_hash = iter_info->network_path_hash_iterating; 
        if (iter_info->excess_keys.size() != 0) {
          smg_info(logger, "Got some excess keys from last call, adding to output buffer, number of keys = %u, container name = %s, target node = %s",
                   iter_info->excess_keys.size(), target_container_name.c_str(), target_node_name.c_str()); 
          auto k_iter = iter_info->excess_keys.begin();
          for(; k_iter != iter_info->excess_keys.end(); k_iter++) {
            (*k_iter).copy((char*)keys[num_keys_iterted].key, (*k_iter).length());
            keys[num_keys_iterted].length = (*k_iter).length();
            num_keys_iterted++;
            if (num_keys_iterted >= *max_keys) {
              smg_warn(logger, "Output buffer full, returning the call, max_keys = %u, key_inserted = %u, container name = %s, target node = %s",
                       *max_keys, num_keys_iterted, target_container_name.c_str(), target_node_name.c_str());
              break;
            }
          }
          iter_info->excess_keys.erase(iter_info->excess_keys.begin(), k_iter);
        }
      } else {
        iter_info = new iterator_info();
        assert(iter_info != NULL);
        /*iter_info->network_path_hash_iterating = 0;
        iter_info->all_done = 0;
        iter_info->excess_keys.clear();
        iter_info->dir_entries_added.clear();
        iter_info->visited_path.clear();*/
        smg_info(logger, "Created an iterator context for NKV iteration, container name = %s, target node = %s, prefix = %s, delimiter = %s",
                 target_container_name.c_str(), target_node_name.c_str(), prefix, delimiter);

        iter_context = (void*) iter_info;
      }

      if ((current_path_hash != 0) && (num_keys_iterted < *max_keys)) {
        smg_info(logger, "Start iterating on iter_context path = %u, container name = %s, target node = %s", 
                 current_path_hash, target_container_name.c_str(), target_node_name.c_str());
        auto p_iter = pathMap.find(current_path_hash);
        if (p_iter == pathMap.end()) {
          smg_error(logger,"No Container path found for hash = %u, container name = %s, target node = %s", 
                    current_path_hash, target_container_name.c_str(), target_node_name.c_str());
          return NKV_ERR_NO_CNT_PATH_FOUND;
        }

        NKVTargetPath* one_p = p_iter->second;
        assert(one_p != NULL);
        stat = one_p->do_list_keys_from_path(&num_keys_iterted, iter_info, max_keys, keys, prefix, delimiter, start_after);
        if (stat != NKV_SUCCESS) {
          *max_keys = num_keys_iterted;
          if (stat != NKV_ITER_MORE_KEYS) {
            if (iter_info) {
              delete(iter_info);
              iter_info = NULL;
            }
            smg_error(logger,"Path iteration failed on dev mount = %s, path ip = %s , error = %x",
                     one_p->dev_path.c_str(), one_p->path_ip.c_str(), stat);
            return stat;
          } else {
            smg_warn(logger,"Out buffer exhausted on dev mount = %s, path ip = %s , error = %x",
                     one_p->dev_path.c_str(), one_p->path_ip.c_str(), stat);
            return stat;
          } 
        }
        
      }

      if ((container_path_hash != 0) && (num_keys_iterted < *max_keys)) {
        smg_info(logger, "Start iterating on container path hash= %u, container name = %s, target node = %s", 
                 container_path_hash, target_container_name.c_str(), target_node_name.c_str());
        auto p_iter = pathMap.find(container_path_hash);
        if (p_iter == pathMap.end()) {
          smg_error(logger,"No Container path found for hash = %u, container name = %s, target node = %s", 
                    container_path_hash, target_container_name.c_str(), target_node_name.c_str());
          return NKV_ERR_NO_CNT_PATH_FOUND;
        }
        
        NKVTargetPath* one_p = p_iter->second; 
        assert(one_p != NULL);
        stat = one_p->do_list_keys_from_path(&num_keys_iterted, iter_info, max_keys, keys, prefix, delimiter, start_after);
        if (stat != NKV_SUCCESS) {
          *max_keys = num_keys_iterted;
          if (stat != NKV_ITER_MORE_KEYS) {
            smg_error(logger,"Path iteration failed dev mount = %s, path ip = %s , error = %x",
                      one_p->dev_path.c_str(), one_p->path_ip.c_str(), stat);
            if (iter_info) {
              delete(iter_info);
              iter_info = NULL;
            }
            return stat;
          } else {
            smg_warn(logger,"Out buffer exhausted on dev mount = %s, path ip = %s , error = %x",
                    one_p->dev_path.c_str(), one_p->path_ip.c_str(), stat);
            return stat;
          }
        }
        iter_info->all_done = 1;
      }

      if ((num_path_per_container_to_iterate > 0) && (num_path_per_container_to_iterate == (int32_t)iter_info->visited_path.size())) {
        smg_info(logger, "Finished iterating the number of path (%d) provided via config option (%d), exiting..",
                  iter_info->visited_path.size(), num_path_per_container_to_iterate);
        iter_info->all_done = 1;
      }

      if ((num_keys_iterted < *max_keys) && (0 == iter_info->all_done)) {
        smg_info(logger, "No Path provided, listing for all path(s)/device(s), container name = %s, target node = %s",
                 target_container_name.c_str(), target_node_name.c_str());

        for (auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
          NKVTargetPath* one_p = p_iter->second;
          assert(one_p != NULL);
          smg_info(logger,"Start Iterating over path hash = %u, dev mount = %s, path ip = %s", one_p->path_hash,
                   one_p->dev_path.c_str(), one_p->path_ip.c_str());
          stat = one_p->do_list_keys_from_path(&num_keys_iterted, iter_info, max_keys, keys, prefix, delimiter, start_after);
          
          if (stat != NKV_SUCCESS) {
            *max_keys = num_keys_iterted;
            if (stat != NKV_ITER_MORE_KEYS) {
              if (iter_info) {
                delete(iter_info);
                iter_info = NULL;
              }
              smg_error(logger,"Path iteration failed on dev mount = %s, path ip = %s , error = %x",
                        one_p->dev_path.c_str(), one_p->path_ip.c_str(), stat);

              return stat;
            } else {
              smg_warn(logger,"Out buffer exhausted on dev mount = %s, path ip = %s , error = %x",
                       one_p->dev_path.c_str(), one_p->path_ip.c_str(), stat);
              break;
            }
          }
          if (num_keys_iterted >= *max_keys) {
            smg_warn(logger, "Output buffer full, returning the call from path iteration, max_keys = %u, key_inserted = %u, container name = %s, target node = %s",
                     *max_keys, num_keys_iterted, target_container_name.c_str(), target_node_name.c_str());
            break;
          }

          if ((num_path_per_container_to_iterate > 0) && (num_path_per_container_to_iterate == (int32_t)iter_info->visited_path.size())) {
            smg_info(logger, "Finished iterating the number of path (%d) provided via config option (%d), exiting..",
                      iter_info->visited_path.size(), num_path_per_container_to_iterate);
            iter_info->all_done = 1;  
          }
          
        }
      }
      *max_keys = num_keys_iterted;
      if ((iter_info->visited_path.size() == pathMap.size()) || (iter_info->all_done)) {
        smg_info(logger, "Iteration is successfully completed for container name = %s, target node = %s, completed_paths = %d, all_done = %d, prefix = %s, delimiter = %s",
                  target_container_name.c_str(), target_node_name.c_str(), iter_info->visited_path.size(), iter_info->all_done, prefix, delimiter);
        if (iter_info) {
          delete(iter_info);
          iter_info = NULL;
        }
        stat = NKV_SUCCESS;
      } else {
        stat = NKV_ITER_MORE_KEYS;  
      }
      
      return stat;
    }    

    void add_network_path(NKVTargetPath* path, uint64_t ip_hash) {
      if (path) {
        auto cnt_p_iter = pathMap.find(ip_hash);
        assert (cnt_p_iter == pathMap.end());
        pathMap[ip_hash] = path;
        smg_info (logger, "Network path  added, ip hash = %u, target_container_name = %s, total path so far = %d", 
                 ip_hash, target_container_name.c_str(), pathMap.size()); 
      }
    }

    // Device level path stats
    void add_nkv_path_stat();
    // CPU level path stats for each device
    void add_nkv_path_cpu_stat();
    void reset_path_stat();
    void remove_path_stat();
    void remove_path_cpu_stat();

    void wait_or_detach_path_thread(bool will_wait) {

      for (auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
        NKVTargetPath* one_path = p_iter->second;
        if (one_path) {
          smg_info(logger, "wait_or_detach_path_thread for path with address = %s , dev_path = %s, port = %d, status = %d",
                   one_path->path_ip.c_str(), one_path->dev_path.c_str(), one_path->path_port, (one_path->path_status).load(std::memory_order_relaxed));

          if (will_wait) {
            one_path->wait_for_thread_completion();          
          } else {
            one_path->detach_thread_completion();
          }
 
        } else {
          smg_error(logger, "NULL path found !!");
          assert(0);
        }
      }

    }

    /* Function Name: verify_min_path_exists
     * Input Args   : <int32_t> = Minimum paths required for a subsystem to
     *                statisfy min path topology.
     * Return       : <bool> = 1/0 = Min Path topology statisfied/ Failed
     * Description  : Check if minimum number of paths available to satisfy 
     *                min topology subsystem paths requirement.
     */
    bool verify_min_path_exists(int32_t min_path_required) {
      if (pathMap.size() < (uint32_t) min_path_required) {
        smg_error(logger, "Not enough container path, minimum required = %d, available = %d",
                 min_path_required, pathMap.size());
        return false;
      }
      bool subsystem_status = true;
      smg_debug(logger, "Minimum subsystem paths required = %d, available = %d",
                 min_path_required, pathMap.size());
      uint32_t device_path_count = 0;
      for (auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
        NKVTargetPath* one_path = p_iter->second;
        if (one_path) {
          smg_debug(logger, "Inspecting path with address = %s , dev_path = %s, port = %d, status = %d",
                   one_path->path_ip.c_str(), one_path->dev_path.c_str(), one_path->path_port, one_path->get_target_path_status());
          // Check path_status as well
          if (one_path->get_target_path_status() && ! one_path->path_ip.empty()
              && ! one_path->dev_path.empty()) {
            device_path_count++;
          } else {
            smg_error(logger, "Path DOWN address = %s , dev_path = %s, status = %d",
                      one_path->path_ip.c_str(), one_path->dev_path.c_str(),
                      one_path->get_target_path_status());
          }
           
        } else {
          smg_error(logger, "NULL path found !!");
        }
      }
      uint32_t total_qd = nkv_async_path_max_qd.load();
      nkv_async_path_max_qd = (total_qd/pathMap.size());

      smg_alert(logger, "Max QD per path = %u", nkv_async_path_max_qd.load());

      if ( device_path_count < (uint32_t) min_path_required ) {
        subsystem_status = false;
      }
      return subsystem_status;

    }

    int32_t add_mount_point_to_path(std::string ip, std::string m_point, int32_t numa, int32_t core) {
     
      uint64_t ip_hash = std::hash<std::string>{}(ip);

      auto p_iter = pathMap.find(ip_hash);
      if (p_iter == pathMap.end()) {
        smg_error(logger,"No path found for ip = %s, node = %s, container = %s", ip.c_str(), target_node_name.c_str(),
                 target_container_name.c_str());
        return 1; 
      }
      NKVTargetPath* one_path = p_iter->second;
      if (!one_path) {
        smg_error(logger,"NULL path found for ip = %s, node = %s, container = %s", ip.c_str(), target_node_name.c_str(),
                 target_container_name.c_str());        
        return 1;
      }
      one_path->add_device_path(m_point);
      one_path->path_numa_node = numa;
      one_path->core_to_pin = core;
      
	return 0;
    }

    /* Function Name: open_paths
     * Input        : <const std::string> = application name 
     * Return       : <bool> Return success (true) / failure(false)
     * Description  : Iterate over all the paths in the container and open those paths.
     *                Success: if atleast one path is opend in the container.
     */ 
    bool open_paths(const std::string& app_name) {
      bool is_container_valid = false;
      for (auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
        NKVTargetPath* one_path = p_iter->second;
        if (one_path) {
          smg_debug(logger, "Opening path with address = %s , dev_path = %s, port = %d, status = %d",
                   one_path->path_ip.c_str(), one_path->dev_path.c_str(), one_path->path_port,one_path->get_target_path_status());
 
          if (one_path->get_target_path_status()) { 
            if (one_path->open_path(app_name)) {
              if (listing_with_cached_keys && !nkv_remote_listing && nkv_device_support_iter ) {
                one_path->start_thread();
              }
              is_container_valid = true;
            } else {
              smg_error(logger, "Opening path failed, address = %s , dev_path = %s, port = %d, status = %d",
                       one_path->path_ip.c_str(), one_path->dev_path.c_str(), one_path->path_port,one_path->get_target_path_status());              
              one_path->set_target_path_status(0);
            }
          }

        } else {
          smg_error(logger, "NULL path found while opening path !!");
        }
      }
      return is_container_valid;
    }

    int32_t populate_path_info(nkv_container_transport  *transportlist) {

      uint32_t cur_pop_index = 0;
      for (auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
        NKVTargetPath* one_path = p_iter->second;
        if (one_path) {
          transportlist[cur_pop_index].network_path_id = one_path->path_id;
          transportlist[cur_pop_index].network_path_hash = one_path->path_hash;
          transportlist[cur_pop_index].port = one_path->path_port;
          transportlist[cur_pop_index].addr_family = one_path->addr_family;
          transportlist[cur_pop_index].speed = one_path->path_speed;
          transportlist[cur_pop_index].status = (one_path->path_status).load(std::memory_order_relaxed);
          one_path->path_ip.copy(transportlist[cur_pop_index].ip_addr, one_path->path_ip.length());
          one_path->dev_path.copy(transportlist[cur_pop_index].mount_point, one_path->dev_path.length());
          one_path->path_nqn.copy(transportlist[cur_pop_index].nqn_name, one_path->path_nqn.length());

          cur_pop_index++;
	  if(nic_load_balance && one_path->path_status) {
	    verify_path = one_path->path_hash;
	    break;
	  }
 
        } else {
          smg_error(logger, "NULL path found while opening path !!");
          return 0;
        }
      }
      if (cur_pop_index == 0) {
        smg_error(logger, "No Path found for the container = %s", target_container_name.c_str());
        return 0;
      }
      return cur_pop_index;
            
    }

    // Move to the protected - shouldn't be accessed by external
    // Easy to extend if pre-select way is needed
    uint64_t load_balance_get_path(std::unordered_set<uint64_t> &visited, int& flag); /*{
      int visited_path_count = visited.size();
      uint64_t selected_path = 0;

      while (1) {
	if (visited_path_count >= path_count)
  	  break;

	int err = load_balance_execute(selected_path);
	if(err) {
	  flag = 1;
	  return 0;
	}
	if (visited.count(selected_path)) {
	  continue;
	}
			
	auto path = pathMap.find(selected_path);
	if( path == pathMap.end()) {
	  flag = 1;
	  return 0;
	} else {
	  visited.insert(selected_path);
	  visited_path_count ++;
	  if(!path->second->path_status) {
	    continue;
	  }
	  break;
	}
      }

      return selected_path;
    }*/

    int load_balance_execute (uint64_t &path) {
      int ret = 0;
      uint64_t idx = 0;
      NKVTargetPath* p = NULL;
      switch(nic_load_balance_policy) {
	// By default, it's RR policy
	case 0:	
	case 1:
	  idx = lb_rr_count.load()%path_count;
	  lb_rr_count.fetch_add(1, std::memory_order_relaxed);
	  path = path_array[idx];
	  break;
	// Failover policy
	case 2:
	  // Make sure that preferred path must be valid inside pathMap. Guarantee before. not here
	  // TODO: path selection based on different nic bandwidth 
	  p = pathMap.find(preferred_path_hash)->second;
	  if (!p->path_status) {
      	    for (auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
	      uint64_t lookup_hash = p_iter->first;
	      if ( lookup_hash == preferred_path_hash) {
		continue;
	      }

	      if (p_iter->second->path_status) {
		preferred_path_hash = lookup_hash;
		break;
	      }
	    }
	  }
				
	  path = preferred_path_hash;
	  break;
	// Least Queue Depth
	case 3:
          ret = load_balancer_helper_get_nextPath(path, 0);
	  break;
	// Least Block Size
	case 4:
	  ret = load_balancer_helper_get_nextPath(path, 1);
	  break;
	default:
          smg_error(logger, "NKV multipath policy input is invalid! ");
	  ret = 1;
	  break;
      }

      return ret;
    }

    int load_balancer_helper_get_nextPath(uint64_t &path, int type) {
      if (type != 0 && type != 1) {
        smg_error(logger, "NKV multipath is not properly executed!");
	return 1;	 	
      }

      int ret = 0;	  
      
      auto p_iter = pathMap.begin();
      uint64_t mn_path{p_iter->first};
      uint64_t mn{0};
      if(type == 0) mn = p_iter->second->pending_io_number;
      else mn = p_iter->second->pending_io_size;
      ++p_iter;

      for (; p_iter != pathMap.end(); ++p_iter) {
	auto curr_path = p_iter->second;
	if (type == 0 && mn > curr_path->pending_io_number) {
	  mn_path = p_iter->first;
	  mn = curr_path->pending_io_number;
	}
	else if(type == 1 && mn > curr_path->pending_io_size) {
	  mn_path = p_iter->first;
	  mn = curr_path->pending_io_size;
	} 
      }

      path = mn_path;
      return ret;

    }
  
    void load_balance_initialize_cnt() {
      int index = 0;

      for(auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
	if (index == 0) {
	  preferred_path_hash = p_iter->first;
	}
	path_array[index++] = p_iter->first;
      } 
      
      path_count = index;
      return;
    }

    void show_listing_stat() {
      for(auto p_iter = pathMap.begin(); p_iter != pathMap.end(); p_iter++) {
        NKVTargetPath* one_path = p_iter->second;
        if (one_path) {
          smg_alert(logger, "Cache based listing = %d, number of listing cache shards = %d, total number of listing cached keys = %u, total number of listing cached prefixes = %u, dev_path = %s",
                    listing_with_cached_keys, nkv_listing_cache_num_shards, one_path->nkv_num_keys.load(), one_path->nkv_num_key_prefixes.load(), one_path->dev_path.c_str());
        }
      }
    }

    int32_t get_object_async(const char* key, uint32_t key_len,  char* buff, uint32_t buff_len, void* cb);
    int32_t put_object_async(const char* key, uint32_t key_len, char* buff, uint32_t buff_len, void* cb, bool is_idempotent);
    int32_t del_object_async(const char* key, uint32_t key_len, void* cb);

  };

  class NKVContainerList {
    std::unordered_map<uint64_t, NKVTarget*> cnt_list;
    //std::unordered_map<std::size_t, NKVTarget*> cnt_list;
    std::atomic<uint64_t> cache_version;
    std::string app_name;
    uint64_t instance_uuid;
    uint64_t nkv_handle;
    ustat_handle_t* nkv_ustat_handle;
    //struct rdd_client_ctx_s *g_rdd_cl_ctx;
    void *g_rdd_cl_ctx;
  public:
    NKVContainerList(uint64_t latest_version, const char* a_uuid, uint64_t ins_uuid, uint64_t n_handle): 
                    cache_version(latest_version), app_name(a_uuid), instance_uuid(ins_uuid), 
                    nkv_handle(n_handle), nkv_ustat_handle(NULL), g_rdd_cl_ctx(NULL) {


    }
    ~NKVContainerList(); 

    uint64_t get_nkv_handle() {
      return nkv_handle;
    }

    int32_t get_container_count() {
      return cnt_list.size();
    }

    const std::string& get_nkv_app_name() {
      return app_name;
    }

    ustat_handle_t* get_nkv_ustat_handle(void) {
      return nkv_ustat_handle;
    }
    
    void set_nkv_ustat_handle(ustat_handle_t* handle) {
      nkv_ustat_handle =  handle;
    }

    nkv_result nkv_send_io(uint64_t container_hash, uint64_t container_path_hash, const nkv_key* key, void* opt, 
                           nkv_value* value, int32_t which_op, nkv_postprocess_function* post_fn,
                           uint32_t client_rdma_key, uint16_t client_rdma_qhandle) {
      nkv_result stat = NKV_SUCCESS;

      auto c_iter = cnt_list.find(container_hash);
      if (c_iter == cnt_list.end()) {
        smg_error(logger,"No Container found for hash = %u, number of containers = %u", container_hash, cnt_list.size());
        return NKV_ERR_NO_CNT_FOUND;
      }

      if((which_op == NKV_LOCK_OP || which_op == NKV_UNLOCK_OP) \
				&& (((nkv_lock_option *)opt)->nkv_lock_uuid != this->instance_uuid)) {
			return NKV_ERR_LOCK_UUID_MISMATCH;
      }

      NKVTarget* one_cnt = c_iter->second;
      // A Subsystem UP is indicated by status 0
      if (one_cnt && ! one_cnt->get_ss_status()) {
        stat = one_cnt->send_io_to_path(container_path_hash, key, opt, value, which_op, post_fn, 
                                        client_rdma_key, client_rdma_qhandle);
      } else {
        smg_error(logger, "NULL Container found for hash = %u!!", container_hash);
        return NKV_ERR_NO_CNT_FOUND;
      }

      //Populate cache for meta keys
      /*if (false && (stat == NKV_SUCCESS) && (!cache_hit)) {
        std::string key_str ((char*) key->key, key->length);
        std::size_t found = key_str.find(iter_prefix);
        if (found != std::string::npos && found == 0) {
          //Meta keys, check if it is present in lru cache
          if (shard_id == -1) {
            std::size_t key_prefix = std::hash<std::string>{}(key_str);
            shard_id = key_prefix % nkv_read_cache_shard_size;
          }
          
          if (which_op != NKV_DELETE_OP) {
            if (which_op == NKV_RETRIEVE_OP) {
              void* c_buffer = malloc(value->actual_length);
              memcpy(c_buffer, value->value, value->actual_length);

              nkv_value_wrapper nkvvalue (c_buffer, value->actual_length, value->actual_length);
              cnt_cache[shard_id].put(key_str, std::move(nkvvalue));
             
            } else {
              void* c_buffer = malloc(value->length);
              memcpy(c_buffer, value->value, value->length);
              nkv_value_wrapper nkvvalue (c_buffer, value->length, value->length);
              cnt_cache[shard_id].put(key_str, std::move(nkvvalue));
            }
          } else {
            cnt_cache[shard_id].del(key_str);
          }
          
        }
          
      }*/

      return stat;
    }

    nkv_result nkv_list_keys (uint64_t container_hash, uint64_t container_path_hash, uint32_t* max_keys, 
                              nkv_key* keys, void*& iter_context, const char* prefix, const char* delimiter, const char* start_after) {

      nkv_result stat = NKV_SUCCESS;
      auto c_iter = cnt_list.find(container_hash);
      if (c_iter == cnt_list.end()) {
        smg_error(logger,"No Container found for hash = %u, number of containers = %u, op = list_keys", container_hash, cnt_list.size());
        return NKV_ERR_NO_CNT_FOUND;
      }
      NKVTarget* one_cnt = c_iter->second;
      if (one_cnt) {
        stat = one_cnt->list_keys_from_path(container_path_hash, max_keys, keys, iter_context, prefix, delimiter, start_after);
      } else {
        smg_error(logger, "NULL Container found for hash = %u, op = list_keys!!", container_hash);
        return NKV_ERR_NO_CNT_FOUND;
      }
      return stat;

    }

    nkv_result nkv_get_path_mount_point (uint64_t container_hash, uint64_t container_path_hash, std::string& p_mount) {
      nkv_result stat = NKV_SUCCESS;
      auto c_iter = cnt_list.find(container_hash);
      if (c_iter == cnt_list.end()) {
        smg_error(logger,"No Container found for hash = %u, number of containers = %u, op = nkv_get_path_mount_point", container_hash, cnt_list.size());
        return NKV_ERR_NO_CNT_FOUND;
      }
      NKVTarget* one_cnt = c_iter->second;
      if (one_cnt) {
        stat = one_cnt->get_path_mount_point(container_path_hash, p_mount);
      } else {
        smg_error(logger, "NULL Container found for hash = %u, op = nkv_get_path_mount_point!!", container_hash);
        return NKV_ERR_NO_CNT_FOUND;
      }
      return stat;

    }

    // Get nqn name associated to a subsystem or remote paths.
    nkv_result nkv_get_target_container_name(uint64_t container_hash,
                                             uint64_t container_path_hash,
                                             std::string& subsystem_nqn,
                                             std::string& p_mount) {
      nkv_result stat = NKV_SUCCESS;
      auto c_iter = cnt_list.find(container_hash);
      if (c_iter == cnt_list.end()) {
        smg_error(logger,"No Container found for hash = %u, number of containers = %u, op = nkv_get_path_mount_point", container_hash, cnt_list.size());
        return NKV_ERR_NO_CNT_FOUND;
      }
      NKVTarget* one_cnt = c_iter->second;
      if (one_cnt) {
        subsystem_nqn = one_cnt->target_container_name;
        stat = one_cnt->get_path_mount_point(container_path_hash, p_mount);
      } else {
        smg_error(logger, "NULL Container found for hash = %u, op = nkv_get_target_container_name!!", container_hash);
        return NKV_ERR_NO_CNT_FOUND;
      }
      return stat;
    }
      
    bool populate_container_info(nkv_container_info *cntlist, uint32_t *cnt_count, uint32_t index);

    bool open_container_paths() {
      bool is_container_list_valid = false;
      for (auto m_iter = cnt_list.begin(); m_iter != cnt_list.end(); m_iter++) {
        NKVTarget* one_cnt = m_iter->second;
        if (one_cnt) {
          smg_alert(logger, "Opening path for target node = %s, container name = %s status=%d",
                   one_cnt->target_node_name.c_str(), one_cnt->target_container_name.c_str(),one_cnt->get_ss_status());
          if ( !one_cnt->get_ss_status() ) {
            if( one_cnt->open_paths(app_name) ) {
              is_container_list_valid = true;
            } else {
              smg_error(logger, "Opening path failed, making SS down, target node = %s, container name = %s status=%d",
                   one_cnt->target_node_name.c_str(), one_cnt->target_container_name.c_str(),one_cnt->get_ss_status());              
              one_cnt->set_ss_status(1);
            }
          } else {
            smg_error(logger, "SS is already down, skipping, target node = %s, container name = %s status=%d",
                      one_cnt->target_node_name.c_str(), one_cnt->target_container_name.c_str(),one_cnt->get_ss_status());
          }
        } else {
          smg_error(logger, "Got NULL container while opening paths !!");
        }
      }
      return is_container_list_valid;
    }

    bool verify_min_topology_exists (int32_t num_required_container, int32_t num_required_container_path) {
      bool min_topology_exist = true;
      if (cnt_list.size() < (uint32_t) num_required_container) {
        smg_error(logger, "Not enough containers, minimum required = %d, available = %d",
                 num_required_container, cnt_list.size());
        min_topology_exist = false;
      }
      else {
        uint32_t valid_container_count = cnt_list.size(); // subsystem count
        for (auto m_iter = cnt_list.begin(); m_iter != cnt_list.end(); m_iter++) {
          NKVTarget* one_cnt = m_iter->second;
          if (one_cnt) {
            smg_info(logger, "Inspecting target id = %d, target node = %s, container name = %s", 
                     one_cnt->t_id, one_cnt->target_node_name.c_str(), one_cnt->target_container_name.c_str()); 
            if (! one_cnt->get_ss_status() && !one_cnt->verify_min_path_exists(num_required_container_path)) {
              valid_container_count--;
              one_cnt->set_ss_status(1);
              smg_error(logger, "Minimum path doesn't exist for the container %s", one_cnt->target_container_name.c_str());
            }
          } else {
            smg_error(logger, "Got NULL container while adding path mount point !!");
            valid_container_count--;
          }
        }
        if ( valid_container_count < (uint32_t) num_required_container ) {
          min_topology_exist = false;
        }
      }
      return min_topology_exist;
    }


    // Code for ustat
    void initiate_nkv_ustat(bool device, bool cpu=false);
    void reset_nkv_ustat();
    void remove_nkv_ustat(bool device, bool cpu=false);
    

    void wait_or_detach_thread (bool will_wait = true) {
      for (auto m_iter = cnt_list.begin(); m_iter != cnt_list.end(); m_iter++) {
        NKVTarget* one_cnt = m_iter->second;
        if (one_cnt) {
          smg_debug(logger, "wait_or_detach_thread for target id = %d, target node = %s, container name = %s",
                   one_cnt->t_id, one_cnt->target_node_name.c_str(), one_cnt->target_container_name.c_str());
          one_cnt->wait_or_detach_path_thread(will_wait);
        } else {
          smg_error(logger, "Got NULL container while inspecting thread !!");
          assert(0);
        }
      }
    }

    /* Function Name: parse_add_path_mount_point
     * Input Args   : <boost::property_tree::ptree> - ClusterMap with remote mounted paths
     * Return       : <int32_t> - 0/1, Success/failure
     * Description  : Populate container and NKVTragetPath with in container with remote
     *                mount path information. and update path/ container status accordingly.
     */
    int32_t parse_add_path_mount_point(boost::property_tree::ptree & pt) {
      try {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("nkv_remote_mounts")) {
          assert(v.first.empty());
          boost::property_tree::ptree pc = v.second;
          std::string subsystem_mount = pc.get<std::string>("mount_point");
          std::string subsystem_address = pc.get<std::string>("nqn_transport_address");
          std::string subsystem_name = pc.get<std::string>("remote_nqn_name");
          std::string subsystem_node = pc.get<std::string>("remote_target_node_name");
          int32_t subsystem_port = pc.get<int>("nqn_transport_port");
          int32_t numa_node_attached = pc.get<int>("numa_node_attached");
          int32_t driver_thread_core = -1;
          if (core_pinning_required)
            driver_thread_core = pc.get<int>("driver_thread_core");

          smg_alert(logger, "Adding device path, mount point = %s, address = %s, port = %d, nqn name = %s, target node = %s, numa = %d, core = %d",
                    subsystem_mount.c_str(), subsystem_address.c_str(), subsystem_port, subsystem_name.c_str(), subsystem_node.c_str(), 
                    numa_node_attached, driver_thread_core);
          
          bool dev_path_added = false;
 
          for (auto m_iter = cnt_list.begin(); m_iter != cnt_list.end(); m_iter++) {
            NKVTarget* one_cnt = m_iter->second;
            if (one_cnt) {
              if ((one_cnt->target_node_name == subsystem_node) && (one_cnt->target_container_name == subsystem_name)){
                int32_t stat = one_cnt->add_mount_point_to_path(subsystem_address, subsystem_mount, numa_node_attached, driver_thread_core);
                if (stat == 0) {
                  dev_path_added = true;
                  break;
                }
              }  

            } else {
              smg_error(logger, "Got NULL container while adding path mount point !!");
            }
          }
          if (!dev_path_added) {
            smg_error(logger, "Could not add Device path, mount point = %s, address = %s, port = %d, nqn name = %s, target node = %s",
                    subsystem_mount.c_str(), subsystem_address.c_str(), subsystem_port, subsystem_name.c_str(), subsystem_node.c_str());
             
          }

        }

      }
      catch (std::exception& e) {
        smg_error(logger, "%s%s", "Error reading config file while adding path mount point, Error = ", e.what());
        return 1;
      } 
      return 0; 
    }
 
    // This is for LKV based deployment
    int32_t add_local_container_and_path (const char* host_name_ip, uint32_t host_port, boost::property_tree::ptree & pt);

    // This is for Network KV based deployment
    /* Function Name: parse_add_container
     * Input Args   : <boost::property_tree::ptree> = ClusterMap information
     * Return       : <int32_t> 0/1 = Success/failure
     * Description  : Create a NKV-Host container list based on "subsystem_map" informaion
     *                of ClusterMap. A container contains subsystem information.
     */
    int32_t parse_add_container(boost::property_tree::ptree & pr) {

      int32_t cnt_id = 0;  
      try {
        BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pr.get_child("subsystem_maps")) {
          assert(v.first.empty());
          
          boost::property_tree::ptree pt = v.second;
          std::string target_server_name = pt.get<std::string>("target_server_name");
          std::string subsystem_nqn_id = pt.get<std::string>("subsystem_nqn_id");
          std::string subsystem_nqn = pt.get<std::string>("subsystem_nqn");
          int32_t subsystem_status = pt.get<int>("subsystem_status");
          float subsystem_space_available_percent = pt.get<float>("subsystem_avail_percent");
          uint64_t ss_hash = std::hash<std::string>{}(subsystem_nqn_id);

          NKVTarget* one_cnt = new NKVTarget(cnt_id, subsystem_nqn_id, target_server_name, subsystem_nqn, ss_hash);
          assert(one_cnt != NULL);
          one_cnt->set_ss_status(subsystem_status);
          one_cnt->set_space_avail_percent(subsystem_space_available_percent);

          int32_t path_id = 0;
          BOOST_FOREACH(boost::property_tree::ptree::value_type &v, pt.get_child("subsystem_transport")) {
            assert(v.first.empty());
            boost::property_tree::ptree pc = v.second;
            int32_t subsystem_np_type = pc.get<int>("subsystem_type");
            std::string subsystem_address = pc.get<std::string>("subsystem_address");
            int32_t subsystem_port = pc.get<int>("subsystem_port");
            int32_t subsystem_addr_fam = pc.get<int>("subsystem_addr_fam");
            int32_t subsystem_in_speed = pc.get<int>("subsystem_interface_speed");
            int32_t subsystem_in_status = pc.get<int>("subsystem_interface_status");
            int32_t numa_aligned = pc.get<bool>("subsystem_interface_numa_aligned");

            uint64_t ss_p_hash = std::hash<std::string>{}(subsystem_address);

            NKVTargetPath* one_path = new NKVTargetPath(ss_p_hash, path_id, subsystem_address, subsystem_port, subsystem_addr_fam, 
                                                        subsystem_in_speed, subsystem_in_status, numa_aligned, subsystem_np_type);
            assert(one_path != NULL);
            smg_info(logger, "Adding path, hash = %u, address = %s, port = %d, fam = %d, speed = %d, status = %d, numa_aligned = %d",
                    ss_p_hash, subsystem_address.c_str(), subsystem_port, subsystem_addr_fam, subsystem_in_speed, subsystem_in_status, numa_aligned);

            one_cnt->add_network_path(one_path, ss_p_hash);
            path_id++;
          }

          auto cnt_iter = cnt_list.find(ss_hash);
          assert (cnt_iter == cnt_list.end());
          cnt_list[ss_hash] = one_cnt;
          one_cnt->load_balance_initialize_cnt();
		  
          smg_info (logger, "Container added, hash = %u, id = %u, uuid = %s, Node name = %s , NQN name = %s , container count = %d",
                   ss_hash, cnt_id, subsystem_nqn_id.c_str(), target_server_name.c_str(), subsystem_nqn.c_str(), cnt_list.size());
          cnt_id++;
 
        }
      }
      catch (std::exception& e) {
        smg_error(logger, "%s%s", "Error reading config file while adding container/path, Error = ", e.what());
        return 1;
      }
      return 0;
    }

    int32_t add_container(const std::string& tuuid, NKVTarget* pcnt) {
      if (pcnt) {
        if (!tuuid.empty()) {
          uint64_t ss_hash = std::hash<std::string>{}(tuuid);

          cnt_list[ss_hash] = pcnt;
          smg_info (logger, "Container added, hash = %u, uuid = %s, container count = %d", ss_hash, tuuid.c_str(), cnt_list.size());
          return 0;
        } else {
          smg_warn (logger, "Empty uuid passed !! "); 
        }
      } else {
        smg_warn (logger, "NULL container passed !! ");
      }
      return 1;
    }
 
    
   /* Function Name: update_container
    * Params       : <string> -Address of Remote Mount Path
    *                <int32_t>-Rremote Mount Path status
    * Return       : <bool>  Updated Mount Path or Not
    * Description  : Update remote mount path status based on the event received
    *                from target cluster. This function gets invoked from event
    *                handler.
    */
    bool update_container(std::string category,
                          std::string node_name,
                          boost::property_tree::ptree& args,
                          int32_t remote_path_status);

    void show_listing_stat() {
      for (auto m_iter = cnt_list.begin(); m_iter != cnt_list.end(); m_iter++) {
        NKVTarget* one_cnt = m_iter->second;
        if (one_cnt) {
          one_cnt->show_listing_stat();
        } else {
          smg_error(logger, "Got NULL container while inspecting stat !!");
          
        }
      }

    }  
  };

#endif

