#include "hwloc.h"
#include "utlist.h"
#include "mpcomp_places_env.h"

mpcomp_places_info_t*
mpcomp_places_build_interval_infos( const char *env, char *string, char **end )
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

    mpcomp_places_merge_interval( place->interval, include_interval, exclude_interval );
    return (error) ? NULL : place;
}


mpcomp_places_info_t*
mpcomp_places_build_place_infos( const char* env, char *string, char **end )
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

        new_place = mpcomp_places_build_interval_infos( env, string, end );
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
            mpcomp_places_expand_place_bitmap( list, new_place, len, stride );
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
mpcomp_places_get_default_include_bitmap( void )
{
    static hwloc_bitmap_t __default_num_threads_bitmap = NULL; 

    if( !__default_num_threads_bitmap )
    {
        int i;
        char *env = getenv("OMP_NUM_THREADS");
        const int __omp_num_threads = (env) ? atoi(env) : 1; 
        __default_num_threads_bitmap = hwloc_bitmap_alloc();
        hwloc_bitmap_zero( __default_num_threads_bitmap );
        for( i = 0; i < __omp_num_threads ; i++ )
        {
            hwloc_bitmap_set( __default_num_threads_bitmap, i );
        }
    }
    return hwloc_bitmap_dup( __default_num_threads_bitmap);
}

mpcomp_places_info_t*
mpcomp_places_env_variable_parsing(void)
{
    char *tmp, *string, *end;
    mpcomp_places_info_t* list = NULL;

    if( tmp = getenv("OMP_PLACES") )
    {
        const char* env = strdup( tmp ); 
        string = ( char*) env;
        list = mpcomp_places_build_place_infos( env, string, &end );

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
    }
    return list;
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
