#include <iostream>

#include <gtest/gtest.h>
#include "MPC_Common/include/mpc_common_topology.h"

#include <stdlib.h>


/*
   THE REFERENCE TOPO:
        Processor    0 real (   0:   0:   0)
        Processor    1 real (   0:   0:   1)
        Processor    2 real (   0:   0:   2)
        Processor    3 real (   0:   0:   3)
        Processor    4 real (   0:   0:   4)
        Processor    5 real (   0:   0:   5)
        Processor    6 real (   0:   0:   6)
        Processor    7 real (   0:   0:   7)
        Processor    8 real (   0:   0:   8)
        Processor    9 real (   0:   0:   9)
        Processor   10 real (   0:   0:  10)
        Processor   11 real (   0:   0:  11)
        Processor   12 real (   0:   0:  12)
        Processor   13 real (   0:   0:  13)
        Processor   14 real (   0:   0:  14)
        Processor   15 real (   0:   0:  15)
        Processor   16 real (   0:   0:  16)
        Processor   17 real (   0:   0:  17)
        Processor   18 real (   0:   0:  18)
        Processor   19 real (   0:   0:  19)
        Processor   20 real (   0:   0:  20)
        Processor   21 real (   0:   0:  21)
        Processor   22 real (   0:   0:  22)
        Processor   23 real (   0:   0:  23)
        Processor   24 real (   1:   1:  24)
        Processor   25 real (   1:   1:  25)
        Processor   26 real (   1:   1:  26)
        Processor   27 real (   1:   1:  27)
        Processor   28 real (   1:   1:  28)
        Processor   29 real (   1:   1:  29)
        Processor   30 real (   1:   1:  30)
        Processor   31 real (   1:   1:  31)
        Processor   32 real (   1:   1:  32)
        Processor   33 real (   1:   1:  33)
        Processor   34 real (   1:   1:  34)
        Processor   35 real (   1:   1:  35)
        Processor   36 real (   1:   1:  36)
        Processor   37 real (   1:   1:  37)
        Processor   38 real (   1:   1:  38)
        Processor   39 real (   1:   1:  39)
        Processor   40 real (   1:   1:  40)
        Processor   41 real (   1:   1:  41)
        Processor   42 real (   1:   1:  42)
        Processor   43 real (   1:   1:  43)
        Processor   44 real (   1:   1:  44)
        Processor   45 real (   1:   1:  45)
        Processor   46 real (   1:   1:  46)
        Processor   47 real (   1:   1:  47)
        NUMA: 1
                * Node nb 2
*/

namespace {

// The fixture for testing class Foo.
class TopologyTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  TopologyTest() {
     setenv("MPC_SET_XML_TOPOLOGY_FILE", "topo.xml", 1);
     mpc_common_topology_init();
     //mpc_common_topo_print(stderr);
  }

  ~TopologyTest() override {
     mpc_common_topology_destroy();
     // You can do clean-up work that doesn't throw exceptions here.
  }

  // If the constructor and destructor are not enough for setting up
  // and cleaning up each test, you can define the following methods:

  void SetUp() override {
     // Code here will be called immediately after the constructor (right
     // before each test).
  }

  void TearDown() override {
     // Code here will be called immediately after each test (right
     // before the destructor).
  }

  // Objects declared here can be used by all tests in the test suite for Foo.
};

TEST_F(TopologyTest, mpc_common_topo_get_pu_neighborhood) {
   int neigh[3];

   mpc_common_topo_get_pu_neighborhood(24, 3, neigh);

   GTEST_ASSERT_EQ(neigh[0], 24);
   GTEST_ASSERT_EQ(neigh[1], 25);
   GTEST_ASSERT_EQ(neigh[2], 26);

   mpc_common_topo_get_pu_neighborhood(0, 3, neigh);

   GTEST_ASSERT_EQ(neigh[0], 0);
   GTEST_ASSERT_EQ(neigh[1], 1);
   GTEST_ASSERT_EQ(neigh[2], 2);
}


TEST_F(TopologyTest, mpc_common_topo_get_numa_node_from_cpu) {
   int i;

   for( i = 0 ; i  < 48 ; i++)
   {
      GTEST_ASSERT_EQ(mpc_common_topo_get_numa_node_from_cpu(i), i/24);
   }
}

TEST_F(TopologyTest, mpc_common_topo_get_numa_node_count) {
   GTEST_ASSERT_EQ(mpc_common_topo_get_numa_node_count(), 2);
}

TEST_F(TopologyTest, mpc_common_topo_has_numa_nodes) {
   GTEST_ASSERT_EQ(mpc_common_topo_has_numa_nodes(), 1);
}

TEST_F(TopologyTest, mpc_common_topo_set_pu_count) {
   mpc_common_topo_set_pu_count(24);

   GTEST_ASSERT_EQ(mpc_common_topo_get_pu_count(), 24);
   GTEST_ASSERT_EQ(mpc_common_topo_get_numa_node_count(), 1);
}

TEST_F(TopologyTest, mpc_common_topo_get_pu_count) {
   GTEST_ASSERT_EQ(mpc_common_topo_get_pu_count(), 48);
}

TEST_F(TopologyTest, mpc_common_topo_get_ht_per_core) {
   GTEST_ASSERT_EQ(mpc_common_topo_get_ht_per_core(), 2);
}


TEST_F(TopologyTest, mpc_common_topo_get_process_cpuset) {
   hwloc_const_cpuset_t pcpuset = mpc_common_topo_get_process_cpuset();

   int pu_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                          pcpuset,
                                                          HWLOC_OBJ_PU);

   GTEST_ASSERT_EQ(pu_count, 48);
}


TEST_F(TopologyTest, mpc_common_topo_get_parent_numa_cpuset) {
   hwloc_const_cpuset_t ncpuset = mpc_common_topo_get_parent_numa_cpuset(0);

   int numa_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                            ncpuset,
                                                            HWLOC_OBJ_NUMANODE);
   GTEST_ASSERT_EQ(numa_count, 1);

   int pu_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                          ncpuset,
                                                          HWLOC_OBJ_PU);
   GTEST_ASSERT_EQ(pu_count, 24);
}

TEST_F(TopologyTest, mpc_common_topo_get_parent_socket_cpuset) {
   hwloc_const_cpuset_t ncpuset = mpc_common_topo_get_parent_socket_cpuset(0);

   int numa_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                            ncpuset,
                                                            HWLOC_OBJ_NUMANODE);
   GTEST_ASSERT_EQ(numa_count, 1);

   int pu_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                          ncpuset,
                                                          HWLOC_OBJ_PU);
   GTEST_ASSERT_EQ(pu_count, 24);
}

TEST_F(TopologyTest, mpc_common_topo_get_parent_core_cpuset) {
   hwloc_const_cpuset_t ncpuset = mpc_common_topo_get_parent_core_cpuset(0);

   int pu_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                          ncpuset,
                                                          HWLOC_OBJ_PU);
   GTEST_ASSERT_EQ(pu_count, 1);
}

TEST_F(TopologyTest, mpc_common_topo_get_pu_cpuset) {
   hwloc_const_cpuset_t ncpuset = mpc_common_topo_get_pu_cpuset(0);

   int pu_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                          ncpuset,
                                                          HWLOC_OBJ_PU);
   GTEST_ASSERT_EQ(pu_count, 1);
}

TEST_F(TopologyTest, mpc_common_topo_get_first_pu_for_level) {

   hwloc_obj_type_t levels[4] = {HWLOC_OBJ_MACHINE, HWLOC_OBJ_PACKAGE, HWLOC_OBJ_NUMANODE, HWLOC_OBJ_PU};
   int counts[4] = {1, 2, 2, 48};

   int i;
   for( i = 0 ; i  < 4 ; i++)
   {

      hwloc_cpuset_t set = mpc_common_topo_get_first_pu_for_level(levels[i]);

      int pu_count = hwloc_get_nbobjs_inside_cpuset_by_depth(mpc_common_topology_get(),
                                                            set,
                                                            HWLOC_OBJ_PU);
      GTEST_ASSERT_EQ(pu_count, counts[i]);

      hwloc_bitmap_free(set);

   }
}



}  // namespace
