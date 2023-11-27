/*
  Copyright 2015 Google LLC All rights reserved.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at:

    http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

/*
   american fuzzy lop - LLVM instrumentation bootstrap
   ---------------------------------------------------

   Written by Laszlo Szekeres <lszekeres@google.com> and
              Michal Zalewski <lcamtuf@google.com>

   LLVM integration design comes from Laszlo Szekeres.

   This code is the rewrite of afl-as.h's main_payload.
*/

#include "../android-ashmem.h"
#include "../config.h"
#include "../types.h"

#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <stdbool.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>

/* This is a somewhat ugly hack for the experimental 'trace-pc-guard' mode.
   Basically, we need to make sure that the forkserver is initialized after
   the LLVM-generated runtime initialization pass, not before. */

#ifdef USE_TRACE_PC
#define CONST_PRIO 5
#else
#define CONST_PRIO 0
#endif /* ^USE_TRACE_PC */

/* Globals needed by the injected instrumentation. The __afl_area_initial region
   is used for instrumentation output before __afl_map_shm() has a chance to
   run. It will end up as .comm, so it shouldn't be too wasteful. */

struct exec_info
{
  u8 coverage[MAP_SIZE];
  u64 min_value[INST_SIZE];
  u64 max_value[INST_SIZE];
  u32 exec_inst_cnt;
  u64 eflag[INST_SIZE];
  u64 last_inst_id;
  //cfg
  u32 bb_start_index[MAP_SIZE];
  u16 bb_succ_cnt[MAP_SIZE];
  u32 exec_edge_info[MAP_SIZE*8];
  u32 last_bb_id;

  u32 exec_order[INST_SIZE+MAP_SIZE];
  u8 is_visited[INST_SIZE+MAP_SIZE];

  u64 stack_start;
  u64 stack_end;
  u64 heap_start;
  u64 heap_end;
};

struct exec_info __afl_area_initial;
struct exec_info * __afl_area_ptr = &__afl_area_initial;

__thread u32 __afl_prev_loc;

/* Running in persistent mode? */

static u8 is_persistent;

/* SHM setup. */

static void __afl_map_shm(void) {

  u8 *id_str = getenv(SHM_ENV_VAR);

  /* If we're running under AFL, attach to the appropriate region, replacing the
     early-stage __afl_area_initial region that is needed to allow some really
     hacky .init code to work correctly in projects such as OpenSSL. */

  if (id_str) {

    u32 shm_id = atoi(id_str);

    __afl_area_ptr = (struct exec_info *)shmat(shm_id, NULL, 0);

    /* Whooooops. */

    if (__afl_area_ptr == (void *)-1)
      _exit(1);

    /* Write something into the bitmap so that even with low AFL_INST_RATIO,
       our parent doesn't give up on us. */

    __afl_area_ptr->coverage[0] = 1;
  }
}

uint64_t signals = 0;
s32 PARENT_PID;
pthread_mutex_t mutex;

void read_procmap(s32 pid) {
  char line[2048];
  sprintf(line, "/proc/%d/maps", pid);
  FILE *fp = fopen(line, "r");
  if (!fp) {
    perror("fopen");
    abort();
  }
  u64 start, end;

  while (fgets(line, 2048, fp) != NULL) {
    if (strstr(line, "stack") != NULL) {
      end = strtol(line + 13, line + 25, 16);
      start = strtol(line, line + 12, 16);

      __afl_area_ptr->stack_start = start;
      __afl_area_ptr->stack_end = end;
    }
    if (strstr(line, "heap") != NULL) {
      end = strtol(line + 9, line + 17, 16);
      start = strtol(line, line + 8, 16);

      __afl_area_ptr->heap_start = start;
      __afl_area_ptr->heap_end = end;
    }
  }
  fclose(fp);
}

static void handle_child(int sig) {
  signals += 1;
  // signals to the main thread that child has exited
  pthread_mutex_unlock(&mutex);
}

__attribute__((destructor)) void __afl_proc_exit(void) {
  if (getpid() == PARENT_PID) return;
  read_procmap(getpid());
}

/* Fork server logic. */

static void __afl_start_forkserver(void) {
  PARENT_PID = getpid();

  static u8 tmp[4];
  s32 child_pid;

  u8 child_stopped = 0;

  /* Phone home and tell the parent that we're OK. If parent isn't there,
     assume we're not running in forkserver mode and just execute program. */

  if (write(FORKSRV_FD + 1, tmp, 4) != 4)
    return;

  while (1) {

    u32 was_killed;
    int status;

    /* Wait for parent by reading from the pipe. Abort if read fails. */

    if (read(FORKSRV_FD, &was_killed, 4) != 4)
      _exit(1);

    /* If we stopped the child in persistent mode, but there was a race
       condition and afl-fuzz already issued SIGKILL, write off the old
       process. */

    if (child_stopped && was_killed) {
      child_stopped = 0;
      if (waitpid(child_pid, &status, 0) < 0)
        _exit(1);
    }

    if (!child_stopped) {

      /* Once woken up, create a clone of our process. */

      child_pid = fork();
      if (child_pid < 0)
        _exit(1);

      /* In child process: close fds, resume execution. */

      if (!child_pid) {

        close(FORKSRV_FD);
        close(FORKSRV_FD + 1);
        return;
      }
    } else {

      /* Special handling for persistent mode: if the child is alive but
         currently stopped, simply restart it with SIGCONT. */

      kill(child_pid, SIGCONT);
      child_stopped = 0;
    }

    /* In parent process: write PID to pipe, then wait for child. */

    if (write(FORKSRV_FD + 1, &child_pid, 4) != 4)
      _exit(1);

    if (waitpid(child_pid, &status, is_persistent ? WUNTRACED : 0) < 0)
      _exit(1);

    /* In persistent mode, the child stops itself with SIGSTOP to indicate
       a successful run. In this case, we want to wake it up without forking
       again. */

    if (WIFSTOPPED(status))
      child_stopped = 1;

    /* Relay wait status to pipe, then loop back. */

    if (write(FORKSRV_FD + 1, &status, 4) != 4)
      _exit(1);
  }
}

/* A simplified persistent mode handler, used as explained in README.llvm. */

int __afl_persistent_loop(unsigned int max_cnt) {

  static u8 first_pass = 1;
  static u32 cycle_cnt;

  if (first_pass) {

    /* Make sure that every iteration of __AFL_LOOP() starts with a clean slate.
       On subsequent calls, the parent will take care of that, but on the first
       iteration, it's our job to erase any trace of whatever happened
       before the loop. */

    if (is_persistent) {

      memset(__afl_area_ptr, 0, sizeof(struct exec_info));
      __afl_area_ptr->coverage[0] = 1;
      __afl_prev_loc = 0;
    }

    cycle_cnt = max_cnt;
    first_pass = 0;
    return 1;
  }

  if (is_persistent) {

    if (--cycle_cnt) {

      raise(SIGSTOP);

      __afl_area_ptr->coverage[0] = 1;
      __afl_prev_loc = 0;

      return 1;
    } else {

      /* When exiting __AFL_LOOP(), make sure that the subsequent code that
         follows the loop is not traced. We do that by pivoting back to the
         dummy output region. */

      __afl_area_ptr = &__afl_area_initial;
    }
  }

  return 0;
}

/* This one can be called from user code when deferred forkserver mode
    is enabled. */

void __afl_manual_init(void) {

  static u8 init_done;

  if (!init_done) {

    __afl_map_shm();
    __afl_start_forkserver();
    init_done = 1;
  }
}

/* Proper initialization routine. */

__attribute__((constructor(CONST_PRIO))) void __afl_auto_init(void) {

  is_persistent = !!getenv(PERSIST_ENV_VAR);

  if (getenv(DEFER_ENV_VAR))
    return;

  __afl_manual_init();
}

/* The following stuff deals with supporting -fsanitize-coverage=trace-pc-guard.
   It remains non-operational in the traditional, plugin-backed LLVM mode.
   For more info about 'trace-pc-guard', see README.llvm.

   The first function (__sanitizer_cov_trace_pc_guard) is callimage.pnged back
   on every edge (as opposed to every basic block). */

void __sanitizer_cov_trace_pc_guard(uint32_t *guard) {
  __afl_area_ptr->coverage[*guard]++;
}

/* Init callback. Populates instrumentation IDs. Note that we're using
   ID of 0 as a special value to indicate non-instrumented bits. That may
   still touch the bitmap, but in a fairly harmless way. */

void __sanitizer_cov_trace_pc_guard_init(uint32_t *start, uint32_t *stop) {

  u32 inst_ratio = 100;
  u8 *x;

  if (start == stop || *start)
    return;

  x = getenv("AFL_INST_RATIO");
  if (x)
    inst_ratio = atoi(x);

  if (!inst_ratio || inst_ratio > 100) {
    fprintf(stderr, "[-] ERROR: Invalid AFL_INST_RATIO (must be 1-100).\n");
    abort();
  }

  /* Make sure that the first element in the range is always set - we use that
     to avoid duplicate calls (which can happen as an artifact of the underlying
     implementation in LLVM). */

  *(start++) = R(MAP_SIZE - 1) + 1;

  while (start < stop) {

    if (R(100) < inst_ratio)
      *start = R(MAP_SIZE - 1) + 1;
    else
      *start = 0;

    start++;
  }
}

void trace_value(uint64_t value, uint64_t id) {
  if (__afl_area_ptr->is_visited[id]) {
    if (value < __afl_area_ptr->min_value[id])
      __afl_area_ptr->min_value[id] = value;

    if (value > __afl_area_ptr->max_value[id])
      __afl_area_ptr->max_value[id] = value;
  } else {
    __afl_area_ptr->min_value[id] = value;
    __afl_area_ptr->max_value[id] = value;
    __afl_area_ptr->is_visited[id] = 1;
  }

  __afl_area_ptr->exec_inst_cnt++;
  if (__afl_area_ptr->exec_order[id] == 0)
    __afl_area_ptr->exec_order[id] = __afl_area_ptr->exec_inst_cnt;
  
  __afl_area_ptr->last_inst_id = id;

  return;
}


void trace_register(uint64_t value, uint64_t id) {
  __afl_area_ptr->eflag[id] = __afl_area_ptr->eflag[id] | value;
  return;
}


void trace_edge(uint32_t id, uint32_t index, u16 cnt_succ) {
  if(__afl_area_ptr->last_bb_id!=0){
    u32 last_start_index = __afl_area_ptr->bb_start_index[__afl_area_ptr->last_bb_id];
    u16 last_cnt_succ = __afl_area_ptr->bb_succ_cnt[__afl_area_ptr->last_bb_id];

    for(u32 i=last_start_index; i<last_start_index+last_cnt_succ; i++){
      if(__afl_area_ptr->exec_edge_info[i]==0){
        __afl_area_ptr->exec_edge_info[i] = id;
        break;
      }
        
      if(__afl_area_ptr->exec_edge_info[i]==id)
        break;
    }
  }

  __afl_area_ptr->is_visited[INST_SIZE+id] = 1;
  if (__afl_area_ptr->exec_order[INST_SIZE+id] == 0)
    __afl_area_ptr->exec_order[INST_SIZE+id] = __afl_area_ptr->exec_inst_cnt;

  __afl_area_ptr->bb_start_index[id] = index;
  __afl_area_ptr->bb_succ_cnt[id] = cnt_succ;
  __afl_area_ptr->last_bb_id = id;
  return;
}