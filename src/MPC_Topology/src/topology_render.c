#include "topology_render.h"

#include <sys/types.h>
#include <sys/syscall.h>
#include <mpc_common_debug.h>
#include <mpc_common_spinlock.h>
#include <mpc_topology.h>
#include <mpc_common_rank.h>
#include <mpc_common_flags.h>

#include <sctk_alloc.h>

static char file_placement[128];
static char placement_txt[128];
static char textual_file[128];
static char textual_file_output[128];

static hwloc_topology_t topology_compute_node;

typedef struct sctk_text_option_s{
                int *os_index ;
                int *vp_tab ;
                int *rank_mpi;
                int *compact_tab ;
                int *scatter_tab ;
                int *balanced_tab ;
                int *pid_tab;
} sctk_text_option_t;


/* used by the graphical option */
static hwloc_bitmap_t topology_cpuset_compute_node;



/* return the os index of the topology_compute_node of the thread executed */
int mpc_topology_render_get_current_binding()
{
	hwloc_cpuset_t set = hwloc_bitmap_alloc();

	int ret = hwloc_get_last_cpu_location( topology_compute_node, set, HWLOC_CPUBIND_THREAD );

	assume( ret != -1 );
	assume( !hwloc_bitmap_iszero( set ) );

	hwloc_obj_t pu;
	if ( mpc_common_get_flags()->enable_smt_capabilities )
	{
		pu = hwloc_get_obj_inside_cpuset_by_type( topology_compute_node, set, HWLOC_OBJ_PU, 0 );
	}
	else
	{
		pu = hwloc_get_obj_inside_cpuset_by_type( topology_compute_node, set, HWLOC_OBJ_CORE, 0 );
	}

	if ( !pu )
	{
		hwloc_bitmap_free( set );
		return -1;
	}

	/* return the os index */
	int cpu_os = pu->os_index;
	//int cpu = mpc_topology_render_get_logical_from_os_id(cpu_os);

	return cpu_os;
}

/* return logical index from topology_compute_node os index */
int mpc_topology_render_get_logical_from_os_id( unsigned int cpu_os )
{
	/* hwloc_get_pu_obj_by_os_inde give false resultat i suppose */
	/*hwloc_obj_t cpu = hwloc_get_pu_obj_by_os_index(topology_compute_node, cpu_os);
    return cpu->logical_index;*/
	unsigned int nb;
	unsigned int i;
	if ( mpc_common_get_flags()->enable_smt_capabilities )
	{
		nb = hwloc_get_nbobjs_by_type( topology_compute_node, HWLOC_OBJ_PU );

		for ( i = 0; i < nb; i++ )
		{
			if ( hwloc_get_obj_by_type( topology_compute_node, HWLOC_OBJ_PU, i )->os_index == cpu_os )
			{
				hwloc_obj_t pu = hwloc_get_obj_by_type( topology_compute_node, HWLOC_OBJ_PU, i );
				return pu->logical_index;
			}
		}
	}
	else
	{
		nb = hwloc_get_nbobjs_by_type( topology_compute_node, HWLOC_OBJ_CORE );

		for ( i = 0; i < nb; i++ )
		{
			if ( hwloc_get_obj_by_type( topology_compute_node, HWLOC_OBJ_CORE, i )->os_index == cpu_os )
			{
				hwloc_obj_t pu = hwloc_get_obj_by_type( topology_compute_node, HWLOC_OBJ_CORE, i );
				return pu->logical_index;
			}
		}
	}
	return -1;
}

/* return the os index of the topology_compute_node from logical index */
int mpc_topology_render_get_current_binding_from_logical( int logical_pu )
{
	hwloc_obj_t pu;
	if ( mpc_common_get_flags()->enable_smt_capabilities )
	{
		pu = hwloc_get_obj_by_type( topology_compute_node, HWLOC_OBJ_PU, logical_pu );
	}
	else
	{
		pu = hwloc_get_obj_by_type( topology_compute_node, HWLOC_OBJ_CORE, logical_pu );
	}

	if ( pu )
	{
		return pu->os_index;
	}
	else
	{
		return -1;
	}
}


/* used by graphic option */
static void compute_master_color(int r, int g, int b,  int *r_m, int *g_m, int *b_m){
	const int color_shift = 25;
	*r_m = r + color_shift;
	*g_m = g + color_shift;
	*b_m = b + color_shift;
}

/* used by graphic option */
static void choose_color_task(int task_id, int nb_task, int *r, int *g, int *b, int *r_m, int *g_m, int *b_m){
    int red, green, blue;
    int red_m, green_m, blue_m;
    int step = (230) / nb_task;

    switch((task_id + 6) % 6){
        case 0:
            red = 230;
            blue = 0;
			green = 0 + step * task_id;
            break;

        case 1:
            green = 230;
            blue = 0;
			red = 230 - step * task_id;
            break;

        case 2:
            red = 0;
            green = 230;
			blue = 0 + step * task_id;
            break;
        case 3:
            red = 0;
            blue = 230;
			green = 230 - step * task_id;
            break;
        case 4:
            green = 0;
            blue = 230;
			red = 0 + step * task_id;
            break;
        case 5:
            red = 230;
            green = 0;
			blue = 230 - step * task_id;
            break;
        default:
            red = 0;
            green = 0;
            blue = 0;
            red_m = 0;
            green_m = 0;
            blue_m = 0;
            break;

    }

	compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);

    *r = red;
    *g = green;
    *b = blue;
    *r_m = red_m;
    *g_m = green_m;
    *b_m = blue_m;
}

static char *convert_rgb_to_string(int red, int green, int blue, char * rgb){
    char r[8];
    char g[8];
    char b[8];
    char temp1[8];
    char temp2[8];
    char temp3[8];
    sprintf(r, "%x", red);
    sprintf(g, "%x", green);
    sprintf(b, "%x", blue);
    char * r1;
    char * g1;
    char * b1;
    if(red < 16){
        sprintf(temp1, "%d", 0);
        r1 = strcat(temp1, r);
    }
    else{
        temp1[0] = '\0';
        r1 = strcat(temp1, r);
    }
    if(green < 16){
        temp2[0] = '\0';
        g1 = strcat(temp2, g);
    }
    else{
        temp2[0] = '\0';
        g1 = strcat(temp2, g);
    }
    if(blue < 16){
        sprintf(temp3, "%d", 0);
        b1 = strcat(temp3, b);
    }
    else{
        temp3[0] = '\0';
        b1 = strcat(temp3, b);
    }
    char *temp = strcat(g1,b1);
    strcat(r1, temp);
    strcpy(rgb, r1);
    return rgb;
}

/* fill thread placement informations in file to communicate between processes of the same node for text placement option */
void mpc_topology_render_text(int os_pu, int os_master_pu, int task_id, int vp, __UNUSED__ int rank_open_mp, int* min_idex, int pid){

    /*acces global topo*/
    hwloc_topology_load(mpc_topology_get());
    /*add color info on pu*/



    if(os_pu == os_master_pu){
            /* implemtation txt */
            FILE *f_textual = fopen(textual_file, "a");
            if(f_textual != NULL){
                fprintf(f_textual,"%d %d %d %d %d %d %d\n", os_master_pu, vp, task_id, min_idex[0], min_idex[1],min_idex[2] , pid);
                fclose(f_textual);
            }
    }
    else{
            /* implemtation txt */
            FILE *f_textual = fopen(textual_file, "a");
            if(f_textual != NULL){
                fprintf(f_textual,"%d %d %d %d %d %d %d\n", os_pu, vp, task_id, min_idex[0], min_idex[1],min_idex[2], pid);
                fclose(f_textual);
            }

    }
}

/* fill thread placement informations in file to communicate between processes of the same node for graphic placement option */
void mpc_topology_render_create(int os_pu, int os_master_pu, int task_id){
    int red, green, blue;
    int red_m, green_m, blue_m;
    char string_rgb_hexa[512];
    char string_rgb_hexa_master[512];


    choose_color_task(task_id, mpc_common_get_task_count(), &red, &green, &blue, &red_m, &green_m, &blue_m);

    convert_rgb_to_string(red, green, blue, string_rgb_hexa);
    convert_rgb_to_string(red, green, blue, string_rgb_hexa_master);

    /*acces global topo*/
    hwloc_topology_load(mpc_topology_get());
    /*add color info on pu*/


    if(os_pu == os_master_pu){
        char temp_background_master[128];

        sprintf(temp_background_master, "Background=#");
        strcat(temp_background_master, string_rgb_hexa_master);
        strcpy(string_rgb_hexa_master, temp_background_master);

        strcat( string_rgb_hexa_master, ";");
        strcat(string_rgb_hexa_master, "Text=#ffffff");
            /* implemtation txt */
            FILE *f = fopen(placement_txt, "a");
            if(f != NULL){
                fprintf(f,"%d %s\n", os_master_pu, string_rgb_hexa_master);
                fclose(f);
            }
    }
    else{
            char temp_background[128];

            sprintf(temp_background, "Background=#");
            strcat(temp_background, string_rgb_hexa);
            strcpy(string_rgb_hexa, temp_background);

            strcat( string_rgb_hexa, ";");
            strcat(string_rgb_hexa, "Text=#000000");
            /* implemtation txt */
            FILE *f = fopen(placement_txt, "a");
            if(f != NULL){
                fprintf(f,"%d %s\n", os_pu, string_rgb_hexa);
                fclose(f);
            }
    }
}

static mpc_common_spinlock_t __lock_graphic = MPC_COMMON_SPINLOCK_INITIALIZER;

void mpc_topology_render_lock()
{
    mpc_common_spinlock_lock(&__lock_graphic);
}

void mpc_topology_render_unlock()
{
    mpc_common_spinlock_unlock(&__lock_graphic);
}


void mpc_topology_render_notify(int task_id)
{

    /* graphic placement option */
    if(mpc_common_get_flags()->enable_topology_graphic_placement){
        /* get os ind */
        int master = mpc_topology_render_get_current_binding();
        mpc_topology_render_lock();
        /* fill file to communicate between process of the same compute node */
        mpc_topology_render_create(master, master, task_id);
        mpc_topology_render_unlock();
    }
	/* text placement option */
    if(mpc_common_get_flags()->enable_topology_text_placement){
        int master = mpc_topology_render_get_current_binding();
        mpc_topology_render_lock();
        /* need to lock to write in the node file for each mpi master of the processus */
        int min_index[3] = {0,0,0};
        mpc_topology_render_text(master, master, task_id, 0, 0, min_index, syscall(SYS_gettid));
        mpc_topology_render_unlock();
    }
}


/* used by graphic and text option */
static void read_char(char * buff, int *cpt, int *c, FILE *f){
    buff[*cpt] = (char)*c;
    (*cpt)++;
    *c = getc(f);
}

/* used by graphic and text option */
static void sctk_read_format_option_graphic_placement_and_complet_topo_infos(FILE *f, int lenght){
    while(1){
        //malloc buffer for infos of other ranks in the same processus
        char * os_indbuff = (char *)malloc(64*lenght);
        char * infosbuff = (char *)malloc(64*lenght);
        int c = getc(f);
        if(c == EOF){
            break;
        }
        /* read os ind infos */
        int cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(os_indbuff, &cpt, &c, f);
        }
        if(c == EOF){
            break;
        }
        os_indbuff [cpt] = '\0';
        char os_ind[cpt + 1];
        strncpy(os_ind, os_indbuff , (cpt+ 1));
        c = getc(f);
        cpt = 0;
        while((c != '\n') && (c != EOF)){
            read_char(infosbuff, &cpt, &c, f);
        }

        /* read infos lstopoStyle */
        infosbuff[cpt] = '\0';
        char infos[cpt + 1];
        strncpy(infos, infosbuff, (cpt+ 1));
        if(c == EOF){
            break;
        }
        int logical_ind =
            mpc_topology_render_get_logical_from_os_id(atoi(os_ind));
        hwloc_obj_t obj;
        if(mpc_common_get_flags()->enable_smt_capabilities){
            obj = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_PU, logical_ind);
        }
        else{
            obj = hwloc_get_obj_by_type(mpc_topology_get(), HWLOC_OBJ_CORE, logical_ind);
        }
        if(!obj){
            sctk_free(infosbuff);
            sctk_free(os_indbuff );
            return;
        }

        hwloc_obj_add_info(obj, "lstopoStyle", infos);

        sctk_free(infosbuff);
        sctk_free(os_indbuff );
    }
    fclose(f);
}


static void transform_char(char *str){
    int x = 0;
    while(str[x] != '\0'){
        if(str[x] == ' '){
            str[x] = '_';
        }
        x++;
    }
    str[x - 1] = str[x];
}

/* used by graphic and text option */
static void name_and_date_file_text(char *file_name){

    time_t timestamp = time(NULL);
    char *buffer= ctime(&timestamp);
    transform_char(buffer);
    strcat(file_name, "_");
    strcat(file_name, buffer);
}

/* init struct for text placement option */
static void sctk_init_text_option(struct sctk_text_option_s **tab_option){
    int lenght = hwloc_get_nbobjs_by_type(mpc_topology_get(), HWLOC_OBJ_PU);
    *tab_option = (struct sctk_text_option_s *)malloc(sizeof(struct sctk_text_option_s));
    (*tab_option)->os_index = (int *)malloc(sizeof(int)*lenght);
    (*tab_option)->vp_tab = sctk_malloc(sizeof(int)*lenght);
    (*tab_option)->rank_mpi= sctk_malloc(sizeof(int)*lenght);
    (*tab_option)->compact_tab = sctk_malloc(sizeof(int)*lenght);
    (*tab_option)->scatter_tab = sctk_malloc(sizeof(int)*lenght);
    (*tab_option)->balanced_tab = sctk_malloc(sizeof(int)*lenght);
    (*tab_option)->pid_tab = sctk_malloc(sizeof(int)*lenght);
    int l=0;
    struct sctk_text_option_s * temp = *tab_option;
    for(l=0; l< lenght; l++){
        temp->vp_tab[l] = -1;
        temp->os_index[l] = -1;
        temp->rank_mpi[l] = -1;
        temp->compact_tab[l] = -1;
        temp->scatter_tab[l] = -1;
        temp->balanced_tab[l] = -1;
        temp->pid_tab[l] = -1;
    }
}

/* used by the reader for text placement option file */
static void convert_char(char * buff,int cpt, int *tab, int cpt_line){
    buff [cpt] = '\0';
    char temp[cpt + 1];
    strncpy(temp, buff , (cpt+ 1));
    tab[cpt_line] = atoi(temp);
}

/* read file of the node for text placement option */
static void sctk_read_format_option_text_placement(FILE *f_textual, struct sctk_text_option_s *tab_option, int lenght){
    /*! \brief Destroy the topology module
    */

    int cpt_ligne = 0;
    while(1){// one ligne per iteration
        //malloc buffer for infos of other ranks in the same processus
        char * os_indbuff = (char *)sctk_malloc(64*lenght);
        char * infosbuff = (char *)sctk_malloc(64*lenght);
        char * infos_rank_mpibuff= (char *)sctk_malloc(64*lenght);
        char * infos_compactbuff = (char *)sctk_malloc(64*lenght);
        char * infos_scatterbuff = (char *)sctk_malloc(64*lenght);
        char * infos_balancedbuff = (char *)sctk_malloc(64*lenght);
        char * infos_pidbuff = (char *)sctk_malloc(64*lenght);
        /* read os ind infos */
        int c = getc(f_textual);
        int cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(os_indbuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(os_indbuff, cpt, tab_option->os_index, cpt_ligne);

        /* read vp infos */
        c = getc(f_textual);
        cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(infosbuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(infosbuff, cpt, tab_option->vp_tab, cpt_ligne);

        /* read mpi rank infos */
        c = getc(f_textual);
        cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(infos_rank_mpibuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(infos_rank_mpibuff, cpt, tab_option->rank_mpi, cpt_ligne);

        /* read COMPACT policy*/
        c = getc(f_textual);
        cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(infos_compactbuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(infos_compactbuff, cpt, tab_option->compact_tab, cpt_ligne);

        /* read SCATTER policy*/
        c = getc(f_textual);
        cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(infos_scatterbuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(infos_scatterbuff, cpt, tab_option->scatter_tab, cpt_ligne);

        /* read BALANCED policy*/
        c = getc(f_textual);
        cpt = 0;
        while((c != ' ') && (c != EOF)){
            read_char(infos_balancedbuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(infos_balancedbuff, cpt, tab_option->balanced_tab, cpt_ligne);

        /* read pid */
        c = getc(f_textual);
        cpt = 0;
        while((c != '\n') && (c != EOF)){ /*end ligne*/
            read_char(infos_pidbuff, &cpt, &c, f_textual);
        }
        if(c == EOF){
            break;
        }
        convert_char(infos_pidbuff, cpt, tab_option->pid_tab, cpt_ligne);

        /* free buff */
        sctk_free(os_indbuff );
        sctk_free(infos_rank_mpibuff);
        sctk_free(infosbuff );
        sctk_free(infos_balancedbuff);
        sctk_free(infos_scatterbuff);
        sctk_free(infos_compactbuff);
        sctk_free(infos_pidbuff);
        cpt_ligne++;
    }
    fclose(f_textual);
}


/* Write in file as lstopo adding informations on the topology and thread placement (text option) */
static void print_children(hwloc_topology_t hwtopology, hwloc_obj_t obj,
        int depth, struct sctk_text_option_s *tab_option, int num_os, unsigned int higher_logical , int lower_logical, const char* HostName, FILE* f, int __UNUSED__ ind_child, int last_arity)
{
    char string[128];
    char string_mpc[128];
    unsigned i;

    int k;
    static hwloc_obj_type_t type;
    type = HWLOC_OBJ_PU;

    hwloc_obj_type_snprintf(string, sizeof(string), obj, 0);
    static int already_begining_done = 1;
    if(already_begining_done){
        hwloc_obj_t lower_index_obj_pu = hwloc_get_obj_by_type(hwtopology, HWLOC_OBJ_PU, lower_logical);
        /* if ancestor of the first pu booked */
#if (HWLOC_API_VERSION < 0x00020000)
        if(hwloc_get_ancestor_obj_by_type(hwtopology, obj->type, lower_index_obj_pu)->logical_index == obj->logical_index
        && obj->type != HWLOC_OBJ_MACHINE && obj->type != HWLOC_OBJ_NODE && obj->type != HWLOC_OBJ_SOCKET && obj->type != HWLOC_OBJ_SYSTEM)
#else
        if(hwloc_get_ancestor_obj_by_type(hwtopology, obj->type, lower_index_obj_pu)->logical_index == obj->logical_index
        && obj->type != HWLOC_OBJ_MACHINE && obj->type != HWLOC_OBJ_PACKAGE)
#endif
        {
            fprintf(f,"\n\n|------------------------------BEGINNING RESERVATION HOST %s-------------------------|\n", HostName);
            already_begining_done = 0;
        }
    }
/* Check if os ind for pus and cores are always the same for the first pu of a core */
    if(obj->type == type){
        for(k = 0; k < num_os; k++){
            hwloc_obj_t pu;
            int os_index_to_compare;
            if(mpc_common_get_flags()->enable_smt_capabilities){
                os_index_to_compare = obj->os_index;
            }
            else{
                pu = hwloc_get_ancestor_obj_by_type(hwtopology, HWLOC_OBJ_CORE, obj);
                os_index_to_compare = pu->os_index;
            }
            if(tab_option->os_index[k] == os_index_to_compare){
                sprintf(string_mpc, "Virtual Processor %d | rank MPI %d | Policy : compact %d, scatter %d, balanced %d | tid %d", tab_option->vp_tab[k], tab_option->rank_mpi[k], tab_option->compact_tab[k], tab_option->scatter_tab[k], tab_option->balanced_tab[k],  tab_option->pid_tab[k]);
                if(last_arity == 1){
                    fprintf(f," + %s#%d %s", string,obj->logical_index, string_mpc);
                }
                else{
                    fprintf(f,"\n%*s%s#%d %s", 2*depth, "", string,obj->logical_index, string_mpc);
                }
                if(obj->logical_index == higher_logical){
                    fprintf(f,"\n\n|------------------------------END RESERVATION HOST %s------------------------------|\n", HostName);
                }
                goto arity; /* pu has been find */
            }
        }
    }
    if(last_arity == 1){
        fprintf(f," + %s#%d", string,obj->logical_index);
    }
    else{
        fprintf(f,"\n%*s%s#%d", 2*depth, "", string,obj->logical_index);
    }
    if(obj->type == type){
        if(obj->logical_index == higher_logical){
            fprintf(f,"\n\n|------------------------------END RESERVATION HOST %s------------------------------|\n", HostName);
        }
    }

    arity: /* update arity for the next object */

    if(obj->arity == 1){
        last_arity = 1;
    }
    else{
        last_arity = 0;
   }
    for (i = 0; i < obj->arity; i++) {
        print_children(hwtopology, obj->children[i], depth + 1, tab_option,  num_os, higher_logical, lower_logical, HostName, f, i, last_arity);
    }
}

static void sctk_destroy_text_option(struct sctk_text_option_s *tab_option){
                    sctk_free(tab_option->os_index);
                    sctk_free(tab_option->vp_tab);
                    sctk_free(tab_option->rank_mpi);
                    sctk_free(tab_option->compact_tab);
                    sctk_free(tab_option->scatter_tab);
                    sctk_free(tab_option->balanced_tab);
                    sctk_free(tab_option->pid_tab);
                    sctk_free(tab_option);
}



static hwloc_obj_t hwloc_get_core_by_os_index(hwloc_topology_t hwtopology, unsigned int os_i){
    int depth_core = hwloc_get_type_depth(hwtopology, HWLOC_OBJ_CORE);
    int i;
    int nb_core = hwloc_get_nbobjs_by_depth(hwtopology, depth_core);
    for(i = 0; i <  nb_core; i++){
        hwloc_obj_t core = hwloc_get_obj_by_depth(hwtopology, HWLOC_OBJ_CORE, i);
        if(core->os_index == os_i){
            return core;
        }
    }
    return NULL;
}




/* determine the lower logical index pu used for text placement option */
static int determine_lower_logical(int *os_index, int lenght){
    int i;
    int lower_logical = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);
    int current_logical = 0;
    hwloc_obj_t pu;
    for(i=0;i<lenght;i++){
        if(os_index[i] != -1){
                if(mpc_common_get_flags()->enable_smt_capabilities){
                    pu = hwloc_get_pu_obj_by_os_index(topology_compute_node, os_index[i]);
                }
                else{
                    hwloc_obj_t core = hwloc_get_core_by_os_index(topology_compute_node, os_index[i]);
                    pu = core->children[0];
                }
                if(pu == NULL){
                    return lower_logical;
                }
                current_logical = pu->logical_index;
        }
        if( current_logical < lower_logical){
            lower_logical = current_logical;
        }
    }
    return lower_logical;
}

/* determine the higher logical index pu used for text placement option */
static int sctk_determine_higher_logical(int *os_index, int lenght){
    int i;
    int higher_logical = -1;
    int current_logical = 0;
    hwloc_obj_t pu;
    for(i=0;i<lenght;i++){
        if(os_index[i] != -1){
                if(mpc_common_get_flags()->enable_smt_capabilities){
                    pu = hwloc_get_pu_obj_by_os_index(topology_compute_node, os_index[i]);
                }
                else{
                    hwloc_obj_t core = hwloc_get_core_by_os_index(topology_compute_node, os_index[i]);
                    pu = core->children[0];
                }
                if(pu == NULL){
                    return higher_logical;
                }
                current_logical = pu->logical_index;
        }
        if( current_logical > higher_logical){
            higher_logical = current_logical;
        }
    }
    return higher_logical;
}




/* used by graphic and text option */
    static void
sctk_restrict_topology_compute_node ()
{
    fflush(stdout);

restart_restrict:

    if (mpc_common_get_flags()->enable_smt_capabilities)
    {
        int i;
        mpc_common_debug_warning ("SMT capabilities ENABLED");

        int proc_count = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);
        hwloc_bitmap_zero(topology_cpuset_compute_node);

        for(i=0;i < proc_count; ++i)
        {
            hwloc_obj_t core = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_PU, i);
            hwloc_bitmap_or(topology_cpuset_compute_node, topology_cpuset_compute_node, core->cpuset);
        }

    }
    else
    {
        /* disable SMT capabilities */
        unsigned int i;
        int err;
        hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
        hwloc_cpuset_t set = hwloc_bitmap_alloc();
        hwloc_bitmap_zero(cpuset);
        const unsigned int core_number = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_CORE);

        for(i=0;i < core_number; ++i)
        {
            hwloc_obj_t core = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_CORE, i);
            hwloc_bitmap_copy(set, core->cpuset);
            hwloc_bitmap_singlify(set);
            hwloc_bitmap_or(cpuset, cpuset, set);
        }
        /* restrict the topology to physical CPUs */
#if (HWLOC_API_VERSION < 0x00020000)
        err = hwloc_topology_restrict(topology_compute_node, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
#else
        err = hwloc_topology_restrict(topology_compute_node, cpuset, 0);
#endif
        if(err)
        {
            hwloc_bitmap_free(cpuset);
            hwloc_bitmap_free(set);
            mpc_common_get_flags()->enable_smt_capabilities = 1;
            mpc_common_debug_warning ("Topology reduction issue");
            goto restart_restrict;
        }
    }

}

void _mpc_topology_render_init(void)
{
    /*graphical option*/
    if(mpc_common_get_flags()->enable_topology_graphic_placement || mpc_common_get_flags()->enable_topology_text_placement){


	    hwloc_topology_init( &topology_compute_node );
        char *xml_path = getenv( "MPC_SET_XML_TOPOLOGY_FILE" );

        if ( xml_path != NULL )
        {
            hwloc_topology_set_xml( topology_compute_node, xml_path );
	    }

        hwloc_topology_load( topology_compute_node );

        hwloc_cpuset_t newset;
        newset = hwloc_bitmap_alloc();
        int ret = hwloc_get_last_cpu_location(mpc_topology_get(), newset, HWLOC_CPUBIND_THREAD);
        assert(ret == 0);
        hwloc_obj_t obj;
        obj = hwloc_get_obj_inside_cpuset_by_type(mpc_topology_get(), newset,HWLOC_OBJ_PU, 0);
        hwloc_obj_t cluster = hwloc_get_ancestor_obj_by_type(topology_compute_node, HWLOC_OBJ_MACHINE, obj);
        strcpy(file_placement, "");
        strcat(file_placement, "placement_");
        strcpy(textual_file, "");
        strcat(textual_file, "textual_");
        strcpy(textual_file_output, "");
        strcat(textual_file_output, "textual_");
        strcpy(placement_txt, "");
        strcat(placement_txt, "placement_");
        if(cluster != NULL){
            const char * HostName  = hwloc_obj_get_info_by_name(cluster, "HostName");
            //sprintf(proc_id, "%d", syscall(SYS_gettid));
            strcat(file_placement, HostName);
            strcat(textual_file, HostName);
            strcat(textual_file_output, HostName);
            strcat(placement_txt, HostName);
            remove(placement_txt);
            remove(textual_file);
        }
        topology_cpuset_compute_node = hwloc_bitmap_alloc();
        /* topology usesd by the graphic option */
        sctk_restrict_topology_compute_node ();
    }
    /*end graphical*/
}



void topology_graph_enable_graphic_placement( void )
{

	hwloc_obj_t cluster = hwloc_get_obj_by_type( mpc_topology_get(), HWLOC_OBJ_MACHINE, 0 );
	int lenght_max;

	lenght_max = hwloc_get_nbobjs_by_type( mpc_topology_get(), HWLOC_OBJ_PU );

	if ( cluster != NULL )
	{
		FILE *f = fopen( placement_txt, "r" );
		if ( f != NULL )
		{
			sctk_read_format_option_graphic_placement_and_complet_topo_infos( f, lenght_max );
		}

		name_and_date_file_text( file_placement );
		strcat( file_placement, ".xml" );
#if (HWLOC_API_VERSION < 0x00020000)
		hwloc_topology_export_xml( mpc_topology_get(), file_placement );
#else
    hwloc_topology_export_xml( mpc_topology_get(), file_placement, HWLOC_TOPOLOGY_EXPORT_XML_FLAG_V1);
#endif
		if ( 1 )
		{ //TODO one among each nodes
			fprintf( stdout, "/* --graphic-placement : \n.xml dated file has been generated for each compute node to vizualise topology and thread placement with their infos.\nYou can use the command \"lstopo -i file.xml\" to vizualise graphicaly */\n" );
			fprintf( stdout, "\n/* .xml legend : one color per MPI task. White text policy means the thread is master of a MPI task in MPC */\n\n" );
			fflush( stdout );
			remove( placement_txt );
		}
	}
}


void topology_graph_enable_text_placement( void )
{
	hwloc_obj_t cluster = hwloc_get_obj_by_type( mpc_topology_get(), HWLOC_OBJ_MACHINE, 0 );
	int lenght_max;
	int lenght_min;
	int lenght;
	lenght_max = hwloc_get_nbobjs_by_type( mpc_topology_get(), HWLOC_OBJ_PU );
	lenght_min = hwloc_get_nbobjs_by_type( mpc_topology_get(), HWLOC_OBJ_CORE );
	if ( mpc_common_get_flags()->enable_smt_capabilities )
	{
		lenght = lenght_max;
	}
	else
	{
		lenght = lenght_min;
	}
	/* read textual informations */
	FILE *f_textual = fopen( textual_file, "r" );
	if ( f_textual != NULL )
	{
		struct sctk_text_option_s *tab_option;

		sctk_init_text_option( &tab_option );

		sctk_read_format_option_text_placement( f_textual, tab_option, lenght_max );

		const char *HostName = hwloc_obj_get_info_by_name( cluster, "HostName" );

		name_and_date_file_text( textual_file_output );
		strcat( textual_file_output, ".txt" );

		hwloc_obj_t root = hwloc_get_root_obj( mpc_topology_get() );
		FILE *f = fopen( textual_file_output, "a" );

		if ( f != NULL )
		{
			fprintf( f, "|------------------------------HOST                 %s------------------------------|\n", HostName );
			int higher_logical = sctk_determine_higher_logical( tab_option->os_index, lenght );
			int lower_logical = determine_lower_logical( tab_option->os_index, lenght );
			if ( 1 )
			{ //TODO one proc amongs nodes
				fprintf( stdout, "/* --text-placement : \n.txt dated file has been generated for each compute node to vizualise topology and thread placement with their infos. */\n" );
				fflush( stdout );
			}
			print_children( mpc_topology_get(), root, 0, tab_option, lenght_max, higher_logical, lower_logical, HostName, f, 0, 0 );
			fclose( f );
		}
		/* remove temp files */
		sctk_destroy_text_option( tab_option );
		remove( textual_file );
	}
}



void _mpc_topology_render_render(void)
{
    if( mpc_common_get_local_process_rank() == 0){
        if(mpc_common_get_flags()->enable_topology_graphic_placement){
            topology_graph_enable_graphic_placement();
        }
        if(mpc_common_get_flags()->enable_topology_text_placement){
            topology_graph_enable_text_placement();
        }
    }
}
