#include <iostream>

#include <gtest/gtest.h>
#include "MPC_Common/include/mpc_common_topology.h"
#include "MPC_Common/include/mpc_common_device_topo.h"

#include <stdlib.h>


/*
   THE REFERENCE TOPO:

   MACHINE LEVEL:

      DEV blk0

   NUMA 0

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

        DEV enp55s0f0
        DEV enp55s0f1
        DEV enp55s0f2
        DEV enp55s0f3
        DEV mlx5_0
        DEV ib0

   NUMA 1

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

        DEV enp55s0f4
        DEV enp55s0f5
        DEV enp55s0f6
        DEV enp55s0f7
        DEV mlx5_1
        DEV ib1

        NUMA: 1
                * Node nb 2
*/

namespace {

// The fixture for testing class Foo.
class DevTopologyTest : public ::testing::Test {
 protected:
  // You can remove any or all of the following functions if its body
  // is empty.

  DevTopologyTest() {
     setenv("MPC_SET_XML_TOPOLOGY_FILE", "topo.xml", 1);
     mpc_common_topology_init();
     //mpc_common_topo_print(stderr);
  }

  ~DevTopologyTest() override {
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


TEST_F(DevTopologyTest, mpc_common_topo_device_get_from_handle) {

   mpc_common_topo_device_t * dev = mpc_common_topo_device_get_from_handle((char*)"mlx5_0");

   GTEST_ASSERT_NE(dev, nullptr);

   GTEST_ASSERT_EQ(dev->root_core, 0);
   GTEST_ASSERT_EQ(dev->root_numa, 0);
   ASSERT_STREQ(dev->name, "mlx5_0");
}

TEST_F(DevTopologyTest, mpc_common_topo_device_get_from_handle_regexp) {
   int count = 0;
   mpc_common_topo_device_t ** two_dev = mpc_common_topo_device_get_from_handle_regexp((char*)"mlx.*", &count);

   GTEST_ASSERT_EQ(count, 2);

   GTEST_ASSERT_EQ(two_dev[0]->root_core, 0);
   GTEST_ASSERT_EQ(two_dev[0]->root_numa, 0);
   GTEST_ASSERT_EQ(two_dev[0]->type, MPC_COMMON_TOPO_DEVICE_NETWORK_OFA);
   ASSERT_STREQ(two_dev[0]->name, "mlx5_0");

   GTEST_ASSERT_EQ(two_dev[1]->root_core, 24);
   GTEST_ASSERT_EQ(two_dev[1]->root_numa, 1);
   GTEST_ASSERT_EQ(two_dev[1]->type, MPC_COMMON_TOPO_DEVICE_NETWORK_OFA);
   ASSERT_STREQ(two_dev[1]->name, "mlx5_1");

   free(two_dev);


   mpc_common_topo_device_t ** eight_dev = mpc_common_topo_device_get_from_handle_regexp((char*)"enp.*", &count);

   GTEST_ASSERT_EQ(count, 8);

   int i;
   for(i = 0 ; i < 8 ; i++)
   {
      GTEST_ASSERT_EQ(eight_dev[i]->type, MPC_COMMON_TOPO_DEVICE_NETWORK_HANDLE);
      GTEST_ASSERT_NE(strstr(eight_dev[i]->name, "enp"), nullptr);
   }


   int root_numa[2] = {0};

   for(i = 0 ; i < 8 ; i++)
   {
      int numa = eight_dev[i]->root_numa;

      if( 0 <= numa )
      {
         GTEST_ASSERT_LT(numa, 2);
         root_numa[numa]++;
      }
   }

   GTEST_ASSERT_EQ(root_numa[0], 4);
   GTEST_ASSERT_EQ(root_numa[1], 4);

   free(eight_dev);
}


TEST_F(DevTopologyTest, mpc_common_topo_device_get_closest_from_pu) {

   mpc_common_topo_device_t * response = NULL;

   int i;

   for( i = 0 ; i < 24 ; i++)
   {
      response = mpc_common_topo_device_get_closest_from_pu(i, (char*)"mlx.*" );

      GTEST_ASSERT_NE(response, nullptr);
      ASSERT_STREQ(response->name, "mlx5_0");
   }

   for( i = 24 ; i < 48 ; i++)
   {
      response = mpc_common_topo_device_get_closest_from_pu(24, (char*)"mlx.*" );

      GTEST_ASSERT_NE(response, nullptr);
      ASSERT_STREQ(response->name, "mlx5_1");
   }
}



TEST_F(DevTopologyTest, mpc_common_topo_device_matrix_get_list_closest_from_pu) {
   int count = 0;

   mpc_common_topo_device_t ** list = mpc_common_topo_device_matrix_get_list_closest_from_pu(0,
                                                                                       (char*)"enp.*",
                                                                                       &count );
   GTEST_ASSERT_NE(list, nullptr);
   GTEST_ASSERT_EQ(count, 4);

   int i;

   const char * const possible_dev[4] = {"enp55s0f0",
                                         "enp55s0f1",
                                         "enp55s0f2",
                                         "enp55s0f3"};

   int match_array[4] = {0};

   for( i = 0 ; i  < 4 ; i++)
   {
      int expected_device = 0;
      int j;

      for(j = 0 ; j < 4 ; j++ )
      {
         if(!strcmp(list[i]->name, possible_dev[j]))
         {
            expected_device++;
            match_array[i]++;
         }
      }
      GTEST_ASSERT_EQ(expected_device, 1);
   }

   GTEST_ASSERT_EQ(match_array[0], 1);
   GTEST_ASSERT_EQ(match_array[1], 1);
   GTEST_ASSERT_EQ(match_array[2], 1);
   GTEST_ASSERT_EQ(match_array[3], 1);

   free(list);
}


TEST_F(DevTopologyTest, mpc_common_topo_device_matrix_is_equidistant) {
   int ib_eq = mpc_common_topo_device_matrix_is_equidistant("mlx.*");
   GTEST_ASSERT_EQ(ib_eq, 0);

   int enp_eq = mpc_common_topo_device_matrix_is_equidistant("enp.*");
   GTEST_ASSERT_EQ(enp_eq, 0);

   int blk_eq = mpc_common_topo_device_matrix_is_equidistant("blk.*");
   GTEST_ASSERT_EQ(blk_eq, 1);
}


TEST_F(DevTopologyTest, mpc_common_topo_device_attach_freest_device) {

   int i;

   for(i = 0 ; i < 7 ; i++)
   {
      char name[64];
      snprintf(name, 64, "enp55s0f%d", i);

      mpc_common_topo_device_t * dev = mpc_common_topo_device_get_from_handle( name );

      GTEST_ASSERT_NE(dev, nullptr);

      mpc_common_topo_device_attach_resource(dev);
   }

   /* Now all devs have attached res except 7 lets see if MPC responds 0*/
   mpc_common_topo_device_t *free_dev = mpc_common_topo_device_attach_freest_device( "enp.*" );

   GTEST_ASSERT_NE(free_dev, nullptr);

   ASSERT_STREQ(free_dev->name, "enp55s0f7");
}

TEST_F(DevTopologyTest, mpc_common_topo_device_attach_freest_device_from) {

   int i;
   char name[64];

   for(i = 0 ; i < 4 ; i++)
   {

      snprintf(name, 64, "enp55s0f%d", i);

      mpc_common_topo_device_t * dev = mpc_common_topo_device_get_from_handle( name );

      GTEST_ASSERT_NE(dev, nullptr);

      mpc_common_topo_device_attach_resource(dev);
   }

   /* Build a dev set only 0-3 (not 4) is loaded */

   mpc_common_topo_device_t * dev_set[2];
   snprintf(name, 64, "enp55s0f0");
   dev_set[0] = mpc_common_topo_device_get_from_handle( name );
   snprintf(name, 64, "enp55s0f4");
   dev_set[1] = mpc_common_topo_device_get_from_handle( name );

   mpc_common_topo_device_t * free_dev = mpc_common_topo_device_attach_freest_device_from( dev_set, 2 );

   GTEST_ASSERT_NE(free_dev, nullptr);

   ASSERT_STREQ(free_dev->name, "enp55s0f4");
}



}  // namespace
