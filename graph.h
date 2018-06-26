/* Copyright 2018 Stanford, UT Austin, LANL
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _LUX_GRAPH_H_
#define _LUX_GRAPH_H_

#include <cstdio>
#include "app.h"
#include "legion.h"
#include <unistd.h>
#include <cuda_runtime.h>

using namespace Legion;
template<typename FT, int N, typename T = coord_t> using AccessorRO = FieldAccessor<READ_ONLY,FT,N,T,Realm::AffineAccessor<FT,N,T> >;
template<typename FT, int N, typename T = coord_t> using AccessorRW = FieldAccessor<READ_WRITE,FT,N,T,Realm::AffineAccessor<FT,N,T> >;
template<typename FT, int N, typename T = coord_t> using AccessorWO = FieldAccessor<WRITE_ONLY,FT,N,T,Realm::AffineAccessor<FT,N,T> >;

#define MAX_FILE_LEN 64
#define MAX_NUM_PARTS 64
#define FILE_HEADER_SIZE (sizeof(E_ID) + sizeof(V_ID))
#define MAP_TO_FB_MEMORY 0xABCD0000
#define MAP_TO_ZC_MEMORY 0xABCE0000

enum {
  TOP_LEVEL_TASK_ID,
  LOAD_TASK_ID,
  SCAN_TASK_ID,
  INIT_TASK_ID,
  APP_TASK_ID,
  PUSH_LOAD_TASK_ID,
  PUSH_INIT_TASK_ID,
  PUSH_APP_TASK_ID,
};

enum FieldIDs {
  FID_DATA,
};

class Graph
{
public:
  Graph(Context ctx, Runtime* rt, int _numParts, std::string& file_name);
  int numParts;
  V_ID nv;
  E_ID ne;
  V_ID frontierSize; // maximum allowed size for frontier queues
  V_ID rowLeft[MAX_NUM_PARTS], rowRight[MAX_NUM_PARTS];
  V_ID fqLeft[MAX_NUM_PARTS], fqRight[MAX_NUM_PARTS];
  LogicalRegion row_ptr_lr;
  LogicalPartition row_ptr_lp;
  LogicalRegion raw_row_lr;
  LogicalPartition raw_row_lp;
  LogicalRegion in_vtx_lr;
  LogicalPartition in_vtx_lp;
  LogicalRegion col_idx_lr;
  LogicalPartition col_idx_lp;
  LogicalRegion raw_col_lr;
  LogicalPartition raw_col_lp;
  LogicalRegion degree_lr;
  LogicalPartition degree_lp;
  LogicalRegion raw_weight_lr;
  LogicalPartition raw_weight_lp;
  LogicalRegion frontier_lr[2];
  LogicalPartition frontier_lp[2];
  LogicalRegion dist_lr[2];
  LogicalPartition dist_lp[2];
};

class GraphPiece
{
public:
  V_ID myInVtxs;
  V_ID nv;
  E_ID ne;
  char *oldFqFb, *newFqFb;
  Vertex *oldPrFb, *newPrFb;
  cudaStream_t streams[MAX_NUM_PARTS];
};

struct FrontierHeader
{
  static const V_ID DENSE_BITMAP = 0x12345678;
  static const V_ID SPARSE_QUEUE = 0x87654321;
  V_ID numNodes;
  V_ID type;
};

// ----------------------------------------------------------------------------
// Tasks for Pull-based Execution
// ----------------------------------------------------------------------------

class LoadTask : public IndexLauncher
{
public:
  LoadTask(const Graph &graph,
           const IndexSpaceT<1> &domain,
           const ArgumentMap &arg_map,
           std::string &fn);
};

class ScanTask : public TaskLauncher
{
public:
  ScanTask(const Graph &graph);
};

class InitTask : public IndexLauncher
{
public:
  InitTask(const Graph &graph,
           const IndexSpaceT<1> &domain,
           const ArgumentMap &arg_map);
};

class AppTask : public IndexLauncher
{
public:
  AppTask(const Graph &graph,
          const IndexSpaceT<1> &domain,
          const ArgumentMap &arg_map,
          int iteration);
};

void load_task_impl(const Task *task,
                    const std::vector<PhysicalRegion> &regions,
                    Context ctx, Runtime *runtime);

void scan_task_impl(const Task *task,
                    const std::vector<PhysicalRegion> &regions,
                    Context ctx, Runtime *runtime);

void app_task_impl(const Task *task,
                   const std::vector<PhysicalRegion> &regions,
                   Context ctx, Runtime *runtime);

GraphPiece init_task_impl(const Task *task,
                          const std::vector<PhysicalRegion> &regions,
                          Context ctx, Runtime *runtime);

// ----------------------------------------------------------------------------
// Tasks for Pull-based Execution
// ----------------------------------------------------------------------------
class PushLoadTask : public IndexLauncher
{
public:
  PushLoadTask(const Graph &graph,
               const IndexSpaceT<1> &domain,
               const ArgumentMap &arg_map,
               std::string &fn);
};

class PushInitTask : public IndexLauncher
{
public:
  PushInitTask(const Graph &graph,
               const IndexSpaceT<1> &domain,
               const ArgumentMap &arg_map);
};

class PushAppTask : public IndexLauncher
{
public:
  PushAppTask(const Graph &graph,
              const IndexSpaceT<1> &domain,
              const ArgumentMap &arg_map,
              int iteration);
};

void push_load_task_impl(const Task *task,
                         const std::vector<PhysicalRegion> &regions,
                         Context ctx, Runtime *runtime);

V_ID push_app_task_impl(const Task *task,
                        const std::vector<PhysicalRegion> &regions,
                        Context ctx, Runtime *runtime);

GraphPiece push_init_task_impl(const Task *task,
                               const std::vector<PhysicalRegion> &regions,
                               Context ctx, Runtime *runtime);




#endif
