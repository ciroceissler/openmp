//===---- omptarget-nvptxi.h - NVPTX OpenMP GPU initialization --- CUDA -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.txt for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the declarations of all library macros, types,
// and functions.
//
//===----------------------------------------------------------------------===//

////////////////////////////////////////////////////////////////////////////////
// Task Descriptor
////////////////////////////////////////////////////////////////////////////////

INLINE omp_sched_t omptarget_nvptx_TaskDescr::GetRuntimeSched() {
  // sched starts from 1..4; encode it as 0..3; so add 1 here
  uint8_t rc = (data.items.flags & TaskDescr_SchedMask) + 1;
  return (omp_sched_t)rc;
}

INLINE void omptarget_nvptx_TaskDescr::SetRuntimeSched(omp_sched_t sched) {
  // sched starts from 1..4; encode it as 0..3; so add 1 here
  uint8_t val = ((uint8_t)sched) - 1;
  // clear current sched
  data.items.flags &= ~TaskDescr_SchedMask;
  // set new sched
  data.items.flags |= val;
}

INLINE void omptarget_nvptx_TaskDescr::InitLevelZeroTaskDescr() {
  // slow method
  // flag:
  //   default sched is static,
  //   dyn is off (unused now anyway, but may need to sample from host ?)
  //   not in parallel

  data.items.flags = 0;
  data.items.nthreads = GetNumberOfProcsInTeam();
  ;                                // threads: whatever was alloc by kernel
  data.items.threadId = 0;         // is master
  data.items.threadsInTeam = 1;    // sequential
  data.items.runtimeChunkSize = 1; // prefered chunking statik with chunk 1
}

INLINE void omptarget_nvptx_TaskDescr::CopyData(
    omptarget_nvptx_TaskDescr *sourceTaskDescr) {
  data.vect[0] = sourceTaskDescr->data.vect[0];
  data.vect[1] = sourceTaskDescr->data.vect[1];
}

INLINE void
omptarget_nvptx_TaskDescr::Copy(omptarget_nvptx_TaskDescr *sourceTaskDescr) {
  CopyData(sourceTaskDescr);
  prev = sourceTaskDescr->prev;
}

INLINE void omptarget_nvptx_TaskDescr::CopyParent(
    omptarget_nvptx_TaskDescr *parentTaskDescr) {
  CopyData(parentTaskDescr);
  prev = parentTaskDescr;
}

INLINE void omptarget_nvptx_TaskDescr::CopyForExplicitTask(
    omptarget_nvptx_TaskDescr *parentTaskDescr) {
  CopyParent(parentTaskDescr);
  data.items.flags = data.items.flags & ~TaskDescr_IsParConstr;
  ASSERT0(LT_FUSSY, IsTaskConstruct(), "expected task");
}

INLINE void omptarget_nvptx_TaskDescr::CopyToWorkDescr(
    omptarget_nvptx_TaskDescr *masterTaskDescr, uint16_t tnum) {
  CopyParent(masterTaskDescr);
  // overrwrite specific items;
  data.items.flags |=
      TaskDescr_InPar | TaskDescr_IsParConstr; // set flag to parallel
  data.items.threadsInTeam = tnum;             // set number of threads
}

INLINE void omptarget_nvptx_TaskDescr::CopyFromWorkDescr(
    omptarget_nvptx_TaskDescr *workTaskDescr) {
  Copy(workTaskDescr);
  // overrwrite specific items;
  data.items.threadId =
      GetThreadIdInBlock(); // get ids from cuda (only called for 1st level)
}

////////////////////////////////////////////////////////////////////////////////
// Thread Private Context
////////////////////////////////////////////////////////////////////////////////

INLINE omptarget_nvptx_TaskDescr *
omptarget_nvptx_ThreadPrivateContext::GetTopLevelTaskDescr(int gtid) {
  ASSERT0(
      LT_FUSSY, gtid < MAX_NUM_OMP_THREADS,
      "Getting top level, gtid is larger than allocated data structure size");
  return topTaskDescr[gtid];
}

INLINE void
omptarget_nvptx_ThreadPrivateContext::InitThreadPrivateContext(int gtid) {
  // levelOneTaskDescr is init when starting the parallel region
  // top task descr is NULL (team master version will be fixed separately)
  topTaskDescr[gtid] = NULL;
  // no num threads value has been pushed
  tnumForNextPar[gtid] = 0;
  // priv counter init to zero
  priv[gtid] = 0;
  // the following don't need to be init here; they are init when using dyn
  // sched
  // current_Event, events_Number, chunk, num_Iterations, schedule
}

////////////////////////////////////////////////////////////////////////////////
// Work Descriptor
////////////////////////////////////////////////////////////////////////////////

INLINE void omptarget_nvptx_WorkDescr::InitWorkDescr() {
  cg.Clear(); // start and stop to zero too
  // threadsInParallelTeam does not need to be init (done in start parallel)
  hasCancel = FALSE;
}

////////////////////////////////////////////////////////////////////////////////
// Team Descriptor
////////////////////////////////////////////////////////////////////////////////

INLINE void omptarget_nvptx_TeamDescr::InitTeamDescr() {
  levelZeroTaskDescr.InitLevelZeroTaskDescr();
  workDescrForActiveParallel.InitWorkDescr();
  // omp_init_lock(criticalLock);
}

////////////////////////////////////////////////////////////////////////////////
// Get private data structure for thread
////////////////////////////////////////////////////////////////////////////////

// Utility routines for CUDA threads
INLINE omptarget_nvptx_TeamDescr &getMyTeamDescriptor() {
  return omptarget_nvptx_threadPrivateContext->TeamContext()[GetOmpTeamId()];
}

INLINE omptarget_nvptx_WorkDescr &getMyWorkDescriptor() {
  omptarget_nvptx_TeamDescr &currTeamDescr = getMyTeamDescriptor();
  return currTeamDescr.WorkDescr();
}

INLINE omptarget_nvptx_TaskDescr *getMyTopTaskDescriptor(int globalThreadId) {
  return omptarget_nvptx_threadPrivateContext->GetTopLevelTaskDescr(
      globalThreadId);
}

INLINE omptarget_nvptx_TaskDescr *getMyTopTaskDescriptor() {
  return getMyTopTaskDescriptor(GetGlobalThreadId());
}
