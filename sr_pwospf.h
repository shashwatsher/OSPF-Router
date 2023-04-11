/*-----------------------------------------------------------------------------
 * file:  sr_pwospf.h
 * date:  Tue Nov 23 23:21:22 PST 2004 
 * Author: Martin Casado
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#ifndef SR_PWOSPF_H
#define SR_PWOSPF_H

//#ifdef _LINUX_
//#include <stdint.h>
//#endif /* _LINUX_ */

#include <sys/types.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_rt.h"

#define IF_MASK 0xfffffffe
#define ZERO_MASK 0x00000000
//#include <pthread.h>
//#include <inttypes.h>

/* forward declare */

struct sr_instance * sr_add_r;
struct sr_instance * sr_lsu_proc;
uint8_t * packet_lsu_proc;
struct sr_instance * sr_hello_proc;
uint8_t * packet_hello_proc;
unsigned int len_lsu_proc;
//lt0 ct[5];
//static lt0 lt[5];


//struct nei_list * neighbour_list_g;


struct pwospf_subsys
{
    /* -- pwospf subsystem state variables here -- */


    /* -- thread and single lock for pwospf subsystem -- */
    pthread_t thread;
    pthread_mutex_t lock;
    struct interface_list* nei_ifaces;
};

struct interface_list
{
//struct in_addr address;
uint32_t addr;
struct in_addr mask;
char name[32];
uint16_t helloint;
unsigned char mac[ETHER_ADDR_LEN];
struct nei_list * neighbour_router;
int filled;
//int nbr_size;
//int nbr_buf_size;
struct interface_list *next;
pthread_mutex_t lock;
}__attribute__ ((packed));

struct nei_list
{
//uint32_t n_ip;
uint32_t rid;
struct in_addr ipa;
unsigned char mac[ETHER_ADDR_LEN];
char nei_name[32];
time_t time_alive;
/*time time*/
//time_t timenotvalid; /*The time when this entry is no longer valid*/
//struct neighbor_list *next;
}__attribute__ ((packed));

int pwospf_init(struct sr_instance* sr);
void make_nei_ifaces(struct sr_instance *sr);
void hello_packet_broadcast(struct sr_instance* sr);
//uint16_t calc_ospf_cs(struct ospfv2_hdr* pwospf_header, uint8_t * packet, int len);
void calc_IP_cs2(struct ip* ip_header);
//void process_hello_msg(struct sr_instance* sr, uint8_t * packet, char * interface);
void lsu_packet_processing(void * lsu_iface);
void shaztra (void * sr);
//int check_packet_info(struct ospfv2_lsu* lsa1, struct ospfv2_lsu* lsa2, struct ospfv2_lsu* lsa3, uint32_t from);
void hello_packet_processing(void * iface_hello);
#endif /* SR_PWOSPF_H */
