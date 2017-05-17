//===----RTLs/fpga/src/rtl.cpp - Target RTLs Implementation ------- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// RTL for FPGA machine
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cstddef>
#include <list>
#include <string>
#include <vector>

#include "omptarget.h"

#ifndef TARGET_NAME
#define TARGET_NAME FPGA
#endif

#define GETNAME2(name) #name
#define GETNAME(name) GETNAME2(name)
#define DP(...) DEBUGP("Target " GETNAME(TARGET_NAME) " RTL", __VA_ARGS__)

#define OFFLOADSECTIONNAME ".omp_offloading.entries"

// Utility for retrieving and printing FPGA error string.
#ifdef FPGA_ERROR_REPORT
#define FPGA_ERR_STRING(err)                                                   \
  do {                                                                         \
    const char *errStr;                                                        \
    cuGetErrorString(err, &errStr);                                            \
    DP("FPGA error is: %s\n", errStr);                                         \
  } while (0)
#else
#define FPGA_ERR_STRING(err)                                                   \
  {}
#endif

/// Keep entries table per device.
struct FuncOrGblEntryTy {
  __tgt_target_table Table;
  std::vector<__tgt_offload_entry> Entries;
};

/// Class containing all the device information.
class RTLDeviceInfoTy {
  std::vector<FuncOrGblEntryTy> FuncGblEntries;

public:
  int NumberOfDevices;

  // Device properties
  std::vector<int> ThreadsPerBlock;
  std::vector<int> BlocksPerGrid;
  std::vector<int> WarpSize;

  // OpenMP properties
  std::vector<int> NumTeams;
  std::vector<int> NumThreads;

  // OpenMP Environment properties
  int EnvNumTeams;
  int EnvTeamLimit;

  //static int EnvNumThreads;
  static const int HardTeamLimit = 1<<16; // 64k
  static const int HardThreadLimit = 1024;
  static const int DefaultNumTeams = 128;
  static const int DefaultNumThreads = 128;

  // Record entry point associated with device
  void addOffloadEntry(int32_t device_id, __tgt_offload_entry entry) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[device_id];

    E.Entries.push_back(entry);
  }

  // Return true if the entry is associated with device
  bool findOffloadEntry(int32_t device_id, void *addr) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[device_id];

    for (auto &it : E.Entries) {
      if (it.addr == addr)
        return true;
    }

    return false;
  }

  // Return the pointer to the target entries table
  __tgt_target_table *getOffloadEntriesTable(int32_t device_id) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[device_id];

    int32_t size = E.Entries.size();

    // Table is empty
    if (!size)
      return 0;

    __tgt_offload_entry *begin = &E.Entries[0];
    __tgt_offload_entry *end = &E.Entries[size - 1];

    // Update table info according to the entries and return the pointer
    E.Table.EntriesBegin = begin;
    E.Table.EntriesEnd = ++end;

    return &E.Table;
  }

  // Clear entries table for a device
  void clearOffloadEntriesTable(int32_t device_id) {
    assert(device_id < (int32_t)FuncGblEntries.size() &&
           "Unexpected device id!");
    FuncOrGblEntryTy &E = FuncGblEntries[device_id];
    E.Entries.clear();
    E.Table.EntriesBegin = E.Table.EntriesEnd = 0;
  }

  RTLDeviceInfoTy() {
    DP("Start FPGA\n");

    NumberOfDevices = 1;

    // Get environment variables regarding teams
    char *envStr = getenv("OMP_TEAM_LIMIT");
    if (envStr) {
      // OMP_TEAM_LIMIT has been set
      EnvTeamLimit = std::stoi(envStr);
      DP("Parsed OMP_TEAM_LIMIT=%d\n", EnvTeamLimit);
    } else {
      EnvTeamLimit = -1;
    }
    envStr = getenv("OMP_NUM_TEAMS");
    if (envStr) {
      // OMP_NUM_TEAMS has been set
      EnvNumTeams = std::stoi(envStr);
      DP("Parsed OMP_NUM_TEAMS=%d\n", EnvNumTeams);
    } else {
      EnvNumTeams = -1;
    }
  }

  ~RTLDeviceInfoTy() {
    DP("Finish FPGA\n");
  }
};

static RTLDeviceInfoTy DeviceInfo;

#ifdef __cplusplus
extern "C" {
#endif

int32_t __tgt_rtl_is_valid_binary(__tgt_device_image *image) {
  return 1;
}

int32_t __tgt_rtl_number_of_devices() { return 1; }

int32_t __tgt_rtl_init_device(int32_t device_id) {

  printf("[fpga] __tgt_rtl_init_device\n");

  return OFFLOAD_SUCCESS;
}

__tgt_target_table *__tgt_rtl_load_binary(int32_t device_id,
    __tgt_device_image *image) {

  printf("[fpga] __tgt_rtl_load_binary\n");

  return NULL;
}

void *__tgt_rtl_data_alloc(int32_t device_id, int64_t size) {
  return NULL;
}

int32_t __tgt_rtl_data_submit(int32_t device_id, void *tgt_ptr, void *hst_ptr,
    int64_t size) {

  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_data_retrieve(int32_t device_id, void *hst_ptr, void *tgt_ptr,
    int64_t size) {

  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_data_delete(int32_t device_id, void *tgt_ptr) {

  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_run_target_team_region(int32_t device_id, void *tgt_entry_ptr,
    void **tgt_args, int32_t arg_num, int32_t team_num, int32_t thread_limit,
    uint64_t loop_tripcount) {

  return OFFLOAD_SUCCESS;
}

int32_t __tgt_rtl_run_target_region(int32_t device_id, void *tgt_entry_ptr,
    void **tgt_args, int32_t arg_num) {

  return OFFLOAD_SUCCESS;

  // return __tgt_rtl_run_target_team_region(device_id, tgt_entry_ptr, tgt_args,
  //     arg_num, team_num, thread_limit, 0);
}

#ifdef __cplusplus
}
#endif
