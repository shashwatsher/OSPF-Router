/*-----------------------------------------------------------------------------
 * file: sr_pwospf.c
 * date: Tue Nov 23 23:24:18 PST 2004
 * Author: Martin Casado
 *
 * Description:
 *
 *---------------------------------------------------------------------------*/

#include "sr_pwospf.h"
#include "sr_router.h"
#include "sr_if.h"
#include "sr_rt.h"
#include "pwospf_protocol.h"

#include <time.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>

lt0 lt[5];
struct interface_list * nei_ifaces_global = 0;
int sequence = 1;
int seq_last_processed = 0;
uint32_t nei_last_rcv;

int h = 0;
int table_entry = 1;


static int count_rt;

/* -- declaration of main thread function for pwospf subsystem --- */
static void* pwospf_run_thread(void* arg);

/*---------------------------------------------------------------------
 * Method: pwospf_init(..)
 *
 * Sets up the internal data structures for the pwospf subsystem
 *
 * You may assume that the interfaces have been created and initialized
 * by this point.
 *---------------------------------------------------------------------*/

int pwospf_init(struct sr_instance* sr)
{
    assert(sr);
    
    sr->ospf_subsys = (struct pwospf_subsys*)malloc(sizeof(struct
                                                      pwospf_subsys));

    assert(sr->ospf_subsys);
    pthread_mutex_init(&(sr->ospf_subsys->lock), 0);
    printf("code before making neighbour interfaces \n");
    for (int i=0; i<6; i++){
        //memset(&(lt[i]),0,sizeof(lt0));// setting all the lsa info as 0
        lt[i].subnet = 0;
        lt[i].mask = 0;
        lt[i].nei_rid = 0;
        lt[i].src_rid = 0;
        lt[i].src_ip = 0;
        lt[i].del = 0;
        memmove(lt[i].iface, "eth3", 5);
    }
    make_nei_ifaces(sr);
    //sr->ospf_subsys->nei_ifaces->neighbour_router = (struct nei_list *)malloc(sizeof(struct nei_list));
    //neighbour_list_g = sr->ospf_subsys->nei_ifaces->neighbour_router;
    //neighbour_list_g = (struct nei_list *)malloc(sizeof(struct nei_list));

    /* -- handle subsystem initialization here! -- */
    //sr_g = sr;
    /* -- start thread subsystem -- */
    if( pthread_create(&sr->ospf_subsys->thread, 0, pwospf_run_thread, sr)) {
        perror("pthread_create");
        assert(0);
    }
    

    return 0; /* success */
} /* -- pwospf_init -- */


/*---------------------------------------------------------------------
 * Method: pwospf_lock
 *
 * Lock mutex associated with pwospf_subsys
 *
 *---------------------------------------------------------------------*/

void pwospf_lock(struct pwospf_subsys* subsys)
{
    if ( pthread_mutex_lock(&subsys->lock) )
    { assert(0); }
} /* -- pwospf_subsys -- */

void inf_lock(struct interface_list * inf)
{
    if ( pthread_mutex_lock(&inf->lock) )
    { assert(0); }
} 
/*---------------------------------------------------------------------
 * Method: pwospf_unlock
 *
 * Unlock mutex associated with pwospf subsystem
 *
 *---------------------------------------------------------------------*/

void pwospf_unlock(struct pwospf_subsys* subsys)
{
    if ( pthread_mutex_unlock(&subsys->lock) )
    { assert(0); }
} /* -- pwospf_subsys -- */

void inf_unlock(struct interface_list * inf)
{
    if ( pthread_mutex_unlock(&inf->lock) )
    { assert(0); }
} 
/*---------------------------------------------------------------------
 * Method: pwospf_run_thread
 *
 * Main thread of pwospf subsystem.
 *
 *---------------------------------------------------------------------*/

static
void* pwospf_run_thread(void* arg)
{
    struct sr_instance* sr_temp;// = (struct sr_instance*)arg;
    sr_temp = (struct sr_instance*)arg;
    //sr_temp = sr_g;

    while(1){
        /* -- PWOSPF subsystem functionality should start  here! -- */
        /* broadcasting hello packets */
   
        for(int i = 0; i < OSPF_DEFAULT_LSUINT; i += OSPF_DEFAULT_HELLOINT){
            pwospf_lock(sr_temp->ospf_subsys);
            printf(" trying to broadcast hello packets \n");
            hello_packet_broadcast(sr_temp);
            pwospf_unlock(sr_temp->ospf_subsys);
            printf(" hello packet finally broadcasted \n");   
            sleep(OSPF_DEFAULT_HELLOINT);
            check_neighbour(sr_temp);
        }
        send_lsu_packet(sr_temp);
        /*pwospf_lock(sr->ospf_subsys);
        printf(" pwospf subsystem sleeping \n");
        pwospf_unlock(sr->ospf_subsys);
        sleep(2);
        printf(" pwospf subsystem awake \n");*/
    };
} /* -- run_ospf_thread -- */

void make_nei_ifaces(struct sr_instance *sr){

/* Called before any new threads are created */
//struct sr_if *cur_sr_if = sr->if_list;
//struct pwospf_iflist *cur_pw_if = sr->ospf_subsys->interfaces;
struct interface_list *if_print_nei_ifaces = 0;
struct interface_list *if_nei_ifaces = sr->ospf_subsys->nei_ifaces;
struct sr_if *if_putter = 0;
printf(" code atleast entered \n");
//sr->ospf_subsys->nei_ifaces = (struct interface_list *)malloc(sizeof(struct interface_list));

    //if_nei_ifaces2 = sr->ospf_subsys->nei_ifaces;
    //sr->ospf_subsys->nei_ifaces
    if_putter = sr->if_list;

    if(if_putter != NULL){   

          sr->ospf_subsys->nei_ifaces = (struct interface_list *)malloc(sizeof(struct interface_list));
          if_nei_ifaces = sr->ospf_subsys->nei_ifaces;
          //if_nei_ifaces = (struct interface_list *)malloc(sizeof(struct interface_list));
          
          if_nei_ifaces->addr = if_putter->ip;

          //sr->ospf_subsys->nei_ifaces->name = if_putter->name;
          memmove(if_nei_ifaces->name,if_putter->name,5);

          for(int pu=0; pu < ETHER_ADDR_LEN; pu++){
              if_nei_ifaces->mac[pu] = if_putter->addr[pu];
          }
          if_nei_ifaces->helloint = OSPF_DEFAULT_HELLOINT;//htons(OSPF_DEFAULT_HELLOINT);//
          if_nei_ifaces->mask.s_addr = if_putter->mask;
          pthread_mutex_init(&(if_nei_ifaces->lock), 0);
          if_putter = if_putter->next;
          if_nei_ifaces->next = NULL;

    }    
    while(if_putter){ 
          if_nei_ifaces->next = (struct interface_list *)malloc(sizeof(struct interface_list));
          if_nei_ifaces = if_nei_ifaces->next;

          if_nei_ifaces->addr = if_putter->ip;

          //sr->ospf_subsys->nei_ifaces->name = if_putter->name;
          memmove(if_nei_ifaces->name,if_putter->name,5);

          for(int pu=0; pu < ETHER_ADDR_LEN; pu++){
              if_nei_ifaces->mac[pu] = if_putter->addr[pu];
          }
          if_nei_ifaces->helloint = OSPF_DEFAULT_HELLOINT;//htons(OSPF_DEFAULT_HELLOINT);//
          if_nei_ifaces->mask.s_addr = if_putter->mask;//ntohl(IF_MASK);
          pthread_mutex_init(&(if_nei_ifaces->lock), 0);
          if_putter = if_putter->next;
          if_nei_ifaces->next = NULL;

    }
    
    //cur_pw_if->next = (struct pwospf_iflist *)malloc(sizeof(struct pwospf_iflist)); 
    if_print_nei_ifaces = sr->ospf_subsys->nei_ifaces;
    nei_ifaces_global = if_print_nei_ifaces;
    while(if_print_nei_ifaces){
          printf("address: %s\n", inet_ntoa(*(struct in_addr *)&if_print_nei_ifaces->addr));  
          if_print_nei_ifaces = if_print_nei_ifaces->next;
    }
}

void hello_packet_broadcast(struct sr_instance* sr)
{

struct interface_list* send_iface = 0;
struct sr_ethernet_hdr* ethernet_header = 0;
struct ip* ip_header = 0;
struct ospfv2_hdr* ospf_header = 0;
struct ospfv2_hello_hdr* hello_header = 0;
struct sr_if *if_rid = 0;

int len_eth_header, len_ip_header, len_ospfv2_header, len_ospfv2_hello_header, broadcast_length, hardware_len;

hardware_len = (sizeof(struct ip))/4;
//send_iface = sr->ospf_subsys->nei_ifaces;
send_iface = nei_ifaces_global;
if_rid = sr->if_list;

len_eth_header = sizeof (struct sr_ethernet_hdr);// Calculating the length of ethernet header
len_ip_header = sizeof (struct ip);// Calculating the length of ARP header
len_ospfv2_header = sizeof(struct ospfv2_hdr);
len_ospfv2_hello_header = sizeof(struct ospfv2_hello_hdr);
broadcast_length = len_eth_header + len_ip_header + len_ospfv2_header + len_ospfv2_hello_header;// Creating the Broadcasting length
uint8_t* hello_packet = (uint8_t*)malloc(broadcast_length);

ethernet_header = (struct sr_ethernet_hdr *) hello_packet;
ip_header = (struct ip *) (hello_packet + sizeof (struct sr_ethernet_hdr));
ospf_header = (struct ospfv2_hdr*) (hello_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
hello_header = (struct ospfv2_hello_hdr*) (hello_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr));
    
/* Set Ethernet destination MAC address to ff:ff:ff:ff:ff:ff (Broadcast) */
ethernet_header->ether_type = ntohs(ETHERTYPE_IP);
    for (int i=0; i< ETHER_ADDR_LEN; i++){
         ethernet_header->ether_dhost[i] = MAX_LEN;
    }

//eth_hdr->ether_type=htons(ETHERTYPE_IP);
/* Set IP destination IP address to 224.0.0.5 (0xe0000005) (Broadcast) */
ip_header->ip_hl = hardware_len;
ip_header->ip_v = 4;
ip_header->ip_tos= 0;
ip_header->ip_len = htons(broadcast_length - 14);
ip_header->ip_id = 0;
ip_header->ip_off = 0;
ip_header->ip_ttl = 64;
ip_header->ip_p = 89;
ip_header->ip_dst.s_addr = htonl(OSPF_AllSPFRouters);

/* Set up HELLO header. */
hello_header->helloint = OSPF_DEFAULT_HELLOINT;//htons(OSPF_DEFAULT_HELLOINT);//
hello_header->padding = 0;

/* Set up PWOSPF header. */
ospf_header->version = OSPF_V2;
ospf_header->type = OSPF_TYPE_HELLO;
ospf_header->len = htons(len_ospfv2_header + len_ospfv2_hello_header);
printf("rid in ospf: %s \n", inet_ntoa(*(struct in_addr *)&if_rid->ip)); 
ospf_header->rid = if_rid->ip;//sr->ospf_subsys->this_router->rid;
ospf_header->aid = 0;//htonl(sr->ospf_subsys->area_id);
ospf_header->autype = OSPF_DEFAULT_AUTHKEY;
ospf_header->audata = OSPF_DEFAULT_AUTHKEY;

/* Send the packet out on each interface. */

printf(" in the code for hello broadcasting \n");
while(send_iface)
{
hello_header->nmask = send_iface->mask.s_addr;
//ospf_header->csum = 0;
calc_ospf_cs(ospf_header, hello_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), len_ospfv2_header-8);
printf("send iface name: %s\n", inet_ntoa(*(struct in_addr *)&send_iface->addr)); 
ip_header->ip_src.s_addr = send_iface->addr;
ip_header->ip_sum = 0;
//ip_header->ip_sum = ((uint8_t *)ip_hdr, sizeof(struct ip));
calc_IP_cs2(ip_header);
//ip_header->ip_sum = htons(ip_hdr->ip_sum);
//memmove(eth_hdr->ether_shost, iface->mac, ETHER_ADDR_LEN);
for (int j=0; j< ETHER_ADDR_LEN; j++){
     ethernet_header->ether_shost[j] = send_iface->mac[j];
}
printf(" iface name %s \n", send_iface->name);
sr_send_packet(sr, hello_packet, broadcast_length, send_iface->name);
printf(" hello packet broadcasted \n");
send_iface = send_iface->next;
}
if(hello_packet)
free(hello_packet);
//pwospf_unlock(sr->ospf_subsys);
}
          
void calc_ospf_cs(struct ospfv2_hdr* pwospf_header, uint8_t * packet, int len) { // Calculating ICMP checksum
    
    uint32_t total_sum = 0;
    int count;
    pwospf_header->csum = 0;

    uint16_t* hdr_temp = (uint16_t *) packet;

    for (count = 0; count < len / 2; count++) {
         total_sum = total_sum + hdr_temp[count];
    }
    total_sum = (total_sum >> 16) + (total_sum & 0xFFFF);
    total_sum = total_sum + (total_sum >> 16);
   
    pwospf_header->csum = ~total_sum;
    return(pwospf_header->csum);
     
}

void calc_IP_cs2(struct ip* ip_header){ // Calculating IP checksum
 
    uint32_t total_sum = 0;
    int count;
    ip_header->ip_sum = 0;

    uint16_t* hdr_temp = (uint16_t *) ip_header;

    for (count = 0; count < ip_header->ip_hl * 2; count++) {
        total_sum = total_sum + hdr_temp[count];
    }
    total_sum = (total_sum >> 16) + (total_sum & 0xFFFF);
    total_sum = total_sum + (total_sum >> 16);
    ip_header->ip_sum = ~total_sum;
}

int ospf_cs_diff(struct ospfv2_hdr* pwospf_header, uint8_t * packet, int len) { // Calculating ICMP checksum
    
    int diff = 0;
    uint32_t total_sum = 0;
    int count;
    uint16_t csum = pwospf_header->csum;
    pwospf_header->csum = 0;

    uint16_t* hdr_temp = (uint16_t *) packet;

    for (count = 0; count < len / 2; count++) {
         total_sum = total_sum + hdr_temp[count];
    }
    total_sum = (total_sum >> 16) + (total_sum & 0xFFFF);
    total_sum = total_sum + (total_sum >> 16);
   
    pwospf_header->csum = ~total_sum;
    if( pwospf_header->csum == csum){
        diff = 1;
    }
    return(diff);
     
}

//void hello_packet_processing(struct sr_instance* sr, uint8_t * packet, char * interface){
void hello_packet_processing(void * iface_hello){    
    
    struct sr_ethernet_hdr* ethernet_header = 0;
    struct ip* ip_header = 0;
    struct ospfv2_hdr* ospf_header = 0;
    struct ospfv2_hello_hdr* hello_header = 0;
    struct interface_list *nr = 0;
    struct interface_list *if_print_ni = 0;
    uint16_t temp2;
    int len_ospfv2_header;
    
    printf("\n code before hello processing previous lock \n");
    pwospf_lock(sr_hello_proc->ospf_subsys);
    ethernet_header = (struct sr_ethernet_hdr *) packet_hello_proc;
    ip_header = (struct ip *) (packet_hello_proc + sizeof (struct sr_ethernet_hdr));
    ospf_header = (struct ospfv2_hdr*) (packet_hello_proc + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
    hello_header = (struct ospfv2_hello_hdr*) (packet_hello_proc + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr));
    len_ospfv2_header = sizeof(struct ospfv2_hdr); 
    nr = sr_hello_proc->ospf_subsys->nei_ifaces;
    printf(" hello packet aaya re baba from............**********************************..........*****............... %s \n", inet_ntoa(*(struct in_addr *)&ospf_header->rid));    

    while(nr){
          //temp = ospf_header->csum;
          temp2 = ospf_cs_diff(ospf_header, packet_hello_proc + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), len_ospfv2_header-8);
          if((ospf_header->version == OSPF_V2) && (1 == temp2) && (ospf_header->aid == 0) && (ospf_header->autype == OSPF_DEFAULT_AUTHKEY)){
              printf("code went past first comparison \n");
              printf(" nr->helloint %d \n", nr->helloint);
              if ((nr->helloint == hello_header->helloint) && (nr->mask.s_addr == hello_header->nmask)){
                   printf("code went past second comparison \n");
                   if (!strcmp(nr->name, iface_hello)){
                       printf("code went past third comparison \n");
                       if(nr->filled != 1){
                          nr->neighbour_router = (struct nei_list *)malloc(sizeof(struct nei_list));
                          nr->neighbour_router->rid = ospf_header->rid;
                          printf("error A \n");
                          nr->neighbour_router->ipa = ip_header->ip_src;
                          printf("error B \n");
                          for (int k=0; k< 6; k++){
                               nr->neighbour_router->mac[k] = ethernet_header->ether_shost[k];
                          }
                          printf("error C \n");
                          memmove(nr->neighbour_router->nei_name, iface_hello, 5);
                          nr->neighbour_router->time_alive = time(0);
                          printf("new neighbour added: %s \n", inet_ntoa(*(struct in_addr *)&nr->neighbour_router->rid)); 
                          nr->filled = 1;
                          break;
                      }else{
                            printf("neibhour Already stored \n");
                            nr->neighbour_router->time_alive = 0;
                            nr->neighbour_router->time_alive = time(0);
                      }
                  }
              }
          } nr = nr->next;      
    }
    
    if_print_ni = sr_hello_proc->ospf_subsys->nei_ifaces;
    while(if_print_ni){
          if (if_print_ni->filled == 1){
              printf("this interface for which stored %s\n", inet_ntoa(*(struct in_addr *)&if_print_ni->addr)); 
              printf("rid %s\n", inet_ntoa(*(struct in_addr *)&if_print_ni->neighbour_router->rid)); 
              printf("ip of connected interface %s\n", inet_ntoa(*(struct in_addr *)&if_print_ni->neighbour_router->ipa.s_addr)); 
          }
          if_print_ni = if_print_ni->next;
    }
    
     pwospf_unlock(sr_hello_proc->ospf_subsys);   
     //free(packet_hello_proc);
}

void send_lsu_packet(struct sr_instance* sr){

    struct interface_list* send_iface = 0;
    struct sr_ethernet_hdr* ethernet_header = 0;
    struct ip* ip_header = 0;
    struct ospfv2_hdr* ospf_header = 0;
    struct ospfv2_lsu_hdr* lsu_header = 0;
    struct sr_if *if_rid = 0;
    double timediff;
    time_t now;
    time(&now);

    int len_eth_header, len_ip_header, len_ospfv2_header, len_ospfv2_lsu_header, len_lsu, broadcast_length, hardware_len;
   
    printf(" Code in LSU sending part ---------------------------\n");

    hardware_len = (sizeof(struct ip))/4;
    send_iface = sr->ospf_subsys->nei_ifaces;
    if_rid = sr->if_list;

    len_eth_header = sizeof (struct sr_ethernet_hdr);// Calculating the length of ethernet header
    len_ip_header = sizeof (struct ip);// Calculating the length of ARP header
    len_ospfv2_header = sizeof(struct ospfv2_hdr);
    len_ospfv2_lsu_header = sizeof(struct ospfv2_hello_hdr);
    len_lsu = sizeof(struct ospfv2_lsu);
    broadcast_length = len_eth_header + len_ip_header + len_ospfv2_header + len_ospfv2_lsu_header + 3*(len_lsu);// Creating the Broadcasting length
    uint8_t* lsu_packet = (uint8_t*)malloc(broadcast_length);
    

    ethernet_header = (struct sr_ethernet_hdr *) lsu_packet;
    ip_header = (struct ip *) (lsu_packet + sizeof (struct sr_ethernet_hdr));
    ospf_header = (struct ospfv2_hdr*) (lsu_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
    lsu_header = (struct ospfv2_lsu_hdr*) (lsu_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr));
    

    ip_header->ip_hl = hardware_len;
    ip_header->ip_v = 4;
    ip_header->ip_tos= 0;
    ip_header->ip_len = htons(broadcast_length - 14);
    ip_header->ip_id = 0;
    ip_header->ip_off = 0;
    ip_header->ip_ttl = 64;
    ip_header->ip_p = 89;

/* Set up PWOSPF header. */
    ospf_header->version = OSPF_V2;
    ospf_header->type = OSPF_TYPE_LSU;
    ospf_header->len = htons(len_ospfv2_header + len_ospfv2_lsu_header);
    ospf_header->rid = if_rid->ip;
    ospf_header->aid = 0;
    ospf_header->autype = OSPF_DEFAULT_AUTHKEY;
    ospf_header->audata = OSPF_DEFAULT_AUTHKEY;

/* Set up LSU header. */
    lsu_header->seq = sequence;
    lsu_header->ttl = 64;
    lsu_header->num_adv = 3;
    sequence += 1; 

/*********** create LSA **********/
    create_lsa(sr, lsu_packet);

/* Send the packet out on each interface. */

    printf(" in the code for sending lsu \n");
    while(send_iface){
          if(send_iface->filled == 1){
              timediff = difftime(now, send_iface->neighbour_router->time_alive);// Check if the cache is outdated or not for the particular server
              if(timediff < 10){
                 
                 calc_ospf_cs(ospf_header, lsu_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), len_ospfv2_header-8);
                 
                 ip_header->ip_src.s_addr = send_iface->addr;
                 ip_header->ip_dst.s_addr = send_iface->neighbour_router->ipa.s_addr;
                 calc_IP_cs2(ip_header);

                 for (int j=0; j< ETHER_ADDR_LEN; j++){
                      ethernet_header->ether_shost[j] = send_iface->mac[j];
                 }
                 ethernet_header->ether_type = ntohs(ETHERTYPE_IP);
                 for (int i=0; i< ETHER_ADDR_LEN; i++){
                      ethernet_header->ether_dhost[i] = send_iface->neighbour_router->mac[i];
                 }
                
                 sr_send_packet(sr, lsu_packet, broadcast_length, send_iface->name);
                 printf("/******************lsu announcement sent********** to : %s \n",inet_ntoa(*(struct in_addr *)&send_iface->neighbour_router->ipa.s_addr));
              }
               
          }
          send_iface = send_iface->next;
     }
     if(lsu_packet){
        free(lsu_packet);
     }
}

void create_lsa(struct sr_instance * sr, uint8_t* lsu_packet){
       
     struct interface_list * if_list_lsa = 0;
     struct ospfv2_lsu* lsu1 = 0;
     struct ospfv2_lsu* lsu2 = 0;
     struct ospfv2_lsu* lsu3 = 0; 
     int lsa_set = 0;

     if_list_lsa = sr->ospf_subsys->nei_ifaces; 
     lsu1 = (struct ospfv2_lsu_hdr*) (lsu_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr)); 

     if(sr->routing_table != 0){
        if((sr->routing_table->gw.s_addr & if_list_lsa->mask.s_addr) == (if_list_lsa->addr & if_list_lsa->mask.s_addr)){
            lsu1->subnet = ZERO_MASK;
            lsu1->mask = ZERO_MASK;
            lsu1->rid = ZERO_MASK;
            lsa_set = 1;
        }
     }
     if(lsa_set == 0){
        lsu1->subnet = (if_list_lsa->mask.s_addr & if_list_lsa->addr);
        lsu1->mask = if_list_lsa->mask.s_addr;
        if(if_list_lsa->filled == 1){
           lsu1->rid = if_list_lsa->neighbour_router->rid;
        }else{ 
              lsu1->rid = ZERO_MASK;
        }
     }
     
     if_list_lsa = if_list_lsa->next;
     lsu2 = (struct ospfv2_lsu_hdr*) (lsu_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) + sizeof(struct     ospfv2_lsu)); 
     
     lsu2->subnet = (if_list_lsa->mask.s_addr & if_list_lsa->addr);
     lsu2->mask = if_list_lsa->mask.s_addr;
     if(if_list_lsa->filled == 1){
        lsu2->rid = if_list_lsa->neighbour_router->rid;
     }else{ 
           lsu2->rid = ZERO_MASK;
     }

     if_list_lsa = if_list_lsa->next;
     lsu3 = (struct ospfv2_lsu_hdr*) (lsu_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) + 2*(sizeof(struct ospfv2_lsu))); 
     
     lsu3->subnet = (if_list_lsa->mask.s_addr & if_list_lsa->addr);
     lsu3->mask = if_list_lsa->mask.s_addr;
     if(if_list_lsa->filled == 1){
        lsu3->rid = if_list_lsa->neighbour_router->rid;
     }else{ 
           lsu3->rid = ZERO_MASK;
     }

}

void lsu_packet_processing(void * lsu_iface){
     
    struct sr_ethernet_hdr* ethernet_header = 0;
    struct ip* ip_header = 0;
    struct ospfv2_hdr* ospf_header = 0;
    struct ospfv2_lsu_hdr* lsu_header = 0;
    struct ospfv2_lsu* lsa1 = 0;
    struct ospfv2_lsu* lsa2 = 0;
    struct ospfv2_lsu* lsa3 = 0;
    struct interface_list *nr = 0;
    struct interface_list *nr_chk = 0;
    struct interface_list *nr_chk_up = 0;
    struct sr_if *if_rid = 0;
    struct sr_if * if_walker_checker = 0;
    struct sr_rt* rt_walker_checker = 0;

    int dont_proceed = 0;
    int temp2, len_ospfv2_header;
    unsigned int len;
    uint8_t * packet;
    uint32_t gateway;
    int gh;
    int block = 0;
    int continue_ahead = 0;
    int continue_ahead2 = 0;
    int connected_router = 0;
    uint32_t array[2];
    int count_rt_value = 0;
    
    pthread_t t6 = 0;

    packet = packet_lsu_proc;
    len = len_lsu_proc;
    printf("\n code in lsu processing\n");
    pwospf_lock(sr_lsu_proc->ospf_subsys);
    ethernet_header = (struct sr_ethernet_hdr *) packet;
    ip_header = (struct ip *) (packet + sizeof (struct sr_ethernet_hdr));
    gateway = ip_header->ip_src.s_addr;
    ospf_header = (struct ospfv2_hdr*) (packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
    lsu_header = (struct ospfv2_lsu_hdr*) (packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr));
    lsa1 = (struct ospfv2_lsu_hdr*) (packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr)); 
    lsa2 = (struct ospfv2_lsu_hdr*) (packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) + sizeof(struct     ospfv2_lsu));
    lsa3 = (struct ospfv2_lsu_hdr*) (packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip) + sizeof(struct ospfv2_hdr) + sizeof(struct ospfv2_lsu_hdr) + 2*(sizeof(struct ospfv2_lsu)));
    len_ospfv2_header = sizeof(struct ospfv2_hdr); 

    printf(" error yahaan hai A\n");
    nr = sr_lsu_proc->ospf_subsys->nei_ifaces;
    nr_chk = sr_lsu_proc->ospf_subsys->nei_ifaces;
    nr_chk_up = sr_lsu_proc->ospf_subsys->nei_ifaces;
    printf(" error yahaan hai B\n");
    if_rid = sr_lsu_proc->if_list;
    
    while(nr_chk){
          if(nr_chk->filled == 1){
             if(ip_header->ip_src.s_addr == nr_chk->neighbour_router->ipa.s_addr){
                continue_ahead = 1;
                break;
             }
          }
          nr_chk = nr_chk->next;
    }
    
    if(continue_ahead == 1){
       
       if_walker_checker = sr_lsu_proc->if_list;

       array[0] = lsa1->rid;
       array[1] = lsa2->rid;
       array[2] = lsa3->rid;
       
       printf(" ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^From hop source :  %s \n",inet_ntoa(*(struct in_addr *)&gateway));
       printf(" @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ and real source %s \n",inet_ntoa(*(struct in_addr *)&ospf_header->rid));
       for(gh = 0; gh<3 ; gh++){
           if(array[gh] == if_walker_checker->ip){
              connected_router = 1;
           }
       }
       printf("connected_router %d \n", connected_router);
       if(connected_router == 1){
          while(nr_chk_up){
                if(nr_chk_up->filled == 1){
                   if(nr_chk_up->neighbour_router->rid == ospf_header->rid){
                      continue_ahead2 = 1;
                   }
                }
                nr_chk_up = nr_chk_up->next;
          }
       }
       printf("continue_ahead2 %d \n", continue_ahead2);
       if((connected_router == 1) && (continue_ahead2 == 0)){
           continue_ahead = 0;
       }
    }

    if(continue_ahead == 1){
    
    printf(" error yahaan hai C\n");
    if( ospf_header->rid == if_rid->ip){
       dont_proceed = 1;
       printf("dropping packet cuz belonging to same router \n"); 
    }
    
    printf("real rid in packet %s\n", inet_ntoa(*(struct in_addr *)&ospf_header->rid));       
    printf(" Sequence in packet %d \n", lsu_header->seq);                                 
    if((lsu_header->seq == seq_last_processed) && (nei_last_rcv == ospf_header->rid)){
        dont_proceed = 1;  //dont_proceed2 = 1;
        printf("dropping packet cuz already received \n"); 
        printf("rid in dropped packet %s\n", inet_ntoa(*(struct in_addr *)&ospf_header->rid));  
        printf(" sequence number in dropped packet %d \n", lsu_header->seq);
    }

    if(dont_proceed == 0){
       
          if(sr_lsu_proc->routing_table != 0){
             rt_walker_checker = sr_lsu_proc->routing_table;
             while(rt_walker_checker){
                   
                   count_rt_value += 1;
                   rt_walker_checker = rt_walker_checker->next;
             }
          }


       if(table_entry == 1){
          count_rt = count_rt_value;
          table_entry = 0;
       }
       
       block = check_packet_info(lsa1, lsa2, lsa3, ospf_header->rid);
    }

     printf(" dont proceed :  %d  block :  %d    count_rt :    %d \n", dont_proceed,block,count_rt);   
    if((dont_proceed == 0) && (block != 1) && (count_rt < 2)){
        
        printf(" FIRST packet received/////////////////////////////////////////////////////////////////// \n");
        printf(" From hop source :  %s \n",inet_ntoa(*(struct in_addr *)&gateway));
        printf(" and real source %s \n",inet_ntoa(*(struct in_addr *)&ospf_header->rid));

        if(h != 0){
           if(lt[0].src_rid == ospf_header->rid){
              h = 0;
           }
           if(lt[3].src_rid == ospf_header->rid){
              h = 3;
           }
        }
        
        lt[h].subnet = lsa1->subnet;
        lt[h].mask = lsa1->mask;
        lt[h].nei_rid = lsa1->rid;
        lt[h].src_rid = ospf_header->rid;
        lt[h].src_ip = gateway;
        memmove(lt[h].iface, lsu_iface, 5);

        lt[h + 1].subnet = lsa2->subnet;
        lt[h + 1].mask = lsa2->mask;
        lt[h + 1].nei_rid = lsa2->rid;
        lt[h + 1].src_rid = ospf_header->rid;
        lt[h + 1].src_ip = gateway;
        memmove(lt[h + 1].iface, lsu_iface, 5);

        lt[h + 2].subnet = lsa3->subnet;
        lt[h + 2].mask = lsa3->mask;
        lt[h + 2].nei_rid = lsa3->rid;
        lt[h + 2].src_rid = ospf_header->rid;
        lt[h + 2].src_ip = gateway;
        memmove(lt[h + 2].iface, lsu_iface, 5);
        
        h = 3;
    }

    if((lt[3].src_rid != 0) && (dont_proceed == 0) && (block != 1)){

        pthread_create(&t6, NULL, shaztra, (void *)sr_lsu_proc);
    }
    
     if(dont_proceed == 0){
        printf(" error yahaan hai D\n"); 
        temp2 = ospf_cs_diff(ospf_header, packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), len_ospfv2_header-8);
        if((ospf_header->version == OSPF_V2) && (1 == temp2) && (ospf_header->aid == 0) && (ospf_header->autype == OSPF_DEFAULT_AUTHKEY)){ 
            printf(" error yahaan hai E\n");  
            nei_last_rcv = ospf_header->rid;
            while(nr){
                  if(nr->filled == 1){
                     if(ip_header->ip_src.s_addr != nr->neighbour_router->ipa.s_addr){
                        printf("ip header source of lsu packet %s\n", inet_ntoa(*(struct in_addr *)&ip_header->ip_src.s_addr));  
                        printf("ip from neighbour where packet is to be forwarded %s\n", inet_ntoa(*(struct in_addr *)&nr->neighbour_router->ipa.s_addr));  
                        ethernet_header->ether_type = ntohs(ETHERTYPE_IP);
                        for (int i=0; i< ETHER_ADDR_LEN; i++){
                             ethernet_header->ether_shost[i] = nr->mac[i];
                             ethernet_header->ether_dhost[i] = nr->neighbour_router->mac[i];  
                        }
                        ip_header->ip_src.s_addr = nr->addr;
                        ip_header->ip_dst = nr->neighbour_router->ipa;
                        lsu_header->ttl -= 1;
                        calc_ospf_cs(ospf_header, packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), len_ospfv2_header-8);
                        calc_IP_cs2(ip_header);
                        seq_last_processed = lsu_header->seq;
                        sr_send_packet(sr_lsu_proc, packet, len, nr->name);
                        printf("LSU packet forwarded ------------>>>>> \n");
                        printf("rid in sent packet %s\n", inet_ntoa(*(struct in_addr *)&ospf_header->rid));  
                        printf(" sequence number in sent packet %d \n", lsu_header->seq); 
                        break;
                      }
                   }
                   nr = nr->next;
            }
         }
      }
     }                          
     pwospf_unlock(sr_lsu_proc->ospf_subsys);
}
                           
void shaztra (void * sr){

     struct sr_instance* sr_temp;
     struct sr_rt* rt_walker = 0;
     struct sr_if * if_walker1 = 0;
     struct sr_if * if_walker_unknown_subnet = 0;
     int j, k, p;
     char iface_temp[5] = "eth1";

     sr_temp = (struct sr_instance*)sr;

     printf (" CODE to compute shaztra............................................................>>>>>>>>>> \n");
        for(int c = 0; c<6; c++){
            printf(" Subnet : %s\n", inet_ntoa(*(struct in_addr *)&lt[c].subnet)); 
            printf(" mask : %s\n", inet_ntoa(*(struct in_addr *)&lt[c].mask)); 
            printf(" nei_id : %s\n", inet_ntoa(*(struct in_addr *)&lt[c].nei_rid)); 
            printf(" src_id : %s\n", inet_ntoa(*(struct in_addr *)&lt[c].src_rid)); 
            printf(" Interface : %s \n", lt[c].iface);
            printf("&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&& end of announcement @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n");
        }

        for( k = 0; k < 3; k++){
            for( j = 5; j > 2; j--){
                if(lt[k].subnet == lt[j].subnet){
                   printf(" SAME subnets found .........>>>>>>>>>>>>>>>>>>>>????????????????????????????@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n");
                   if((lt[k].nei_rid == ZERO_MASK) && (lt[j].nei_rid == ZERO_MASK)){
                       if((lt[k].src_rid & lt[k].mask) == lt[k].subnet){
                           printf("dont delete the entry \n");
                       }else{
                             lt[k].del = 1;
                             printf("One duplicate entry deleted///////////////////////////////////////////////////////////// \n");
                             break;
                       }
                       if((lt[j].src_rid & lt[j].mask) == lt[j].subnet){
                           printf("dont delete the entry \n");
                       }else{
                             lt[j].del = 1;
                             printf("One duplicate entry deleted///////////////////////////////////////////////////////////// \n");
                             break;
                       }
                   }else{ 
                         lt[k].del = 1;
                         printf("One duplicate entry deleted///////////////////////////////////////////////////////////// \n");
                         break;
                   }
                }
            }
            if((lt[k].del == 1) || (lt[j].del == 1)){
                break;
            }
        }
        
        if(sr_temp->routing_table != 0){
           rt_walker = sr_temp->routing_table;
           rt_walker->next = (struct sr_rt *)malloc(sizeof(struct sr_rt));
           rt_walker = rt_walker->next;
        }else{     
              sr_temp->routing_table = (struct sr_rt *)malloc(sizeof(struct sr_rt));
              rt_walker = sr_temp->routing_table;
              if_walker_unknown_subnet = sr_get_interface(sr_temp, iface_temp);
              rt_walker->dest.s_addr = if_walker_unknown_subnet->ip;
              rt_walker->mask.s_addr = if_walker_unknown_subnet->mask;
              rt_walker->gw.s_addr = ZERO_MASK;
              memmove(rt_walker->interface, iface_temp, 5);
              rt_walker->next = (struct sr_rt *)malloc(sizeof(struct sr_rt));
              rt_walker = rt_walker->next;
        }
      
        if_walker1 = sr_temp->if_list;
        for(p = 0; p < 6; p++){
            if((lt[p].del != 1) && (lt[p].iface != "")){
               rt_walker->dest.s_addr = lt[p].subnet;
               rt_walker->mask.s_addr = lt[p].mask;
               if(if_walker1->ip == lt[p].nei_rid){
                  rt_walker->gw.s_addr = ZERO_MASK;
               }else{
                     rt_walker->gw.s_addr = lt[p].src_ip;
               }
               memmove(rt_walker->interface, lt[p].iface, 5);
               if(p < 5){
                  rt_walker->next = (struct sr_rt *)malloc(sizeof(struct sr_rt));
                  rt_walker = rt_walker->next;
               }else{
                     rt_walker->next = NULL;
               }
            }
        }
        
        sr_print_routing_table(sr_temp);  
}
                                 
void check_neighbour( struct sr_instance* sr){
    
     struct interface_list* check_iface = 0;  
     double timediff;    
     time_t now;
     time(&now);
     int fail = 0;

     printf("Neighbour router ke code ke andar...................................>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
     check_iface = sr->ospf_subsys->nei_ifaces;;
     
     while(check_iface){
           if(check_iface->filled == 1){
              timediff = difftime(now, check_iface->neighbour_router->time_alive);
              printf(" ........................................... Nei router ki validity check ho rahi hai abhi >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
              if(timediff > 10){
                 check_iface->neighbour_router = 0;
                 check_iface->filled = 0;
                 printf(" clearing data of an old neighbour77777777777777777777777777777777777777777777777777MMMMMMMMMMMMMMM*********** \n");   
                 fail = 1;
              }
           }
           check_iface = check_iface->next;
     }

     if (fail == 1){
         send_lsu_packet(sr);
     }
}             

int check_packet_info(struct ospfv2_lsu* lsa1, struct ospfv2_lsu* lsa2, struct ospfv2_lsu* lsa3, uint32_t from){
   
    uint32_t array[2];
    int mismatch = 0;
    int match = 0;
    
    array[0] = lsa1->rid;
    array[1] = lsa2->rid;
    array[2] = lsa3->rid;
    
    printf(" >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> Code in mismatch check <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<< \n");
    for(int y=0; y < 3; y++){
        if(lt[y].src_rid == 0){
           printf("first time check ******************************************************************** \n");
           return(2);
        }
        if(lt[y].src_rid == from){
           match += 1;
           printf(" MAtch found///////////////////////////////////////////////////// from %s \n",inet_ntoa(*(struct in_addr *)&from)); 
           if(lt[y].nei_rid != array[y]){
              printf(" Mismatch found ################################################################################################ \n");
              mismatch = 1;
              break;
           }
        }
    }

    if((match == 3) && (mismatch != 1)){
       return(1);
    }

    if(mismatch == 1){
       return(0);
    }

    for(int y=3; y < 6; y++){
        if(lt[y].src_rid == 0){
           printf("first time check ******************************************************************** \n");
           return(2);
        }
        if(lt[y].src_rid == from){
           printf(" MAtch found///////////////////////////////////////////////////// from %s \n",inet_ntoa(*(struct in_addr *)&from)); 
           if(lt[y].nei_rid != array[y-3]){
              printf(" Mismatch found ################################################################################################ \n");
              mismatch = 1;
              break;
           }
        }
    }

    if(mismatch == 1){
       printf("mismatch found??????????????????????????????????????????????????????????????????????????????? \n");
       return(0);
    }else{  
          printf(" No.....................mismatch found??????????????????????????????????????????????????????????????????????????????? \n");
          return(1);
    }
}
    




















    



























