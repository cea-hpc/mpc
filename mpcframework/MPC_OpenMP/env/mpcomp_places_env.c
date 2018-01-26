#include "hwloc.h"
#include "utlist.h"
#include "sctk_spinlock.h"
#include "sctk_topology.h"
#include "mpcomp_places_env.h"

mpcomp_places_info_t*
mpcomp_places_build_interval_infos( const char *env, char *string, char **end, const int nb_mvps )
{
    mpcomp_places_info_t* place;
    int res, num_places, stride, exclude, error;
    hwloc_bitmap_t bitmap_tmp, bitmap_to_update;
    hwloc_bitmap_t include_interval, exclude_interval;
    
    if( *string == '\0') return NULL;

    place = mpcomp_places_infos_init(1);

    include_interval = hwloc_bitmap_alloc();
    assert( include_interval );
    hwloc_bitmap_zero(include_interval);

    exclude_interval = hwloc_bitmap_alloc();
    assert( exclude_interval );
    hwloc_bitmap_zero(exclude_interval); 

    error = 0;
    while(1)
    {
        stride = 1;
        exclude = 0;
        num_places = 1;
        bitmap_to_update = include_interval;
        
        if( *string == '!' )
        {
            string++;
            exclude = 1;
            bitmap_to_update = exclude_interval;
        }
        
        res = mpcomp_safe_atoi( env, string, end );
        string = *end;

        if( !exclude )
        {
            if( *string == ':' )
            {
                string++;
                num_places = mpcomp_safe_atoi( env, string, end );
                if( string == *end )
                {
                    error = 1; // missing value
                    break;
                }
                if( num_places <= 0 )
                {
                    error = 1;
                    fprintf(stderr, "error: num_places in subplace is not positive integer ( %d )\n", num_places ); 
                    break;
                }
                string = *end;
            }
            if( *string == ':' )
            {
                string++;
                stride = mpcomp_safe_atoi( env, string, end );
                if( string == *end )
                {
                    error = 1; // missing value
                    break;
                }
                string = *end;
            }
        }
        
        bitmap_tmp = mpcomp_places_build_interval_bitmap(res, num_places, stride);    
        hwloc_bitmap_or( bitmap_to_update, bitmap_to_update, bitmap_tmp ); 
        hwloc_bitmap_free( bitmap_tmp );
        
        if( *string != ',' ) break;
        string++; // skip ','
    }

    mpcomp_places_merge_interval( place->interval, include_interval, exclude_interval, nb_mvps );
    return (error) ? NULL : place;
}

static char* 
mpcomp_places_build_numas_places( const int places_number, int* error )
{
   int numa_found;
   size_t cnwrites, tnwrites;
   char* places_string;
   hwloc_topology_t topology;
   hwloc_obj_t prev_numa, next_numa;
   hwloc_obj_t prev_pu, next_pu;

   topology = sctk_get_topology_object();
   prev_numa = NULL;

   places_string = (char*) malloc( 4096 * sizeof( char ) );
   assert( places_string );
   
   numa_found = 0; tnwrites = 0;
   const int number_numas = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_NUMANODE );
   const int __places_number = ( places_number == -1 ) ? number_numas : places_number;
   while( next_numa = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_NUMANODE, prev_numa ) )
   {  
      /* Level with no PU ... skip */
      if( hwloc_get_nbobjs_inside_cpuset_by_type( topology, next_numa->cpuset, HWLOC_OBJ_PU ) <= 0 )
          continue;
       
      size_t current_index = 0; int pu_found = 0;
      char currend_bitmap_list[255]; prev_pu = NULL;

      while( next_pu = hwloc_get_next_obj_inside_cpuset_by_type( topology, next_numa->cpuset, HWLOC_OBJ_PU, prev_pu ) )
      {
         int tmp_write;
         assert( current_index < 254 );
         if( pu_found ) 
             tmp_write = snprintf( currend_bitmap_list+current_index, 255 - current_index, ",%d", next_pu->logical_index );
         else
             tmp_write = snprintf( currend_bitmap_list+current_index, 255 - current_index, "%d", next_pu->logical_index );
         current_index += tmp_write;
         pu_found++; prev_pu = next_pu;
      }

      assert( current_index < 254 ); 
      currend_bitmap_list[current_index] = '\0';
 
      /* {"pulist"} */
      const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( numa_found ) ? 1 : 0 );
      assume( tnwrites + size2copy < 4096 );

      if( !numa_found )
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list ); 
      else
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );

      tnwrites += cnwrites;

      prev_numa = next_numa;
      numa_found++;
      
      /* Enough PLACE for the user */
      if( numa_found == __places_number )
         break;
   }

   if( numa_found < __places_number )
      fprintf(stderr, "Resize number places from %d to %d\n", __places_number, numa_found );

   if( !numa_found )
   {
      fprintf(stderr, "error: No numa nodes found on node\n");
      free( places_string );
      places_string = NULL;
      assert( error );
      *error = 1;
   }
   else
   {
      assume( tnwrites + 1 < 4096 );
      places_string[tnwrites] = '\0';
      assert( error );
      *error = 0;
   }

   return places_string;
}

static char* 
mpcomp_places_build_sockets_places( const int places_number, int *error )
{
   int socket_found;
   size_t cnwrites, tnwrites;
   char* places_string;
   hwloc_topology_t topology;
   hwloc_obj_t prev_socket, next_socket;
   hwloc_obj_t prev_pu, next_pu;

   topology = sctk_get_topology_object();
   prev_socket = NULL;

   places_string = (char*) malloc( 4096 * sizeof( char ) );
   assert( places_string );
   
   socket_found = 0; tnwrites = 0;
   const int number_sockets = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_SOCKET );
   const int __places_number = ( places_number == -1 ) ? number_sockets : places_number;
   while( next_socket = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_SOCKET, prev_socket ) )
   {  
      /* Level with no PU ... skip */
      if( hwloc_get_nbobjs_inside_cpuset_by_type( topology, next_socket->cpuset, HWLOC_OBJ_PU ) <= 0 )
          continue;
 
      size_t current_index = 0; int pu_found = 0;
      char currend_bitmap_list[255]; prev_pu = NULL;

      while( next_pu = hwloc_get_next_obj_inside_cpuset_by_type( topology, next_socket->cpuset, HWLOC_OBJ_PU, prev_pu ) )
      {
         int tmp_write;
         assert( current_index < 254 );
         if( pu_found )
             tmp_write = snprintf( currend_bitmap_list+current_index, 255 - current_index, ",%d", next_pu->logical_index );
         else
             tmp_write = snprintf( currend_bitmap_list+current_index, 255 - current_index, "%d", next_pu->logical_index );
         current_index += tmp_write;
         pu_found++; prev_pu = next_pu;
      }
      
      /* {"pulist"} */
      const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( socket_found ) ? 1 : 0 );
      assume( tnwrites + size2copy < 4096 );

      if( !socket_found )
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list ); 
      else
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );

      tnwrites += cnwrites;

      prev_socket = next_socket;
      socket_found++;
      
      /* Enough PLACE for the user */
      if( socket_found == __places_number )
         break;
   }

   if( socket_found < __places_number )
      fprintf(stderr, "warning: Resize number places from %d to %d\n", __places_number, socket_found );

   if( !socket_found )
   {
      fprintf(stderr, "error: No socket found on node\n"); 
      free( places_string );
      places_string = NULL;
      assert( error );
      *error = 1;
   }
   else
   {
      assume( tnwrites + 1 < 4096 );
      places_string[tnwrites] = '\0';
      assert( error );
      *error = 0;
   }

   return places_string;
}

static char* 
mpcomp_places_build_threads_places( const int places_number, int* error )
{
   int pu_found;
   size_t cnwrites, tnwrites;
   char* places_string;
   hwloc_topology_t topology;
   hwloc_obj_t prev_pu, next_pu;

   topology = sctk_get_topology_object();
   prev_pu = NULL;

   places_string = (char*) malloc( 4096 * sizeof( char ) );
   assert( places_string );
   
   pu_found = 0; tnwrites = 0;
   const int threads_number = hwloc_get_nbobjs_by_type( topology, HWLOC_OBJ_PU );
   const int __places_number = ( places_number == -1 ) ? threads_number : places_number;
   while( next_pu = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_PU, prev_pu ) )
   {
      char currend_bitmap_list[255];
      hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, next_pu->cpuset );
      
      /* {"pulist"} */
      const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( ( pu_found ) ? 1 : 0 );
      assume( tnwrites + size2copy < 4096 );

      if( !pu_found )
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list ); 
      else
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );

      tnwrites += cnwrites;

      prev_pu = next_pu;
      pu_found++;
      
      /* Enough PLACE for the user */
      if( pu_found == __places_number )
         break;
   }

   if( pu_found < __places_number )
      fprintf(stderr, "Resize number places from %d to %d\n", __places_number, pu_found );

   assume( tnwrites + 1 < 4096 );
   places_string[tnwrites] = '\0';

   assert( error );
   *error = 0;

   return places_string;
}

static char* 
mpcomp_places_build_cores_places( const int places_number, int *error )
{
   int core_found;
   size_t cnwrites, tnwrites;
   char* places_string;
   hwloc_topology_t topology;
   hwloc_obj_t prev_core, next_core; 

   topology = sctk_get_topology_object();
   prev_core = NULL;

   places_string = (char*) malloc( 4096 * sizeof( char ) );
   assert( places_string );
   
   core_found = 0; tnwrites = 0;
   const int core_number = hwloc_get_nbobjs_by_type( topology,  HWLOC_OBJ_CORE );
   const int pu_number = hwloc_get_nbobjs_by_type( topology,  HWLOC_OBJ_PU);

   const int __places_number = ( places_number == -1 ) ? core_number : places_number;
   while( next_core = hwloc_get_next_obj_by_type( topology, HWLOC_OBJ_CORE, prev_core ) )
   {
      char currend_bitmap_list[255];
      hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, next_core->cpuset );
      /* {"pulist"} */
      const size_t size2copy = strlen( currend_bitmap_list ) + 2 + ( (core_found ) ? 1 : 0 );
      assume( tnwrites + size2copy < 4096 );

      if( !core_found )
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, "{%s}", currend_bitmap_list ); 
      else
         cnwrites = snprintf( places_string+tnwrites, 4096 - tnwrites, ",{%s}", currend_bitmap_list );

      tnwrites += cnwrites;
      
      prev_core = next_core;
      core_found++;
      
      /* Enough PLACE for the user */
      if( core_found == __places_number )
         break;
   } 

   if( core_found < __places_number )
      fprintf(stderr, "Resize number places from %d to %d\n", __places_number, core_found );

   assume( tnwrites + 1 < 4096 );
   places_string[tnwrites] = '\0';

   assert( error );
   *error = 0;

   return places_string;
}

static int mpcomp_places_named_extract_num( const char* env, char *string  )
{
   char* next_char;

   if( *string == '\0' ) return -1; /* Dynamic size */
   if( *string != '(' ) return 0; /* Error */

   string++;   // skip '('   
  
   const int num_places = mpcomp_safe_atoi( NULL, string, &next_char);
   
   if( num_places <= 0 || string == next_char ) 
   {
      if( num_places <= 0 ) 
         fprintf(stderr, "error: len in place is not positive integer ( %d )\n", num_places ); 
      return 0;
   } 

   string = next_char;

   if( *string != ')' )
      return 0;
  
   string++;   // skip ')' 

   if( 0 && string != '\0' )
   {
      fprintf(stderr,  "error: offset %d with \'%c\' end: %s\n", string - env -1, *string, string );
      return 0;
   }

   return num_places;
}

static char*
mpcomp_places_is_named_places( const char* env, char *string, int* error )
{
   if( !strncmp( string, "threads", 7 ) )
   {
      const int num_places = mpcomp_places_named_extract_num( env, string+7 );
      if( !num_places ) return NULL;
      return mpcomp_places_build_threads_places( num_places, error );
   }

   if( !strncmp( string, "cores", 5 ) )
   {
      const int num_places = mpcomp_places_named_extract_num( env, string+5 );
      if( !num_places ) return NULL; 
      return mpcomp_places_build_cores_places( num_places, error );
   }

   if( !strncmp( string, "sockets", 7 )  )
   {
      const int num_places = mpcomp_places_named_extract_num( env, string+7 );
      if( !num_places ) return NULL; 
      return mpcomp_places_build_sockets_places( num_places, error );
   }

   if( !strncmp( string, "numas", 5 ) )
   {
      const int num_places = mpcomp_places_named_extract_num( env, string+5 );
      if( !num_places ) return NULL;
      return mpcomp_places_build_numas_places( num_places, error );
   }

   *error = 0;
   return NULL;
}

mpcomp_places_info_t*
mpcomp_places_build_place_infos( const char* env, char *string, char **end, const int nb_mvps )
{
    int error, exclude, stride, len;
    mpcomp_places_info_t* new_place, *list;
    static int __mpcomp_places_generate_count = 0; 

    list = NULL; // init utlist.h
    if( *string == '\0') return NULL;


    while(1)
    {
        len = 1;
        stride = 1;
        exclude = 0;

        if( *string == '!' ) 
        {
            exclude = 1;
            string++;   // skip '!'
        }

        if( *string != '{' )
        {
            error = 1;
            break;
        }
        string++; // skip '{'

        new_place = mpcomp_places_build_interval_infos( env, string, end, nb_mvps );
        if( !new_place )
        {
            fprintf(stderr, "error: can't parse subplace\n");
            error = 1;
            break;
        }
        
        DL_APPEND( list, new_place );
        new_place->id = __mpcomp_places_generate_count++;
        string = *end;
       
        if( *string != '}' )
        {
            error = 1;
            break;
        }
        string++; // skip '}'
         
        if( !exclude )
        {
            if( *string == ':' )
            {
                string++;
                len = mpcomp_safe_atoi( env, string, end );
                if( string == *end )
                {
                    error = 1; // missing value
                    break;
                }
                if( len <= 0 )
                {
                    error = 1;
                    fprintf(stderr, "error: len in place is not positive integer ( %d )\n", len );
                    break;
                }
                string = *end;
            }
            if( *string == ':' )
            {
                string++;
                stride = mpcomp_safe_atoi( env, string, end );
                if( string == *end )
                {
                    error = 1; // missing value
                    break;
                }
                string = *end;
            }

            __mpcomp_places_generate_count += ( len - 1 );
            mpcomp_places_expand_place_bitmap( list, new_place, len, stride, nb_mvps );
        }
        else
        {
            hwloc_bitmap_not( new_place->interval, new_place->interval );     
        }


        if( *string != ',' ) break;
        string++; // skip ','
    }

    *end = string;
    return list;
}

hwloc_bitmap_t
mpcomp_places_get_default_include_bitmap( const int nb_mvps )
{
     int i;
     hwloc_obj_t pu;
     static volatile hwloc_bitmap_t __default_num_threads_bitmap = NULL;
     static sctk_spinlock_t multiple_tasks_lock = SCTK_SPINLOCK_INITIALIZER;
 
     if( !__default_num_threads_bitmap )
     {
        sctk_spinlock_lock( &multiple_tasks_lock );
        if( !__default_num_threads_bitmap )
        {
           hwloc_bitmap_t __tmp_default_bitmap_places = hwloc_bitmap_alloc();
           hwloc_topology_t topology = sctk_get_topology_object(); 
           hwloc_bitmap_zero ( __tmp_default_bitmap_places );
           for( i = 0; i < nb_mvps ; i++ )
           {
              pu = hwloc_get_obj_by_type( topology, HWLOC_OBJ_PU, i );
              if( pu )
                 hwloc_bitmap_or( __tmp_default_bitmap_places, __tmp_default_bitmap_places, pu->cpuset );
           }
           __default_num_threads_bitmap = __tmp_default_bitmap_places;
        }
        sctk_spinlock_unlock( &multiple_tasks_lock );
    }

#if 0   /* DEBUG */
    char currend_bitmap_list[256];
    hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, __default_num_threads_bitmap );
    fprintf(stderr, "Default cpuset : %s w/ %d threads\n", currend_bitmap_list, hwloc_bitmap_weight( __default_num_threads_bitmap ) );
#endif  /* DEBUG */

    return hwloc_bitmap_dup( __default_num_threads_bitmap );
}

void mpcomp_display_places( mpcomp_places_info_t* list )
{
    int count = 0;
    char currend_bitmap_list[256];
    mpcomp_places_info_t* place, *saveptr;

    DL_FOREACH_SAFE( list, place, saveptr )
    {
        hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, place->interval ); 
        currend_bitmap_list[255] = '\0';
        fprintf(stdout, "place #%d: %s\n", count++, currend_bitmap_list );
    }
}

int mpcomp_places_get_real_nb_mvps( mpcomp_places_info_t* list )
{
   int real_nb_mvps = 0;
   mpcomp_places_info_t* place, *saveptr;
   DL_FOREACH_SAFE( list, place, saveptr )
   {
      real_nb_mvps += hwloc_bitmap_weight( place->interval );
   }
   return real_nb_mvps;
}

static inline void __mpcomp_places_translate_cpu_list( int** cpus_order, const int cpu_number )
{
   int* __cpus_order;
   int i, next, prev, pos_in_bitmap;
   hwloc_bitmap_t __cpus_order_set = hwloc_bitmap_alloc();
   hwloc_bitmap_zero ( __cpus_order_set );

   __cpus_order = *cpus_order;

   /* Build a set */
   for( i = 0; i < cpu_number; i++ )
      hwloc_bitmap_set( __cpus_order_set, __cpus_order[i] );


#if 0   /* DEBUG */
    char currend_bitmap_list[256];
    hwloc_bitmap_list_snprintf( currend_bitmap_list, 255, __cpus_order_set );
    fprintf(stderr, "%s:%d : %s w/ %d threads\n", __func__, __LINE__, currend_bitmap_list, hwloc_bitmap_weight( __cpus_order_set ) );
#endif  /* DEBUG */
   
   prev = -1; pos_in_bitmap = 0;
   while( ( next = hwloc_bitmap_next( __cpus_order_set, prev ) ) != -1 )
   {
      for( i = 0; i < cpu_number; i++ )
      {
         if( __cpus_order[i] == next ) 
         {
             __cpus_order[i] = pos_in_bitmap++;
             break;
         }
      }
      prev = next;
   }
}

int mpcomp_places_get_topo_info( mpcomp_places_info_t* list, int** shape, int** cpus_order )
{
   int __cur_cpu_id, __place_cur_id, __place_old_id;
   mpcomp_places_info_t* place, *saveptr;
   int * __place_shape, *__cpus_order;

   __place_shape = (int*) malloc( sizeof(int) * 2 );
   assert( __place_shape );
   memset( __place_shape, 0, sizeof(int) * 2 );
   
   DL_COUNT( list, place, __place_shape[0] );

   /* No DL_FIRST ... bypass utlist API */
   __place_shape[1] = hwloc_bitmap_weight( list->interval );

   //fprintf(stderr, "Tree Shape : %d %d\n", __place_shape[0], __place_shape[1] );

   const int __places_nb_mvps = mpcomp_places_get_real_nb_mvps( list );
   assert( __place_shape[0] * __place_shape[1] == __places_nb_mvps );
  
   /* Build cpu order */ 
   __cpus_order = (int*) malloc( sizeof(int) * __places_nb_mvps ); 
   assert( __cpus_order );
   memset( __cpus_order, 0, sizeof(int) * __places_nb_mvps );
  
   __cur_cpu_id = 0; 
   DL_FOREACH_SAFE( list, place, saveptr )
   {
      __place_old_id = -1;
      const int place_num_elts = hwloc_bitmap_weight( place->interval ); 
      /* Get first elt */
      while( ( __place_cur_id = hwloc_bitmap_next( place->interval, __place_old_id ) ) >= 0 )
      {
         __cpus_order[__cur_cpu_id++] = __place_cur_id;
         __place_old_id = __place_cur_id;
      }
   }
   
   /* Rename CPU PROC in places */
   __mpcomp_places_translate_cpu_list( &__cpus_order, __cur_cpu_id );

   /* Build logical mpc task cpu in places */  
   int __rename_cur_cpu_id = 0;
   DL_FOREACH_SAFE( list, place, saveptr )
   {
      int i;
      const int place_num_elts = hwloc_bitmap_weight( place->interval );
      place->logical_interval = hwloc_bitmap_alloc();
      hwloc_bitmap_zero ( place->logical_interval ); 

      for( i = 0; i < place_num_elts; i++ )
         hwloc_bitmap_set( place->logical_interval, __cpus_order[ __rename_cur_cpu_id++ ] );
   }

#if 0 /* Begin debug infos */
   fprintf(stderr, "CPU order: ");
   int i;
   for( i = 0; i < __places_nb_mvps; i++ )
      fprintf(stderr, "%d ", __cpus_order[i] );
   fprintf(stderr, "\n");
#endif /* End debug infos */

   /* Return PLACES INFOS */
   *shape = __place_shape;
   *cpus_order = __cpus_order;
   return __places_nb_mvps;
}


mpcomp_places_info_t*
mpcomp_places_env_variable_parsing(const int nb_mvps)
{
    int error;
    char *tmp, *string, *end;
    mpcomp_places_info_t* list = NULL;

    if( !( tmp = getenv("OMP_PLACES") ) )
    {
       tmp = (char*) malloc( 64 * sizeof( char )); 
       snprintf( tmp, 63, "cores" );
    }
   
    const char* prev_env = strdup( tmp );
    string = ( char* ) prev_env; 

    const char *named_str = mpcomp_places_is_named_places( prev_env, string, &error );

    if( error ) 
    {
       fprintf(stderr, "error : OMP_PLACES ignored\n" );  
       return NULL;
    }

    const char* env = (named_str) ? strdup(named_str) : strdup( tmp );
    string = ( char*) env;

    if( !env )
    {
       fprintf(stderr, "error: Can't parse named places"); 
       return NULL;
    }

    list = mpcomp_places_build_place_infos( env, string, &end, nb_mvps );

    if( *end != '\0' )
    {
       fprintf(stderr,  "error: offset %d with \'%c\' end: %s\n", end - env -1, *end, end );
       return NULL;
    }

    if( mpcomp_places_detect_collision( list ) )
    {
       fprintf(stderr, "MPC doesn't support collision between places");
       return NULL;
    }

    if( mpcomp_places_detect_heretogeneous_places( list ) )
    {
       fprintf(stderr, "Every place must have the same number of threads");
       return NULL;
    }

    return list;
}
