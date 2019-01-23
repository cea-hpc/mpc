/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* # Copyright or (C) or Copr. 2010-2012 Université de Versailles         # */
/* # St-Quentin-en-Yvelines                                               # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - DIDELOT Sylvain sylvain.didelot@exascale-computing.eu            # */
/* #   - TCHIBOUKDJIAN Marc marc.tchiboukdjian@exascale-computing.eu      # */
/* #   - BOUHROUR Stephane stephane.bouhrour@exascale-computing.eu        # */
/* #                                                                      # */
/* ######################################################################## */
#include "sctk_config.h"
#include "sctk_topology.h"
#include "sctk_launch.h"
#include "sctk_config.h"
#include "sctk_debug.h"
#include "sctk_thread.h"
#include "sctk.h"
#include "sctk_kernel_thread.h"
#include "sctk_device_topology.h"

#ifdef MPC_Message_Passing
#include "sctk_pmi.h"
#endif

#include <dirent.h>

#include <stdio.h>
#ifdef HAVE_UTSNAME
#include <sys/utsname.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#define SCTK_MAX_PROCESSOR_NAME 100
#define SCTK_MAX_NODE_NAME 512
#define SCTK_LOCAL_VERSION_MAJOR 0
#define SCTK_LOCAL_VERSION_MINOR 1

static char* sctk_xml_specific_path = NULL;
static int sctk_processor_number_on_node = 0;
static char sctk_node_name[SCTK_MAX_NODE_NAME];
static sctk_spinlock_t topology_lock = SCTK_SPINLOCK_INITIALIZER;
hwloc_bitmap_t pin_processor_bitmap;
#define MAX_PIN_PROCESSOR_LIST 1024
int pin_processor_list[MAX_PIN_PROCESSOR_LIST];
int pin_processor_current = 0;
int bind_processor_current = 0;

/* The topology of the machine + its cpuset */
static hwloc_topology_t topology;
static hwloc_bitmap_t topology_cpuset;
/* used by the graphical option */
static hwloc_bitmap_t topology_cpuset_compute_node;
/* Describe the full topology of the machine.
 * Only used for binding*/
static hwloc_topology_t topology_full;
/* used by the graphical option */
static hwloc_topology_t topology_compute_node;
const struct hwloc_topology_support *support;


/* used by graphic option */
static void compute_master_color(int r, int g, int b,  int *r_m, int *g_m, int *b_m){
    if(r > g && r > b){
        *r_m = r + 25;
        *g_m = g + 25;
        *b_m = b + 25;
    }
    if(g > r && g > b){
        *r_m = r + 25;
        *g_m = g + 25;
        *b_m = b + 25;
    }
    if(b > g && b > r){
        *r_m = r + 25;
        *g_m = g + 25;
        *b_m = b + 25;
    }
}

/* used by graphic option */
static void chose_color_task(int task_id, int nb_task, int *r, int *g, int *b, int *r_m, int *g_m, int *b_m){
    int red, green, blue;
    int red_m, green_m, blue_m;
    int init_x;
    int step = (230) / nb_task;
    switch((task_id + 6) % 6){
        case 0:
            init_x = 0;
            red = 230;
            green = 0;
            blue = 0;
            if(init_x == 0){
                green = 0 + step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }
            else{
                green = 230 - step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }

            break;

        case 1:
            init_x = 230;
            red = 0;
            green = 230;
            blue = 0;
            if(init_x == 0){
                red = 0 + step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }
            else{
                red = 230 - step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }

            break;

        case 2:
            init_x = 0;
            red = 0;
            green = 230;
            blue = 0;
            if(init_x == 0){
                blue = 0 + step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }
            else{
                blue = 230 - step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }

            break;
        case 3:
            init_x = 230;
            red = 0;
            green = 230;
            blue = 230;
            if(init_x == 0){
                green= 0 + step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }
            else{
                green = 230 - step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }

            break;
        case 4:
            init_x = 0;
            red = 0;
            green = 0;
            blue = 230;
            if(init_x == 0){
                red = 0 + step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }
            else{
                red = 230 - step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }

            break;
        case 5:
            init_x = 230;
            red = 230;
            green = 0;
            blue = 230;
            if(init_x == 0){
                blue = 0 + step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }
            else{
                blue = 230 - step * task_id;
                compute_master_color(red, green, blue, &red_m, &green_m, &blue_m);
            }

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
void create_placement_text(int os_pu, int os_master_pu, int task_id, int vp, __UNUSED__ int rank_open_mp, int* min_idex, int pid){

    /*acces global topo*/
    hwloc_topology_load(topology_full);
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
void create_placement_rendering(int os_pu, int os_master_pu, int task_id){
    int red, green, blue;
    int red_m, green_m, blue_m;
    char string_rgb_hexa[512];
    char string_rgb_hexa_master[512];


    chose_color_task(task_id, sctk_get_task_number(), &red, &green, &blue, &red_m, &green_m, &blue_m);

    convert_rgb_to_string(red, green, blue, string_rgb_hexa);
    convert_rgb_to_string(red, green, blue, string_rgb_hexa_master);

    /*acces global topo*/
    hwloc_topology_load(topology_full);
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

/* return the os index of the topology_compute_node of the thread executed */
int sctk_get_cpu_compute_node_topology()
{
    hwloc_cpuset_t set = hwloc_bitmap_alloc();

    int ret = hwloc_get_last_cpu_location(topology_compute_node, set, HWLOC_CPUBIND_THREAD);

    assume(ret!=-1);
    assume(!hwloc_bitmap_iszero(set));



    hwloc_obj_t pu;
    if(sctk_enable_smt_capabilities){
        pu = hwloc_get_obj_inside_cpuset_by_type(topology_compute_node, set, HWLOC_OBJ_PU, 0);
    }
    else{
        pu = hwloc_get_obj_inside_cpuset_by_type(topology_compute_node, set, HWLOC_OBJ_CORE, 0);
    }

    if (!pu)
    {
        hwloc_bitmap_free(set);
        return -1;
    }

    /* return the os index */
    int cpu_os = pu->os_index;
    //int cpu = sctk_get_logical_from_os_compute_node_topology(cpu_os);

    return cpu_os;
}

/* return logical index from topology_compute_node os index */
int sctk_get_logical_from_os_compute_node_topology(unsigned int cpu_os){
    /* hwloc_get_pu_obj_by_os_inde give false resultat i suppose */
    /*hwloc_obj_t cpu = hwloc_get_pu_obj_by_os_index(topology_compute_node, cpu_os);
    return cpu->logical_index;*/
    unsigned int nb;
    unsigned int i;
    if(sctk_enable_smt_capabilities){
        nb = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);

        for(i = 0; i <  nb; i++){
            if(hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_PU, i)->os_index == cpu_os){
                hwloc_obj_t pu = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_PU, i);
                return pu->logical_index;
            }
        }
    }
    else{
        nb =hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_CORE);

        for(i = 0; i <  nb; i++){
            if(hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_CORE, i)->os_index == cpu_os){
                hwloc_obj_t pu = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_CORE, i);
                return pu->logical_index;
            }
        }
    }
    return -1;
}


/* return the os index of the topology_compute_node from logical index */
int sctk_get_cpu_compute_node_topology_from_logical( int logical_pu)
{
    hwloc_obj_t pu;
    if(sctk_enable_smt_capabilities){
        pu = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_PU,logical_pu);
    }
    else{
        pu = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_CORE,logical_pu);
    }

    if (pu)
    {
        return pu->os_index;
    }
    else{
        return - 1;
    }
}

/* used by graphic and text option */
    static void
sctk_restrict_topology_compute_node ()
{
    fflush(stdout);

restart_restrict:

    if (sctk_enable_smt_capabilities)
    {
        int i;
        sctk_warning ("SMT capabilities ENABLED");

        sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);
        hwloc_bitmap_zero(topology_cpuset_compute_node);

        for(i=0;i < sctk_processor_number_on_node; ++i)
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
        err = hwloc_topology_restrict(topology_compute_node, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
        if(err)
        {
            hwloc_bitmap_free(cpuset);
            hwloc_bitmap_free(set);
            sctk_enable_smt_capabilities = 1;
            sctk_warning ("Topology reduction issue");
            goto restart_restrict;
        }
    }

}

/* used by graphic and text option */
static void read_char(char * buff, int *cpt, char *c, FILE *f){
    buff[*cpt] = *c;
    (*cpt)++;
    *c = getc(f);
}

/* used by graphic and text option */
static void sctk_read_format_option_graphic_placement_and_complet_topo_infos(FILE *f, int lenght){
    while(1){
        //malloc buffer for infos of other ranks in the same processus
        char * os_indbuff = (char *)malloc(64*lenght);
        char * infosbuff = (char *)malloc(64*lenght);
        char c = getc(f);
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
            sctk_get_logical_from_os_compute_node_topology(atoi(os_ind));
        hwloc_obj_t obj;
        if(sctk_enable_smt_capabilities){
            obj = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_PU, logical_ind);
        }
        else{
            obj = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_CORE, logical_ind);
        }
        if(!obj){
            return;
        }
        hwloc_obj_add_info(obj, "lstopoStyle", infos);
        free(infosbuff);
        free(os_indbuff );
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
    int lenght = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);
    *tab_option = (struct sctk_text_option_s *)malloc(sizeof(struct sctk_text_option_s)); 
    (*tab_option)->os_index = (int *)malloc(sizeof(int)*lenght);
    (*tab_option)->vp_tab = malloc(sizeof(int)*lenght);
    (*tab_option)->rank_mpi= malloc(sizeof(int)*lenght);
    (*tab_option)->compact_tab = malloc(sizeof(int)*lenght);
    (*tab_option)->scatter_tab = malloc(sizeof(int)*lenght);
    (*tab_option)->balanced_tab = malloc(sizeof(int)*lenght);
    (*tab_option)->pid_tab = malloc(sizeof(int)*lenght);
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
        char * os_indbuff = (char *)malloc(64*lenght);
        char * infosbuff = (char *)malloc(64*lenght);
        char * infos_rank_mpibuff= (char *)malloc(64*lenght);
        char * infos_compactbuff = (char *)malloc(64*lenght);
        char * infos_scatterbuff = (char *)malloc(64*lenght);
        char * infos_balancedbuff = (char *)malloc(64*lenght);
        char * infos_pidbuff = (char *)malloc(64*lenght);
        /* read os ind infos */
        char c = getc(f_textual);
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
        free(os_indbuff );
        free(infos_rank_mpibuff);
        free(infosbuff );
        free(infos_balancedbuff);
        free(infos_scatterbuff);
        free(infos_compactbuff);
        free(infos_pidbuff);
        cpt_ligne++;
    }
    fclose(f_textual);
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
                if(sctk_enable_smt_capabilities){
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
                if(sctk_enable_smt_capabilities){
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
        if(hwloc_get_ancestor_obj_by_type(hwtopology, obj->type, lower_index_obj_pu)->logical_index == obj->logical_index
        && obj->type != HWLOC_OBJ_MACHINE && obj->type != HWLOC_OBJ_NODE && obj->type != HWLOC_OBJ_SOCKET && obj->type != HWLOC_OBJ_SYSTEM){
            fprintf(f,"\n\n|------------------------------BEGINING RESERVATION HOST %s-------------------------|\n", HostName); 
            already_begining_done = 0;
        }
    }
/* Check if os ind for pus and cores are always the same for the first pu of a core */
    if(obj->type == type){
        for(k = 0; k < num_os; k++){
            hwloc_obj_t pu;
            int os_index_to_compare;
            if(sctk_enable_smt_capabilities){
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
                    free(tab_option->os_index);
                    free(tab_option->vp_tab);
                    free(tab_option->rank_mpi);
                    free(tab_option->compact_tab);
                    free(tab_option->scatter_tab);
                    free(tab_option->balanced_tab);
                    free(tab_option->pid_tab);
                    free(tab_option);
}

hwloc_topology_t sctk_get_topology_object(void)
{
	return topology ;
}

/* print a cpuset in a human readable way */
void print_cpuset(hwloc_bitmap_t cpuset)
{
	char buff[256];
	hwloc_bitmap_list_snprintf(buff, 256, cpuset);
	fprintf(stdout, "cpuset: %s\n", buff);
	fflush(stdout);
}

static void
sctk_update_topology ( const int processor_number,  const unsigned int index_first_processor )
{
	sctk_processor_number_on_node = processor_number ;
	hwloc_bitmap_t cpuset = hwloc_bitmap_alloc();
	unsigned int i;
	int err;

	hwloc_bitmap_zero(cpuset);
	if (hwloc_bitmap_iszero(pin_processor_bitmap))
	{
		for( i=index_first_processor; i < index_first_processor+processor_number; ++i)
		{
			#ifdef __MIC__
			hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i%processor_number);
			#else
			hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
			#endif
			hwloc_cpuset_t set = hwloc_bitmap_dup(pu->cpuset);
			hwloc_bitmap_singlify(set);
			hwloc_bitmap_or(cpuset, cpuset, set);
			hwloc_bitmap_free(set);
		}

	}
	else
	{
		int sum = 0;
		
		sum = hwloc_get_nbobjs_inside_cpuset_by_type(topology, pin_processor_bitmap, HWLOC_OBJ_PU);
		
		if (sum != sctk_get_processor_nb ())
		{
			sctk_error("MPC_PIN_PROCESSOR_LIST is set with a different number of processor available on the node: %d in the list, %d on the node", sum, sctk_get_processor_nb());
			sctk_abort();
		}

		hwloc_bitmap_copy(cpuset, pin_processor_bitmap);
	}

	err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
	assume(!err);
	hwloc_bitmap_copy(topology_cpuset, cpuset);
	hwloc_bitmap_free(cpuset);
}

static void sctk_expand_pin_processor_add_to_list(int id)
{
	hwloc_cpuset_t previous = hwloc_bitmap_alloc();
	hwloc_bitmap_copy(previous, pin_processor_bitmap);
	
	if ( id > (sctk_processor_number_on_node - 1) )
		sctk_error("Error in MPC_PIN_PROCESSOR_LIST: processor %d is more than the max number of cores %d", id, sctk_processor_number_on_node);
	
	hwloc_bitmap_set(pin_processor_bitmap, id);

	/* New entry added. We also add it in the list */
	if ( !hwloc_bitmap_isequal(pin_processor_bitmap, previous))
	{
		assume(pin_processor_current < MAX_PIN_PROCESSOR_LIST);
		pin_processor_list[pin_processor_current] = id;
		pin_processor_current++;
	}
	
	hwloc_bitmap_free(previous);
}

static void sctk_expand_pin_processor_list(char *env)
{
	char *c = env;
	char prev_number[5];
	char *prev_ptr=prev_number;
	int id;

	while(*c != '\0') {
		if (isdigit(*c)) {
			*prev_ptr=*c;
			prev_ptr++;
		} else if (*c == ',') {
			*prev_ptr='\0';
			id = atoi(prev_number);
			sctk_expand_pin_processor_add_to_list(id);
			prev_ptr = prev_number;
		} else if (*c == '-') {
			*prev_ptr='\0';
			int start_num = atoi(prev_number);
			prev_ptr = prev_number;
			++c;
			while( isdigit(*c)) {
				*prev_ptr=*c;
				prev_ptr++;
				++c;
			}
			--c;
			*prev_ptr='\0';
			int end_num = atoi(prev_number);
			int i;
			for (i=start_num; i <= end_num; ++i) {
				sctk_expand_pin_processor_add_to_list(i);
			}
			prev_ptr = prev_number;
		}else{
			sctk_error("Error in MPC_PIN_PROCESSOR_LIST: wrong character read: %c", *c);
			sctk_abort();
		}
		++c;
	}

	/*Terminates the string */
	if (prev_ptr != prev_number) {
		*prev_ptr='\0';
		id = atoi(prev_number);
		sctk_expand_pin_processor_add_to_list(id);
	}
}

  static void
sctk_restrict_topology ()
{
	int rank ;
	sctk_only_once();

	restart_restrict:
	
	if (sctk_enable_smt_capabilities)
	{
		int i;
		sctk_warning ("SMT capabilities ENABLED");

		sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
		hwloc_bitmap_zero(topology_cpuset);

		for(i=0;i < sctk_processor_number_on_node; ++i)
		{
			hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
			hwloc_bitmap_or(topology_cpuset, topology_cpuset, core->cpuset);
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
		const unsigned int core_number = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);

		for(i=0;i < core_number; ++i)
		{
			hwloc_obj_t core = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, i);
			hwloc_bitmap_copy(set, core->cpuset);
			hwloc_bitmap_singlify(set);
			hwloc_bitmap_or(cpuset, cpuset, set);
		}
		/* restrict the topology to physical CPUs */
		err = hwloc_topology_restrict(topology, cpuset, HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES);
		if(err)
		{
			hwloc_bitmap_free(cpuset);
			hwloc_bitmap_free(set);
			sctk_enable_smt_capabilities = 1;
			sctk_warning ("Topology reduction issue");
			goto restart_restrict;
		}
		hwloc_bitmap_copy(topology_cpuset, cpuset);

		hwloc_bitmap_free(cpuset);
		hwloc_bitmap_free(set);
		sctk_processor_number_on_node = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
	}

	pin_processor_bitmap = hwloc_bitmap_alloc();
	hwloc_bitmap_zero(pin_processor_bitmap);

	#ifdef __MIC__
	{
		sctk_update_topology (sctk_processor_number_on_node, sctk_get_pu_number_by_core(topology, 0)) ;
	}
	#endif

	char* pinning_env = getenv("MPC_PIN_PROCESSOR_LIST");
	
	if (pinning_env != NULL )
	{
		sctk_expand_pin_processor_list(pinning_env);
	}

	/* Share nodes */
	sctk_share_node_capabilities = 1;
	
	if ((sctk_share_node_capabilities == 1) 
	&& (sctk_process_number > 1))
	{
		
		int detected_on_this_host = 0;
		int detected = 0;

		#ifdef MPC_Message_Passing
		/* Determine number of processes on this node */
		sctk_pmi_get_process_on_node_number(&detected_on_this_host);
		sctk_pmi_get_process_on_node_rank(&rank);
		detected = sctk_process_number;
		sctk_nodebug("Nb process on node %d",detected_on_this_host);
		#else
		detected = 1;
		#endif

		/* More than 1 process per node is not supported by process pinning */
		int nodes_number = sctk_get_node_nb();
		
		if ( (sctk_process_number != nodes_number) && !hwloc_bitmap_iszero(pin_processor_bitmap))
		{
			sctk_error("MPC_PIN_PROCESSOR_LIST cannot be set if more than 1 process per node is set. process_number=%d node_number=%d", sctk_process_number, nodes_number);
			sctk_abort();
		}

		while (detected != sctk_get_process_nb ());
		
		sctk_nodebug ("%d/%d host detected %d share %s", detected,
		sctk_get_process_nb (), detected_on_this_host,
		sctk_node_name);

		{
			/* Determine processor number per process */
			int processor_number = sctk_processor_number_on_node / detected_on_this_host;
			int remaining_procs = sctk_processor_number_on_node % detected_on_this_host;
			int start = processor_number * rank;
			
			if(processor_number < 1)
			{
				processor_number = 1;
				remaining_procs = 0;
				start = (processor_number * rank) % sctk_processor_number_on_node;
			}

			if ( remaining_procs > 0 )
			{
				if ( rank < remaining_procs )
				{
					processor_number++;
					start += rank;
				}
				else
				{
					start += remaining_procs;
				}
			}

			sctk_update_topology ( processor_number, start ) ;
		}

	} /* share node */

	/* Choose the number of processor used */
	{
		int processor_number;

		processor_number = sctk_get_processor_nb ();

		if (processor_number > 0)
		{
			if (processor_number > sctk_processor_number_on_node)
			{
				sctk_error ("Unable to allocate %d core (%d real cores)",
				processor_number, sctk_processor_number_on_node);
				/* we cannot exit with a sctk_abort() because thread
				* library is not initialized at this moment */
				/* sctk_abort (); */
				exit(1);
			}
			else if ( processor_number < sctk_processor_number_on_node )
			{
				sctk_warning ("Do not use the entire node %d on %d",
				processor_number, sctk_processor_number_on_node);
				sctk_update_topology ( processor_number, 0 ) ;
			}
		}
	}
}

/* Called only by __mpcomp_buid_tree */
int sctk_get_global_index_from_cpu(hwloc_topology_t topo, const int vp) {
  hwloc_obj_t obj;
  hwloc_topology_t globalTopology = sctk_get_topology_object();
  const hwloc_obj_t pu = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, vp);

  assume(pu);
  obj = hwloc_get_pu_obj_by_os_index(globalTopology, pu->os_index);
  return obj->logical_index;
}

#if 0 /* MOVE TO OPENMP MPCOMP_TREE_BIS.C */
/*
 * Restrict the topology object of the current mpi task to 'nb_mvps' vps.
 * This function handles multiple PUs per core.
 */
int sctk_restrict_topology_for_mpcomp(hwloc_topology_t *restrictedTopology,
                                      int nb_mvps) {
  hwloc_topology_t topology;
  hwloc_cpuset_t cpuset;
  hwloc_obj_t obj;
  int taskRank, taskVp, err, i, nbvps;
  int nb_cores, nb_pus, nb_mvps_per_core;
  int mvp, core;

  /* Check input parameters */
  sctk_assert(nb_mvps > 0);
  sctk_assert(restrictedTopology);

  /* Get the current MPI task rank */
  taskRank = sctk_get_task_rank();

  /* Initialize the final cpuset */
  cpuset = hwloc_bitmap_alloc();

  /* Get the current global topology (already restricted by MPC) */
  topology = sctk_get_topology_object();

  /* Grab the current VP and the number of VPs for the current task */
  taskVp = sctk_get_init_vp_and_nbvp(taskRank, &nbvps);
  /* We do not support more mVPs than VPs */
  sctk_assert(nb_mvps <= nbvps);

  /* BEGIN DEBUG */
  if (sctk_get_verbosity() >= 3) {
    fprintf(stderr, "[[%d]] __mpcomp_restrict_topology: GENERAL TOPOLOGY "
                    "(rank= %d, vp=%d, logical range=[%d,%d]) \n",
            taskRank, taskRank, taskVp, taskVp, taskVp + nbvps - 1);
    sctk_print_specific_topology(stderr, topology);
  }
  /* END DEBUG */

  /* Get information on the current topology */
  nb_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
  nb_pus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  nb_mvps_per_core = nb_mvps / nb_cores;

  if (sctk_get_verbosity() >= 3) {
    fprintf(stderr, "[[%d]] __mpcomp_restrict_topology: General Topo: nb_cores "
                    "= %d, nb_pus = %d\n",
            taskRank, nb_cores, nb_pus);
  }

  mvp = 0;
  /* Iterate through all cores */
  for (core = 0; core < nb_cores && mvp < nb_mvps; core++) {
    int local_slot_size;
    hwloc_cpuset_t core_cpuset;

    /* Compute the number of PUs to handle for this core */
    local_slot_size = nb_mvps_per_core;
    if ((nb_mvps % nb_cores) > core) {
      local_slot_size++;
    }

    /* Get the core cpuset */
    core_cpuset =
        (hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, core))->cpuset;

    /* Add the PUs we want from the core cpuset to our target cpuset */
    for (i = 0; i < local_slot_size; i++, mvp++) {
      hwloc_obj_t obj;
      obj = hwloc_get_obj_inside_cpuset_by_type(topology, core_cpuset,
                                                HWLOC_OBJ_PU, i);
      hwloc_bitmap_or(cpuset, cpuset, obj->cpuset);
    }
  }

  /* Allocate topology object */
  if ((err = hwloc_topology_init(restrictedTopology)))
    return -1;

  /* Duplicate current topology object */
  if ((err = hwloc_topology_dup(restrictedTopology, topology)))
    return -1;

  /* Restrict topology */
  if ((err = hwloc_topology_restrict(*restrictedTopology, cpuset,
                                     HWLOC_RESTRICT_FLAG_ADAPT_DISTANCES)))
    return -1;
  /* BEGIN DEBUG */
  if (sctk_get_verbosity() >= 3) {
    fprintf(stderr, "[[%d]] __mpcomp_restrict_topology: RESTRICTED TOPOLOGY\n",
            taskRank);
    sctk_print_specific_topology(stderr, *restrictedTopology);
  }
  /* END DEBUG */

  return 0;
}
#endif /* MOVE TO OPENMP MPCOMP_TREE_BIS.C */

#include <sched.h>
#if defined(HP_UX_SYS)
#include <sys/param.h>
#include <sys/pstat.h>
#endif
#ifdef HAVE_UTSNAME
static struct utsname utsname;
#else
struct utsname
{
	char sysname[];
	char nodename[];
	char release[];
	char version[];
	char machine[];
};

static struct utsname utsname;

  static int
uname (struct utsname *buf)
{
	buf->sysname = "";
	buf->nodename = "";
	buf->release = "";
	buf->version = "";
	buf->machine = "";
	return 0;
}

#endif

typedef hwloc_topology_t extls_topo_t;
extls_topo_t* extls_get_topology_addr()
{
	return &topology;
}

int sctk_topology_convert_os_pu_to_logical( int pu_os_id )
{
	hwloc_cpuset_t this_pu_cpuset = hwloc_bitmap_alloc();
	hwloc_bitmap_only(this_pu_cpuset, pu_os_id);
	
	hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type(topology, this_pu_cpuset, HWLOC_OBJ_PU, 0);
	
	hwloc_bitmap_free( this_pu_cpuset );
	
	if( !pu )
	{
		return 0;
	}

	return pu->logical_index;
}

int sctk_topology_convert_logical_pu_to_os( int pu_id )
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, pu_id );
	
	if( !target_pu )
	{
		/* Assume 0 in case of error */
		return 0;
	}
	
	return target_pu->os_index;	
}

hwloc_const_cpuset_t sctk_topology_get_machine_cpuset()
{
	return  hwloc_topology_get_allowed_cpuset( topology );
}

hwloc_const_cpuset_t sctk_topology_get_numa_cpuset( int pu_id )
{
	hwloc_const_cpuset_t ret;
	
	if( !sctk_is_numa_node () )
	{
		ret =  hwloc_topology_get_allowed_cpuset( topology );
	}
	else
	{
		hwloc_obj_t target_pu = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, pu_id );

                if (!target_pu) {
                  sctk_warning("INFO : Could not extract NUMA topology");
                  return hwloc_topology_get_allowed_cpuset(topology);
                }

                assume(target_pu != NULL);

                hwloc_obj_t parent = target_pu->parent;

                while (parent->type != HWLOC_OBJ_NODE) {
                  if (!parent)
                    break;

                  parent = parent->parent;
                }

                assume(parent != NULL);

                ret = parent->cpuset;
        }

        return ret;
}


hwloc_cpuset_t sctk_topology_get_socket_cpuset( int pu_id )
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, pu_id );
	assume( target_pu != NULL );
	
	hwloc_obj_t parent = target_pu->parent;
	
	while( parent->type != HWLOC_OBJ_SOCKET )
	{
		if( !parent )
			break;
		
		parent = parent->parent;
	}
	
	assume( parent != NULL );
	
	return parent->cpuset;
}

int sctk_topology_get_socket_number() {
  return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_SOCKET);
}

int sctk_topology_get_socket_id(int os_level) {
  int cpu = sctk_get_cpu();

  if (cpu < 0) {
    return cpu;
  }

  hwloc_obj_t target_pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpu);
  assume(target_pu != NULL);

  hwloc_obj_t parent = target_pu->parent;

  while (parent->type != HWLOC_OBJ_SOCKET) {
    if (!parent)
      break;

    parent = parent->parent;
  }

  assume(parent != NULL);

  if (os_level) {
    return parent->os_index;
  } else {
    return parent->logical_index;
  }
}

hwloc_cpuset_t sctk_topology_get_core_cpuset( int pu_id )
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, pu_id );
	assume( target_pu != NULL );
	
	hwloc_obj_t parent = target_pu->parent;
	
	while( parent->type != HWLOC_OBJ_CORE )
	{
		if( !parent )
			break;
		
		parent = parent->parent;
	}
	
	assume( parent != NULL );
	
	return parent->cpuset;
}

hwloc_cpuset_t sctk_topology_get_pu_cpuset( int pu_id )
{
	hwloc_obj_t target_pu = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, pu_id );
	assume( target_pu != NULL );
	return target_pu->cpuset;
}


int __sctk_topology_get_roots_for_level( hwloc_obj_t obj, hwloc_cpuset_t roots )
{
	if( obj->type == HWLOC_OBJ_PU )
	{
		hwloc_bitmap_or( roots, roots, obj->cpuset ); 
	 	return 1;
	}
	
	unsigned int i;
	
	int ret = 0;
	
	for( i = 0 ; i < obj->arity ; i++ )
	{
		ret = __sctk_topology_get_roots_for_level( obj->children[i] , roots );
		
		if( ret )
			break;
	}
	
	return ret;
}

hwloc_cpuset_t sctk_topology_get_roots_for_level( hwloc_obj_type_t type )
{
	hwloc_cpuset_t roots = hwloc_bitmap_alloc();
	
	/* Handle the case where the node is not a numa node */
	if( !sctk_is_numa_node () )
	{
		/* Just tell that the expected value was MACHINE */
		type = HWLOC_OBJ_MACHINE;
	}
	
	int pu_count = hwloc_get_nbobjs_by_type(topology, type);

	int i;
	
	for( i = 0 ; i < pu_count ; i++ )
	{
		hwloc_obj_t obj = hwloc_get_obj_by_type(topology, type, i);
		 __sctk_topology_get_roots_for_level( obj, roots );
		obj = hwloc_get_next_obj_by_type( topology, type, obj );
	}
	
	if( hwloc_bitmap_iszero( roots ) )
	{
		sctk_fatal("Did not find any roots for this level");
	}
	
	return roots;
}

int __sctk_topology_distance_fill_prefix(hwloc_obj_t *prefix,
                                         hwloc_obj_t target_obj) {
  /* Register in the tree */
  if (!target_obj)
    return 0;

  int depth = 0;

  /* Compute depth as it is not set for
   * PCI devices ... */
  hwloc_obj_t cur = target_obj;

  while (cur->parent) {
    depth++;
    cur = cur->parent;
  }

  /* Set depth +1 as NULL */
  prefix[depth + 1] = NULL;

  /* Prepare to walk up the tree */
  cur = target_obj;
  int cur_depth = depth;

  sctk_nodebug("================\n");

  while (cur) {
    sctk_nodebug("%d == %d == %s", cur_depth, cur->logical_index,
                 hwloc_obj_type_string(cur->type));
    prefix[cur_depth] = cur;
    cur_depth--;
    cur = cur->parent;
  }

  return depth;
}


int sctk_topology_distance_from_pu( int source_pu, hwloc_obj_t target_obj )
{
	hwloc_obj_t source_pu_obj = hwloc_get_obj_by_type(topology,HWLOC_OBJ_PU, source_pu );
	
	/* No such PU then no distance */
	if( !source_pu_obj )
		return -1;
	
	/* Compute topology depth */
	int topo_depth = hwloc_topology_get_depth(topology);
	
	/* Allocate prefix lists */
        hwloc_obj_t *prefix_PU =
            sctk_malloc((topo_depth + 1) * sizeof(hwloc_obj_t));
        hwloc_obj_t *prefix_target =
            sctk_malloc((topo_depth + 1) * sizeof(hwloc_obj_t));

        assume(prefix_PU != NULL);
        assume(prefix_target != NULL);

        /* Fill them with zeroes */
        int i;
        for (i = 0; i < topo_depth; i++) {
          prefix_PU[i] = NULL;
          prefix_target[i] = NULL;
        }

        /* Compute the prefix of the two objects in the tree */
        int pu_depth =
            __sctk_topology_distance_fill_prefix(prefix_PU, source_pu_obj);
        int obj_depth =
            __sctk_topology_distance_fill_prefix(prefix_target, target_obj);

        /* Extract the common prefix */
        int common_prefix = 0;
        for (i = 0; i < topo_depth; i++) {
          if (prefix_PU[i]->type == prefix_target[i]->type &&
              prefix_PU[i]->logical_index == prefix_target[i]->logical_index) {
            common_prefix = i;
          } else {
            break;
          }
        }

        /* From this we know that the distance is the sum of the two prefixes
         * without the common part plus one */
        int distance = (pu_depth + obj_depth - (common_prefix * 2));

        sctk_nodebug("Common prefix %d PU %d OBJ %d == %d", common_prefix,
                     pu_depth, obj_depth, distance);

        /* Free prefixes */
        sctk_free(prefix_PU);
        sctk_free(prefix_target);

        /* Return the distance */
        return distance;
}


/*! \brief Return the current core_id
*/
static __thread int sctk_get_cpu_val = -1;

static inline int sctk_get_cpu_intern ()
{
	hwloc_cpuset_t set = hwloc_bitmap_alloc();

	int ret = hwloc_get_last_cpu_location(topology, set, HWLOC_CPUBIND_THREAD);

	assume(ret!=-1);
	assume(!hwloc_bitmap_iszero(set));


	/* Check if HWLOC_XMLFILE env variable is set. This variable aims at modifing 
	*  a fake topology to test the runtime bahivior. On the downside, hwloc 
	*  disables binding. */

	int isset_hwloc_xmlfile =  (getenv ("HWLOC_XMLFILE") != NULL);

	/* Check if HWLOC_THISSYSTEM env variable is set. This variable aims at asserting 
	*  that the topology loaded with the HWLOC_XMLFILE env variable is the same as
	*  this system. */

	int isset_hwloc_thissystem = 0;
	char* hwloc_thissystem = getenv ("HWLOC_THISSYSTEM");
	if (hwloc_thissystem != NULL) isset_hwloc_thissystem =  (strcmp ( hwloc_thissystem , "1") == 0);

	if ( !isset_hwloc_xmlfile || (isset_hwloc_xmlfile && isset_hwloc_thissystem) )
	{
		/* Check if only one CPU in the CPU set. Maybe there is a simpler function
		* to do that */
		if(sctk_xml_specific_path == NULL)  
		assume (hwloc_bitmap_first(set) ==  hwloc_bitmap_last(set));
	}

	hwloc_obj_t pu = hwloc_get_obj_inside_cpuset_by_type(topology, set, HWLOC_OBJ_PU, 0);

	if (!pu)
	{
		hwloc_bitmap_free(set);
		return -1;
	}

	/* And return the logical index */
	int cpu = pu->logical_index;

	hwloc_bitmap_free(set);

	return cpu;
}

unsigned int sctk_get_cpu()
{
	if(sctk_get_cpu_val < 0)
	{
		sctk_spinlock_lock(&topology_lock);
		unsigned int ret = (unsigned int)sctk_get_cpu_intern();
		sctk_spinlock_unlock(&topology_lock);
		return ret;
	}
	else
	{
		return (unsigned int)sctk_get_cpu_val;
	}
}

void sctk_topology_init_cpu()
{
	sctk_get_cpu_val = -1;
}

int sctk_get_ht_per_core(void)
{
    int pu_per_core;
    hwloc_obj_t first_core;
    hwloc_topology_t hwtopology;

    hwloc_topology_init(&hwtopology);
    hwloc_topology_load(hwtopology);

    first_core = hwloc_get_obj_by_type( hwtopology, HWLOC_OBJ_CORE, 0 );
    sctk_assert( first_core );

    pu_per_core = hwloc_get_nbobjs_inside_cpuset_by_type( hwtopology, first_core->cpuset, HWLOC_OBJ_PU );
    assert( pu_per_core > 0 );
    
    return pu_per_core; 
}

int sctk_get_pu_number()
{
	int core_number;
	hwloc_topology_t hwtopology;
	hwloc_topology_init(&hwtopology);
	hwloc_topology_load(hwtopology);

	int depth = hwloc_get_type_depth(hwtopology, HWLOC_OBJ_PU);
	if(depth == HWLOC_TYPE_DEPTH_UNKNOWN)
	{
		core_number = -1;
	}
	else
	{
		core_number = hwloc_get_nbobjs_by_depth(hwtopology, depth);
	}

	hwloc_topology_destroy(hwtopology);
	return core_number;
}

int sctk_get_pu_number_by_core(hwloc_topology_t hwtopology, int core)
{
	hwloc_obj_t obj_pu_per_core = hwloc_get_obj_by_type(hwtopology, HWLOC_OBJ_CORE, core);
	int pu_per_core = obj_pu_per_core->arity;

	return pu_per_core;
}

/*! \brief Initialize the topology module
*/
void sctk_topology_init ()
{
	char* xml_path;

	#ifdef MPC_Message_Passing
	/* let the interface choose to init or not */
	sctk_pmi_init();
	#endif

	xml_path = getenv("MPC_SET_XML_TOPOLOGY_FILE");
	sctk_xml_specific_path = xml_path;

	hwloc_topology_init(&topology);

	if(xml_path != NULL)
	{
		fprintf(stderr,"USE XML file %s\n",xml_path);
		hwloc_topology_set_xml(topology,xml_path);
	}



	hwloc_topology_load(topology);

    hwloc_topology_init(&topology_compute_node);
	hwloc_topology_init(&topology_full);

	if(xml_path != NULL)
	{
		hwloc_topology_set_xml(topology_full,xml_path);
        hwloc_topology_set_xml(topology_compute_node,xml_path);
	}

	/* Set flags to make sure devices are also loaded */
	hwloc_topology_set_flags( topology_full, HWLOC_TOPOLOGY_FLAG_IO_DEVICES );

	hwloc_topology_load(topology_full);
    hwloc_topology_load(topology_compute_node);

	topology_cpuset = hwloc_bitmap_alloc();

    /*graphical option*/
    if(sctk_enable_graphic_placement || sctk_enable_text_placement){
        hwloc_cpuset_t newset;
        newset = hwloc_bitmap_alloc();
        int ret = hwloc_get_last_cpu_location(topology, newset, HWLOC_CPUBIND_THREAD);
        assert(ret == 0);
        hwloc_obj_t obj;
        obj = hwloc_get_obj_inside_cpuset_by_type(topology, newset,HWLOC_OBJ_PU, 0);
        hwloc_obj_t cluster = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_MACHINE, obj);
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

	support = hwloc_topology_get_support(topology);

	sctk_only_once ();
	sctk_print_version ("Topology", SCTK_LOCAL_VERSION_MAJOR,
	SCTK_LOCAL_VERSION_MINOR);

	sctk_restrict_topology();

	/*  load devices */
	sctk_device_load_from_topology( topology_full );


	#ifndef WINDOWS_SYS
	gethostname (sctk_node_name, SCTK_MAX_NODE_NAME);
	#else
	sprintf (sctk_node_name, "localhost");
	#endif

	uname (&utsname);

	if (!hwloc_bitmap_iszero(pin_processor_bitmap))
	{
		sctk_print_topology (stderr);
	}	

}

/*! \brief Destroy the topology module
*/
void sctk_topology_destroy (void)
{
    if(sctk_enable_graphic_placement){

        if(sctk_get_local_process_rank() == 0){
            hwloc_obj_t cluster = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_MACHINE, 0);
            int lenght_max;


            lenght_max = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);

            if(cluster != NULL){
                FILE *f = fopen(placement_txt, "r");
                if(f != NULL){
                    sctk_read_format_option_graphic_placement_and_complet_topo_infos(f, lenght_max);
                }

                name_and_date_file_text(file_placement);
                strcat(file_placement, ".xml");
                hwloc_topology_export_xml(topology_compute_node,file_placement); 
                if(1){//TODO one among each nodes
                    fprintf(stdout,"/* --graphic-placement : \n.xml dated file has been generated for each compute node to vizualise topology and thread placement with their infos.\nYou can use the command \"lstopo -i file.xml\" to vizualise graphicaly */\n");
                    fprintf(stdout, "\n/* .xml legend : one color per MPI task. White text policy means the thread is master of a MPI task in MPC */\n\n"); 
                    fflush(stdout);
                    remove(placement_txt);
                }
            }
        }
    }
    if(sctk_enable_text_placement){
        if(sctk_get_local_process_rank() == 0){
            printf("proc %d",sctk_get_local_process_rank());
            hwloc_obj_t cluster = hwloc_get_obj_by_type(topology_compute_node, HWLOC_OBJ_MACHINE, 0);
            int lenght_max;
            int lenght_min;
            int lenght;
            lenght_max = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_PU);
            lenght_min = hwloc_get_nbobjs_by_type(topology_compute_node, HWLOC_OBJ_CORE);
            if(sctk_enable_smt_capabilities){
                lenght = lenght_max;
            }
            else{
                lenght = lenght_min;
            }
            /* read textual informations */
            FILE *f_textual = fopen(textual_file, "r");
            if(f_textual != NULL){
                struct sctk_text_option_s *tab_option;

                sctk_init_text_option(&tab_option);

                sctk_read_format_option_text_placement(f_textual, tab_option, lenght_max);

                const char * HostName  = hwloc_obj_get_info_by_name(cluster, "HostName");

                name_and_date_file_text(textual_file_output);
                strcat(textual_file_output, ".txt");

                hwloc_obj_t root = hwloc_get_root_obj(topology_compute_node);
                FILE *f = fopen(textual_file_output, "a");

                if(f != NULL){
                    fprintf(f,"|------------------------------HOST                 %s------------------------------|\n", HostName); 
                    int higher_logical = sctk_determine_higher_logical(tab_option->os_index, lenght);
                    int lower_logical = determine_lower_logical(tab_option->os_index, lenght);
                    if(1){//TODO one proc amongs nodes
                        fprintf(stdout,"/* --text-placement : \n.txt dated file has been generated for each compute node to vizualise topology and thread placement with their infos. */\n");
                        fflush(stdout);
                    }
                    print_children(topology_compute_node, root, 0, tab_option, lenght_max, higher_logical, lower_logical, HostName, f, 0, 0);
                    fclose(f);
                }
                /* remove temp files */
                sctk_destroy_text_option(tab_option);
                remove(textual_file);
            }
        }
    }
    hwloc_topology_destroy(topology);
}


/*! \brief Return the hostname
*/
char * sctk_get_node_name ()
{
       return sctk_node_name;
}

/*! \brief Return the first child of \p obj of type \p type
 * @param obj Parent object
 * @param type Child type
 */
hwloc_obj_t sctk_get_first_child_by_type(hwloc_obj_t obj, hwloc_obj_type_t type)
{
	hwloc_obj_t child = obj;
	do
	{
		child = child->first_child;
	}
	while(child->type != type);
	return child;
}


void sctk_print_specific_topology (FILE * fd, hwloc_topology_t hwtopology)
{
	int i;
	fprintf(fd, "Node %s: %s %s %s\n", sctk_node_name, utsname.sysname,
	utsname.release, utsname.version);
	const int pu_number = hwloc_get_nbobjs_by_type(hwtopology, HWLOC_OBJ_PU);
	
	for(i=0;i < pu_number; ++i)
	{
		hwloc_obj_t pu = hwloc_get_obj_by_type(hwtopology, HWLOC_OBJ_PU, i);
		hwloc_obj_t tmp[3];
		unsigned int node_os_index;
		tmp[0] = hwloc_get_ancestor_obj_by_type(hwtopology, HWLOC_OBJ_NODE, pu);
		tmp[1] = hwloc_get_ancestor_obj_by_type(hwtopology, HWLOC_OBJ_SOCKET, pu);
		tmp[2] = hwloc_get_ancestor_obj_by_type(hwtopology, HWLOC_OBJ_CORE, pu);

		if(tmp[0] == NULL)
		{
			node_os_index = 0;
		}
		else
		{
			node_os_index = tmp[0]->os_index;
		}

		fprintf(fd, "\tProcessor %4u real (%4u:%4u:%4u)\n", pu->os_index,
		node_os_index, tmp[1]->logical_index, tmp[2]->logical_index );
	}

	fprintf (fd, "\tNUMA: %d\n", sctk_is_numa_node ());
	
	if (sctk_is_numa_node ())
	{
		fprintf (fd, "\t\t* Node nb %d\n", sctk_get_numa_node_number());
	}

	return;
}


/*! \brief Print the topology tree into a file
 * @param fd Destination file descriptor
 */
void sctk_print_topology (FILE * fd)
{
	sctk_print_specific_topology (fd, topology);
}

/* Restrict the binding to the current cpu_set */
void sctk_restrict_binding()
{
	int err;

	err = hwloc_set_cpubind(topology, topology_cpuset, HWLOC_CPUBIND_THREAD);
	assume(!err);

	sctk_topology_init_cpu();
}

/*! \brief Bind the current thread. The function may return -1 if the thread
 * was previously bound to a core that was not managed by the HWLOC topology
 * @ param i The cpu_id to bind
 */
int sctk_bind_to_cpu (int i)
{
	char * msg = "Bind to cpu";
	int supported = support->cpubind->set_thisthread_cpubind;
	const char *errmsg = strerror(errno);

	if(i < 0){
		sctk_restrict_binding();
		return -1;
	}

	sctk_spinlock_lock(&topology_lock);

	int ret = sctk_get_cpu_intern();

	TODO("Handle specific mapping from the user");
	hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, i);
	assume(pu);

	int err = hwloc_set_cpubind(topology, pu->cpuset, HWLOC_CPUBIND_THREAD);
	if (err)
	{
		fprintf(stderr,"%-40s: %sFAILED (%d, %s)\n", msg, supported?"":"X", errno, errmsg);
	}
	//    assume(sctk_get_cpu_intern() == i);
	sctk_get_cpu_val = i;
	sctk_spinlock_unlock(&topology_lock);
	return ret;
}

/*! \brief Return the type of processor (x86, x86_64, ...)
*/
char * sctk_get_processor_name ()
{
	return utsname.machine;
}

/*! \brief Return the first core_id for a NUMA node
 * @ param node NUMA node id.
 */
int sctk_get_first_cpu_in_node (int node)
{
	hwloc_obj_t currentNode = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, node);

	while(currentNode->type != HWLOC_OBJ_CORE)
	{
		currentNode = currentNode->children[0];
	}

	return currentNode->logical_index ;
}

/*! \brief Return the total number of core for the process
*/
int sctk_get_cpu_number ()
{
	return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
}

/*! \brief Return the total number of core for the process
*/
int sctk_get_cpu_number_topology (hwloc_topology_t topo)
{
	return hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_PU);
}

/*! \brief Set the number of core usable for the current process
 * @ param n Number of cores
 * used in ethread
 */
int sctk_set_cpu_number (int n)
{
	if(n <= sctk_get_cpu_number ())
	{
		sctk_update_topology ( n, 0 ) ;
	}
	return n;
}

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
int sctk_is_numa_node ()
{
	return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) != 0;
}

/*! \brief Return 1 if the current node is a NUMA node, 0 otherwise
*/
int sctk_is_numa_node_topology (hwloc_topology_t topo)
{
	return hwloc_get_nbobjs_by_type(topo, HWLOC_OBJ_NODE) != 0;
}

/*! \brief Return the number of NUMA nodes
*/
int sctk_get_numa_node_number ()
{
	return hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE) ;
}

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
 */
int sctk_get_node_from_cpu (const int vp)
{
  if (vp < 0)
    return -1;

  if (sctk_is_numa_node()) {
    const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
    assume(pu);
    const hwloc_obj_t node =
        hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);

    return node->logical_index;
  } else {
    return 0;
  }
}

int sctk_get_physical_node_from_cpu(const int vp) {
#ifdef HAVE_HWLOC
  if (sctk_is_numa_node()) {
    const hwloc_obj_t pu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, vp);
    const hwloc_obj_t node =
        hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_NODE, pu);
    return node->os_index;
  } else {
    return 0;
  }
#else
  return 0;
#endif
}

/*! \brief Return the NUMA node according to the code_id number
 * @param vp VP
 */
int sctk_get_node_from_cpu_topology (hwloc_topology_t topo, const int vp)
{
	if(sctk_is_numa_node_topology (topo))
	{
		const hwloc_obj_t pu = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, vp);
		assume(pu);
		const hwloc_obj_t node = hwloc_get_ancestor_obj_by_type(topo, HWLOC_OBJ_NODE, pu);

		return node->logical_index;
	}
	else
	{
		return 0;
	}
}

/* print the neighborhood*/
void print_neighborhood(int cpuid, int nb_cpus, int* neighborhood, hwloc_obj_t* objs)
{
	int i;
	hwloc_obj_t currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);
	hwloc_obj_type_t type = (sctk_is_numa_node()) ? HWLOC_OBJ_NODE : HWLOC_OBJ_MACHINE;
	int currentCPU_node_id = hwloc_get_ancestor_obj_by_type(topology, type, currentCPU)->logical_index;
	int currentCPU_socket_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, currentCPU)->logical_index;
	int currentCPU_core_id = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, currentCPU)->logical_index;
	
	fprintf(stderr, "Neighborhood result starting with CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
	cpuid, currentCPU_node_id, currentCPU_socket_id, currentCPU_core_id);

	for (i = 0; i < nb_cpus; i++)
	{
		hwloc_obj_t current = objs[i];

		fprintf(stderr, "\tNeighbor[%d] -> CPU %d: (Node:%d, Socket: %d, Core: %d)\n",
		i, neighborhood[i],
		hwloc_get_ancestor_obj_by_type(topology, type, current)->logical_index,
		hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_SOCKET, current)->logical_index,
		hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index);
	}
}

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
 */
void sctk_get_neighborhood(int cpuid, unsigned int nb_cpus, int* neighborhood)
{
	unsigned int i;
	hwloc_obj_t *objs;
	hwloc_obj_t currentCPU ; 
	unsigned int nb_cpus_found;

	currentCPU = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, cpuid);

	/* alloc size for retreiving objects. We could also use a static array */
	objs = sctk_malloc(nb_cpus * sizeof(hwloc_obj_t));
	sctk_assert( objs != NULL ) ;

	/* The closest CPU is actually... the current CPU :-). 
	* Set the closest CPU as the current CPU */
	objs[0] = currentCPU;

	/* get closest objects to the current CPU */
	nb_cpus_found = hwloc_get_closest_objs(topology, currentCPU, &objs[1], (nb_cpus-1));
	/*  +1 because of the current cpu not returned by hwloc_get_closest_objs */
	assume( (nb_cpus_found + 1) == nb_cpus);

	/* fill the neighborhood variable */
	for (i = 0; i < nb_cpus; i++)
	{
		/*
		hwloc_obj_t current = objs[i];
		neighborhood[i] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index;
		*/
		neighborhood[i] = objs[i]->logical_index;
	}

	/* uncomment for printing the returned result */
	//print_neighborhood(cpuid, nb_cpus, neighborhood, objs);

	sctk_free(objs);
}

/*! \brief Return the closest core_id
 * @param cpuid Main core_id
 * @param nb_cpus Number of neighbor
 * @param neighborhood Neighbor list
 */
void  sctk_get_neighborhood_topology(hwloc_topology_t topo, int cpuid, unsigned int nb_cpus, int* neighborhood)
{
	unsigned int i;
	hwloc_obj_t *objs;
	hwloc_obj_t currentCPU ; 
	unsigned nb_cpus_found;

	currentCPU = hwloc_get_obj_by_type(topo, HWLOC_OBJ_PU, cpuid);

	/* alloc size for retreiving objects. We could also use a static array */
	objs = sctk_malloc(nb_cpus * sizeof(hwloc_obj_t));
	sctk_assert( objs != NULL ) ;

	/* The closest CPU is actually... the current CPU :-). 
	* Set the closest CPU as the current CPU */
	objs[0] = currentCPU;

	/* get closest objects to the current CPU */
	nb_cpus_found = hwloc_get_closest_objs(topo, currentCPU, &objs[1], (nb_cpus-1));
	/*  +1 because of the current cpu not returned by hwloc_get_closest_objs */
	assume( (nb_cpus_found + 1) == nb_cpus);

	/* fill the neighborhood variable */
	for (i = 0; i < nb_cpus; i++)
	{
		/*
		hwloc_obj_t current = objs[i];
		neighborhood[i] = hwloc_get_ancestor_obj_by_type(topology, HWLOC_OBJ_CORE, current)->logical_index;
		*/
		neighborhood[i] = objs[i]->logical_index;
	}

	/* uncomment for printing the returned result */
	//print_neighborhood(cpuid, nb_cpus, neighborhood, objs);

	sctk_free(objs);
}

/*
 * For OpenFabrics
 */
#ifdef MPC_USE_INFINIBAND
/* Add functions here for IB */

int sctk_topology_is_ib_device_close_from_cpu (struct ibv_device * dev, int logical_core_id)
{
	int err;

	hwloc_bitmap_t ib_set;
	ib_set = hwloc_bitmap_alloc();
	
	err = hwloc_ibv_get_device_cpuset(topology, dev, ib_set);
	
	if (err < 0)
	{
		return -1;
	}
	else
	{
		int res;

		hwloc_obj_t obj_cpu = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, logical_core_id);
		assume(obj_cpu);

		#if 0
		char *cpuset_string = NULL;
		hwloc_bitmap_list_asprintf(&cpuset_string, ib_set);
		sctk_debug("String: %s", cpuset_string);
		#endif

		hwloc_bitmap_and(ib_set, ib_set, obj_cpu->cpuset);
		res = hwloc_bitmap_iszero(ib_set);

		hwloc_bitmap_free(ib_set);
		/* If the bitmap is different from 0, we are close to the IB card, so
		* we return 1 */
		return ! res;
	}
}



#endif
