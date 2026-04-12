// SPDX-FileCopyrightText: Copyright (c) 2019-2021 Virginia Tech
// SPDX-License-Identifier: Apache-2.0

#define CXL
// #define SL_DRAM
// #define DETECT_MEMORY_USAGE
// #define DRAM_TIGHT

#ifdef CXL
#include <memkind.h>
#include <iostream>
#include <string.h>
#else
#include <libpmemobj.h>
#endif
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <unistd.h>
#include "arch.h"
#include "cxl_allocator.h"

#define MASK 0x8000FFFFFFFFFFFF
#define MASK_DIRTY 0xDFFFFFFFFFFFFFFF //DIRTY_BIT
#define MASK_POOL 0x7FFFFFFFFFFFFFFF

#ifdef SL_DRAM
#ifdef READ_LATEST
#define MISS_RATIO 20
#else
#define MISS_RATIO 45
#endif
#endif

//#define MEM_AMOUNT
#ifdef CXL
typedef struct root_obj {
	void* ptr[2];
} root_obj;
#else
typedef struct root_obj{
	PMEMoid ptr[2];
	//    PMEMoid ptr2;
}root_obj;
#endif
void printMemAmount();
void addMemAmount(unsigned long amt);
void zeroMemAmount();

class PMem {
#ifdef CXL
	public:
		static struct memkind *sl_cxl_kind;
		static struct memkind *dl_cxl_kind;
		static struct memkind *log_cxl_kind;
		static int local_stride;
		static int cxl_stride;
		static thread_local size_t local_ticket;
		static thread_local size_t cxl_ticket;
		static size_t already_allocated_;
		static int cxl_fd;
#ifdef DETECT_MEMORY_USAGE
		static uint64_t sl_memory_usage;
		static uint64_t dl_memory_usage;
#endif
#ifdef DRAM_TIGHT
		static uint64_t max_sl_memory_usage;
#endif
#endif
	private:
		static void *baseAddresses[6]; // dram
		static void* logVaddr[2];
		//	static PMEMoid logSpace[4];

		// copy from cxl_allocator.c
		static int gcd(int a, int b)
		{
				while (b != 0) {
						int temp = b;
						b = a % b;
						a = temp;
				}
				return a;
		}
		static enum DEVICE_TYPE stride_scheduler()
		{   

				enum DEVICE_TYPE result = UNKNOWN_DEV;
				if (cxl_stride == 0) {
						result = CXL_DEV;
				} else if (local_stride == 0) {
						result = LOCAL_DEV;
				} else {
						result = cxl_ticket < local_ticket ? CXL_DEV : LOCAL_DEV;
				}

				// update ticket
				switch (result)
				{
				case CXL_DEV:
						// check in case overlflow
						if (SIZE_MAX - cxl_ticket < (size_t)cxl_stride) {
								local_ticket = 0;
								cxl_ticket = 0;
						}
						cxl_ticket += cxl_stride;
						break;
				case LOCAL_DEV:
						// check in case overlflow
						if (SIZE_MAX - local_ticket < (size_t)local_stride) {
								local_ticket = 0;
								cxl_ticket = 0;
						}
						local_ticket += local_stride;
						break;
				default:
						break;
				}
				return result;
		}

	public:
		static void *getBaseOf(int poolId) {
			return baseAddresses[poolId];
		}
		static bool alloc(int poolId, size_t size, void **p) {
#ifdef CXL
#ifdef SL_DRAM	
#ifdef DETECT_MEMORY_USAGE
			static uint64_t id = 0;
			++id;
#endif
				if (poolId == 0) {
#ifdef DRAM_TIGHT
					if (sl_memory_usage >= max_sl_memory_usage) {
						*p = memkind_malloc(dl_cxl_kind, size);
					} else {
						*p = memkind_malloc(sl_cxl_kind, size);
					}
#else
					*p = memkind_malloc(sl_cxl_kind, size);
#endif
#ifdef DETECT_MEMORY_USAGE
					sl_memory_usage += size;
#endif
				} else if (poolId == 1) {
					*p = memkind_malloc(dl_cxl_kind, size);
#ifdef DETECT_MEMORY_USAGE
					dl_memory_usage += size;
#endif
				} else {
					*p = memkind_malloc(log_cxl_kind, size);
#ifdef DETECT_MEMORY_USAGE
					dl_memory_usage += size;
#endif
				}
#ifdef DETECT_MEMORY_USAGE
				// printf("sl memory usage: %lu, dl memory usage: %lu\n", sl_memory_usage, dl_memory_usage);
#endif
				if (*p == nullptr) {
					perror("alloc with cxl error");
					return false;
				} 
				return true;
#endif
			// printf("line 63, allocating %lu bytes, pool id: %d, sl_cxl_kind: %p\n", size, poolId, sl_cxl_kind);
			enum DEVICE_TYPE dev_type = stride_scheduler();
			if (dev_type == LOCAL_DEV) {
				*p = malloc(size);
				if (*p == nullptr) {
					perror("alloc on dram error");
					return false;
				}
				return true;
			}
			if (poolId == 0) {
				*p = memkind_malloc(sl_cxl_kind, size);
			} else if (poolId == 1) {
				*p = memkind_malloc(dl_cxl_kind, size);
			} else {
				*p = memkind_malloc(log_cxl_kind, size);
			}
			printf("allocated %p\n", *p);
			if (*p == nullptr) {
				perror("alloc with cxl error");
				return false;
			}
			return true;
#else
			// allocate a size memory from pool_id
			// and store/persist address to *p
			// return true on succeed
			PMEMobjpool *pop = (PMEMobjpool *)baseAddresses[poolId];            
			PMEMoid oid;
			int ret = pmemobj_alloc(pop, &oid, size, 0, NULL, NULL);
			if(ret){
				std::cerr<<"alloc error"<<std::endl;	
				return false;
			}
			//DEBUG
			*p= reinterpret_cast<void*> (((unsigned long)poolId) << 48 | oid.off);
			return true;
#endif
		}

#ifdef CXL
		static bool alloc(int poolId, size_t size, void **p, void **oid) {
#else
		static bool alloc(int poolId, size_t size, void **p, PMEMoid *oid) {
#endif
#ifdef CXL
#ifdef SL_DRAM	
#ifdef DETECT_MEMORY_USAGE
			static uint64_t id = 0;
			++id;
#endif
			if (poolId == 0) {
#ifdef DRAM_TIGHT
				if (sl_memory_usage >= max_sl_memory_usage) {
					printf("memory is tight\n");
					*p = memkind_malloc(dl_cxl_kind, size);
				} else {
					*p = memkind_malloc(sl_cxl_kind, size);
				}
#else
				*p = memkind_malloc(sl_cxl_kind, size);
#endif
#ifdef DETECT_MEMORY_USAGE
				sl_memory_usage += size;
#endif
			} else if (poolId == 1) {
				*p = memkind_malloc(dl_cxl_kind, size);
#ifdef DETECT_MEMORY_USAGE
				dl_memory_usage += size;
#endif
			} else {
				*p = memkind_malloc(log_cxl_kind, size);
#ifdef DETECT_MEMORY_USAGE
				dl_memory_usage += size;
#endif
			}
#ifdef DETECT_MEMORY_USAGE	
			// if (id % 10000 == 0) {
			// 	printf("sl memory usage: %lu, dl memory usage: %lu\n", sl_memory_usage, dl_memory_usage);
			// }
#endif
			if (*p == nullptr) {
				perror("alloc with cxl error");
				return false;
			} 
			*oid = *p;
			return true;
#endif
			enum DEVICE_TYPE dev_type = stride_scheduler();
			if (dev_type == LOCAL_DEV) {
				*p = malloc(size);
				if (*p == nullptr) {
					perror("alloc on dram error");
					return false;
				}
				*oid = *p;
				return true;
			}
			// printf("[DEBUG] allocating %lu bytes, pool id: %d, sl_cxl_kind: %p\n", size, poolId, sl_cxl_kind);
			if (poolId == 0) {
				*p = memkind_malloc(sl_cxl_kind, size);
			} else if (poolId == 1) {
				*p = memkind_malloc(dl_cxl_kind, size);
			} else {
				*p = memkind_malloc(log_cxl_kind, size);
			}

			if (*p == nullptr) {
				perror("alloc with cxl error");
				*p = memkind_malloc(dl_cxl_kind, size);
				if (*p == nullptr) {
					perror("realloc with cxl error");
					return false;
				}
			} else {
				// printf("[DEBUG] allocated %p\n", *p);
			}
			*oid = *p;
			return true;
#else
			// allocate a size memory from pool_id
			// and store/persist address to *p
			// return true on succeed
			PMEMobjpool *pop = (PMEMobjpool *)baseAddresses[poolId];            
			int ret = pmemobj_alloc(pop, oid, size, 0, NULL, NULL);
			if(ret){
				std::cerr<<"alloc error"<<std::endl;	
				return false;
			}
			//DEBUG
			*p= reinterpret_cast<void*> (((unsigned long)poolId) << 48 | oid->off);
			return true;
#endif
		}
		static void free(void *pptr) {
#ifdef CXL
			// memkind_free(sl_cxl_kind, pptr);
			if (static_cast<char*>(pptr) >= static_cast<char*>(baseAddresses[1])) {
				memkind_free(dl_cxl_kind, pptr);
			} else {
				memkind_free(sl_cxl_kind, pptr);
			}
#else
			// p -> pool_id and offset
			// then perform free
			int poolId = (((unsigned long)pptr)&MASK_POOL) >> 48;
			void *rawPtr = (void *)(((unsigned long)pptr)& MASK + (unsigned long)baseAddresses[poolId]);
			PMEMoid ptr = pmemobj_oid(rawPtr);
			pmemobj_free(&ptr);
#endif
		}

		static void freeVaddr(void *vaddr) {
#ifdef CXL
			// memkind_free(sl_cxl_kind, vaddr);
			if (static_cast<char*>(vaddr) >= static_cast<char*>(baseAddresses[1])) {
				memkind_free(dl_cxl_kind, vaddr);
			} else {
				memkind_free(sl_cxl_kind, vaddr);
			}
#else
			// p -> pool_id and offset
			// then perform free
			PMEMoid ptr = pmemobj_oid(vaddr);
			pmemobj_free(&ptr);
#endif
		}

		// case 1
		static bool bind(int poolId, const char *nvm_path, size_t size, void **root_p, int *is_created, int percentage_on_cxl = 100) {
#ifdef CXL
			if (cxl_fd == 0) {
				cxl_fd = open(nvm_path, O_RDWR);
			}
			if (cxl_fd < 0) {
        fprintf(stderr, "the path for cxl %s not exist\n", nvm_path);
        exit(EXIT_FAILURE);
    	}
#ifdef SL_DRAM
			void *mapped_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    	// struct bitmask *nodemask = numa_allocate_nodemask();
    	// numa_bitmask_setbit(nodemask, 1);
			// numa_set_membind(nodemask);
			printf("miss ratio: %d\n", MISS_RATIO);
#else
			void *mapped_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, cxl_fd, already_allocated_);
			already_allocated_ += size;
#endif
			printf("mapped_memory: %p, mapped end: %p\n", mapped_memory, (void *)((unsigned long)mapped_memory + size));
			baseAddresses[poolId] = mapped_memory;
			if (mapped_memory == MAP_FAILED) {
        perror("CXL init error");
        exit(EXIT_FAILURE);
    	}
			int err;
			if (poolId == 0) {
				printf("[DEBUG] creating sl_cxl_kind\n");
				err = memkind_create_fixed(mapped_memory, size, &sl_cxl_kind);
			} else {
				printf("[DEBUG] creating dl_cxl_kind\n");
				err = memkind_create_fixed(mapped_memory, size, &dl_cxl_kind);
			}
			if (err) {
				perror("memkind_create_fixed");
				exit(EXIT_FAILURE);
			}
			if (local_ticket == 0) {
				if (percentage_on_cxl == 100) {
					local_stride = 1;
					cxl_stride = 0;
				} else if (percentage_on_cxl == 0) {
					local_stride = 0;
					cxl_stride = 1;
				} else {
					int multiple_result = percentage_on_cxl * (100 - percentage_on_cxl) / gcd(percentage_on_cxl, 100 - percentage_on_cxl);
					cxl_stride = multiple_result /  percentage_on_cxl;
					local_stride = multiple_result / (100 -  percentage_on_cxl);
#ifdef CXL_DEBUG
					printf("cxl stride: %ld, local stride: %ld\n", cxl_stride, local_stride);
#endif
    }
			}
			if (poolId == 0) {
				// create log region
				void *mapped_memory2 = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, cxl_fd, already_allocated_);
				memset(mapped_memory2, 0, size);
				baseAddresses[poolId + 2] = mapped_memory2;
				logVaddr[poolId] = mapped_memory2;
				if (mapped_memory2 == MAP_FAILED) {
					perror("CXL init error");
					exit(EXIT_FAILURE);
				}
				already_allocated_ += size;
				printf("[DEBUG] creating log_cxl_kind\n");
				err = memkind_create_fixed(mapped_memory2, size, &log_cxl_kind);
				if (err) {
					perror("memkind_create_fixed, line 192");
					exit(EXIT_FAILURE);
				}
			}
			alloc(poolId, sizeof(root_obj), root_p);
			printf("root_p: %p\n", *root_p);
			*is_created = 1;
			printf("bind success\n");
			return true;
#else
			// open nvm_path with PMDK api and associate
			PMEMobjpool *pop;
			const char* layout = "phydra";
			if (access(nvm_path, F_OK) != 0) {
				pop = pmemobj_create(nvm_path, layout, size, 0666);
				if(pop == nullptr){
					std::cerr<<"bind create error "<<"path : "<<nvm_path<<std::endl;	
					return false;
				}
				baseAddresses[poolId] = reinterpret_cast<void*>(pop);
				*is_created = 1;
			}
			else{
				pop = pmemobj_open(nvm_path,layout); 
				if(pop == nullptr){
					std::cerr<<"bind open error"<<std::endl;	
					std::cerr<<"pooId: "<<poolId<<" nvm_path : "<<nvm_path<<std::endl;	
					return false;
				}
				baseAddresses[poolId] = reinterpret_cast<void*>(pop);
			}
			PMEMoid g_root = pmemobj_root(pop, sizeof(root_obj));
			*root_p=(root_obj*)pmemobj_direct(g_root);
			zeroMemAmount();
			return true;
#endif
		}
		static bool unbind(int poolId) {
#ifdef CXL
			return true;
#else
			PMEMobjpool *pop = reinterpret_cast<PMEMobjpool *>(baseAddresses[poolId]);
			pmemobj_close(pop);
			return true;
#endif
		}

		static bool bindLog(int poolId, const char *nvm_path, size_t size) {
#ifdef CXL
			if (cxl_fd == 0) {
				cxl_fd = open(nvm_path, O_RDWR);
			}
			// if (cxl_fd < 0) {
			// 	fprintf(stderr, "the path for cxl /dev/dax0.0 not exist\n");
      //   exit(EXIT_FAILURE);
			// }
			// printf("already_allocated_: %lu, fd: %d, size: %lu\n", already_allocated_, cxl_fd, size);
			// void *mmaped_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, cxl_fd, already_allocated_);
			// printf("log mapped_memory: %p, mapped end: %p\n", mmaped_memory, (void *)((unsigned long)mmaped_memory + size));
			// baseAddresses[poolId*3+2] = mmaped_memory;
			// logVaddr[poolId] = mmaped_memory;
			// memset(mmaped_memory, 0, size);
			// if (mmaped_memory == MAP_FAILED) {
			// 	perror("CXL init error");
			// 	exit(EXIT_FAILURE);
			// }
			// already_allocated_ += size;
			// int err = memkind_create_fixed(mmaped_memory, size, &log_cxl_kind);
			// if ( err != 0) {
			// 	printf("memkind_create_fixed, err %d\n", err);
			// 	// exit(EXIT_FAILURE);
			// }
#else
			PMEMobjpool *pop;
			int ret;

			if (access(nvm_path, F_OK) != 0) {
				pop = pmemobj_create(nvm_path, POBJ_LAYOUT_NAME(nv), size, 0666);
				if(pop == nullptr){
					std::cerr<<"bind create error"<<std::endl;	
					return false;
				}
				baseAddresses[poolId*3+2] = reinterpret_cast<void*>(pop);
				PMEMoid g_root = pmemobj_root(pop, sizeof(PMEMoid));
				//       PMEMoid g_root = pmemobj_root(pop, 64UL*1024UL*1024UL*1024UL);
				int ret = pmemobj_alloc(pop, &g_root, 512UL*1024UL*1024UL, 0, NULL, NULL);
				if(ret){
					std::cerr<<"!!! alloc error"<<std::endl;	
					return false;
				}
				logVaddr[poolId] = pmemobj_direct(g_root);
				memset((void*)logVaddr[poolId],0,512UL*1024UL*1024UL);
			}
			else{
				//TODO FIX IT
				pop = pmemobj_open(nvm_path, POBJ_LAYOUT_NAME(nv));
				if(pop == nullptr){
					std::cerr<<"bind log open error"<<std::endl;	
					return false;
				}
				PMEMoid g_root = pmemobj_root(pop, sizeof(PMEMoid));
				baseAddresses[poolId*3+2] = reinterpret_cast<void*>(pop);
				logVaddr[poolId] = pmemobj_direct(g_root);
			}
			return true;
#endif
		}

		static bool unbindLog(int poolId) {
#ifdef CXL
			return true;
#else
			PMEMobjpool *pop = reinterpret_cast<PMEMobjpool *>(baseAddresses[poolId*3+2]);
			pmemobj_close(pop);
			return true;
#endif
		}

		static void* getOpLog(int i){
			unsigned long vaddr = (unsigned long)logVaddr[0];
			//printf("vaddr :%p %p\n",vaddr, (void *)(vaddr+(64*i)));

			return (void*)(vaddr + (64*i));
		}

};

static inline void flushToNVM(char *data, size_t sz){
	// volatile char *ptr = (char *)((unsigned long)data & ~(L1_CACHE_BYTES- 1));
	// for (; ptr < data + sz; ptr += L1_CACHE_BYTES) {
	// 	clwb(ptr);
	// }
}

