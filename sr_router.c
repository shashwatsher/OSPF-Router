/**********************************************************************
 * file:  sr_router.c 
 * date:  Mon Feb 18 12:50:42 PST 2002  
 * Contact: casado@stanford.edu 
 *
 * Description:
 * 
 * This file contains all the functions that interact directly
 * with the routing table, as well as the main entry method
 * for routing. 11
 * 90904102
 * code for IP/ICMP Checksum referenced from http://locklessinc.com/articles/tcp_checksum/
 **********************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>


#include "sr_if.h"
#include "sr_rt.h"
#include "sr_router.h"
#include "sr_protocol.h"
#include "sr_pwospf.h"
#include "pwospf_protocol.h"

/*--------------------------------------------------------------------- 
 * Method: sr_init(void)
 * Scope:  Global
 *
 * Initialize the routing subsystem
 * 
 *---------------------------------------------------------------------*/
nt0 nt[10];
tp0 tp[10];

uint32_t not_known_server;
uint8_t * packet_g;
int reply_obtained = 0;
int thread_started = 0;

struct sr_instance* sr_g;
struct ip* ip_header_g;

unsigned int len_g;
uint32_t univ_mask;
char * ARP_needed_for_iface;

void sr_init(struct sr_instance* sr) 
{
    /* REQUIRES */
    assert(sr);
    for (int i=0; i<11; i++){
        memset(&(nt[i]),0,sizeof(nt0));// setting all the servers as 0
        memset(&(tp[i]),0,sizeof(tp0));// setting all the packets as 0
    }
    
    /* Add initialization code here! */
    
} /* -- sr_init -- */



/*---------------------------------------------------------------------
 * Method: sr_handlepacket(uint8_t* p,char* interface)
 * Scope:  Global
 *
 * This method is called each time the router receives a packet on the
 * interface.  The packet buffer, the packet length and the receiving
 * interface are passed in as parameters. The packet is complete with
 * ethernet headers.
 *
 * Note: Both the packet buffer and the character's memory are handled
 * by sr_vns_comm.c that means do NOT delete either.  Make a copy of the
 * packet instead if you intend to keep it around beyond the scope of
 * the method call.
 *
 *---------------------------------------------------------------------*/

void sr_handlepacket(struct sr_instance* sr, 
        uint8_t * packet/* lent */, 
        unsigned int len,
        char* interface/* lent */)
{
    struct sr_icmphdr* icmp_header = 0;
    struct sr_ethernet_hdr* ethernet_header_handle_pak = 0;
    struct ospfv2_hdr* ospf_header = 0;
    struct ip* ip_header = 0;
    struct sr_if * if_checker = 0;
    struct sr_if * if_checker3 = 0;
    struct sr_rt* rt_walker_fu = 0;
    struct sr_rt* rt_walker_fu2 = 0; 
    struct interface_list* nr = 0; 
    struct interface_list* nr0 = 0;

    pthread_t t1 = 0;
    pthread_t t2 = 0;
    pthread_t t3 = 0;
    pthread_t t4 = 0;

    int toe = 0;
    int this_router = 0;
    int this_router3 = 0;
    int i;
    char * hello_iface = malloc(4);
    uint32_t ipd = 1;
    uint32_t mask_conn_eth;
    uint32_t dest;
    char * lsu_iface = malloc(4);
    int match = 0;
    int dont_proceed = 0; 
    uint32_t gateway_g;
    int zero_made = 0;
    int agg_ip = 0;
    int direct_send = 0;
    int lp;

    /* REQUIRES */
    assert(sr);
    assert(packet);
    assert(interface);
    printf("*** -> Received packet of length %d \n",len);
    
    
    ethernet_header_handle_pak = (struct sr_ethernet_hdr *) packet;
    ip_header = (struct ip *) (packet + sizeof (struct sr_ethernet_hdr));
    icmp_header = ((struct sr_icmphdr*)(packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip)));
    ospf_header = (struct ospfv2_hdr*) (packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip));
    
    

    if (ntohs(ethernet_header_handle_pak->ether_type) == ETHERTYPE_IP) { //To check if the received packet is ICMP/IP type

/*---------------------------------------code for RT buildup----------------------------------------*/
        if (not_known_server == ip_header->ip_dst.s_addr){ //To send host unreachable message on 5 ARP Request fails continously
             
            toe = 3;
            sr_send_GW_ICMP_packet(sr, 0, ethernet_header_handle_pak, ip_header, packet, len, interface, toe); 
            printf("Host is unreachable \n");
        }
        
        if ((ip_header->ip_p == IP_P_TCP || ip_header->ip_p == IP_P_UDP)){ // To check if the received packet is TCP/UDP type
              
             if_checker = sr->if_list; //To check if IP belongs to router
             for (i = 0; i< 3; i++){ 
                  if (if_checker->ip == ip_header->ip_dst.s_addr){
                      this_router = 1;
                      break;
                  }
                  if_checker = if_checker->next;
             }
             if (this_router == 1){ // If the TCP/UDP packet belongs to the router send error message
                  
                 printf("TCP/UDP packets not handled \n");
                 toe = 2;
                 sr_send_GW_ICMP_packet(sr, icmp_header, ethernet_header_handle_pak, ip_header, packet, len, interface, toe);
             }
             
        }  

        if (ip_header->ip_ttl == 1 && toe != 2){ //To check if the received packet doesn't have its TTL as 1
             
            printf("Time exceeded \n");
            toe = 1;
            sr_send_GW_ICMP_packet(sr, icmp_header, ethernet_header_handle_pak, ip_header, packet, len, interface, toe); 
        }
        
        if(toe == 0){
           if (ip_header->ip_p == 89){
                
               if(ospf_header->type == OSPF_TYPE_HELLO){
                  printf(" code in the hello chk \n");
                  sr_hello_proc = sr;
                  packet_hello_proc = malloc(len + 1);
                  memcpy(packet_hello_proc,packet,len); 
                  //hello_packet_processing(sr, packet, interface);
                  memcpy(hello_iface,interface,5);
                  pthread_create(&t4, NULL, hello_packet_processing, (void *)hello_iface);
                  //free(hello_iface);
                  //process_hello_msg(sr, packet, interface);
                  printf("\n code after the hello chk \n");
               }else{
                     sr_lsu_proc = sr;
                     packet_lsu_proc = malloc(len + 1);
                     memcpy(packet_lsu_proc,packet,len); 
                     printf(" code idhar pahunch gaya \n");
                     len_lsu_proc = len;
                     memcpy(lsu_iface,interface,5);
                     pthread_create(&t2, NULL, lsu_packet_processing, (void *)lsu_iface);
                     //lsu_packet_processing(sr, packet, len);
               }
               dont_proceed = 1;
            }

            if (dont_proceed != 1){
                
                printf ("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ Code for forwarding @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n");
                nr0 = sr->ospf_subsys->nei_ifaces; 
                rt_walker_fu = sr->routing_table;
                rt_walker_fu2 = sr->routing_table; 

                if_checker3 = sr->if_list;       
                for (lp = 0; lp< 3; lp++){ 
                     if (if_checker3->ip == ip_header->ip_dst.s_addr){
                         this_router3 = 1;
                         toe = 4;
                         sr_send_GW_ICMP_packet(sr, icmp_header, ethernet_header_handle_pak, ip_header, packet, len, interface, toe); 
                         break;
                     }
                     if_checker3 = if_checker3->next;
                }

                if (this_router3 == 0){ 
                    while(rt_walker_fu){
                          dest = rt_walker_fu->dest.s_addr;
                          printf("dest: %s\n", inet_ntoa(*(struct in_addr *)&dest));                    
                          mask_conn_eth = rt_walker_fu->mask.s_addr;
                          
                          //zero = IP2U32(a,a,a,a);
                          if(mask_conn_eth == ZERO_MASK){ 
                             printf("<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<skipping the mask 0 check>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n"); 
                          }else{
                                ipd = ip_to_send_to(ip_header->ip_dst.s_addr, mask_conn_eth); 
                                printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Mask Check done ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n");
                          }
                          printf("ipd: %s\n", inet_ntoa(*(struct in_addr *)&ipd));
                  
                          if(ipd == dest){
                             printf ("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ succesful match done @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ \n"); 
                             match = 1; 
                             printf("match is done \n");
                             //pwospf_lock(sr->ospf_subsys);
                             nr = sr->ospf_subsys->nei_ifaces;
                             while(nr){
                                   
                                   if(rt_walker_fu->gw.s_addr != ZERO_MASK){
                                      agg_ip += 1;
                                      if(nr->filled == 1){
                                         if(nr->neighbour_router->ipa.s_addr == rt_walker_fu->gw.s_addr){
                                            for (int l=0; l< ETHER_ADDR_LEN; l++){
                                                 ethernet_header_handle_pak->ether_shost[l] = nr->mac[l];
                                                 ethernet_header_handle_pak->ether_dhost[l] = nr->neighbour_router->mac[l];  
                                            }
                                            ip_header->ip_ttl = ip_header->ip_ttl - 1;
                                            calc_IP_cs(ip_header); 
                                            sr_send_packet(sr, packet, len, nr->name);
                                            printf ("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ packet gaya re baba @@@@ to: %s \n",inet_ntoa(*(struct in_addr *)&rt_walker_fu->gw.s_addr));
                                            direct_send = 1;
                                            //forward_pak_in_pwospf(sr, packet, ethernet_header_handle_pak, interface, nr, len, ip_header);
                                            //dont_proceed2 = 1;
                                            break;
                                         }
                                       }
                                       if(agg_ip == 3){
                                           store_pak(sr, packet, len);
                                           printf("Caching the ICMP packet for future sending \n");
                                           if (nt[0].ip_addr != ip_header->ip_dst.s_addr){
                                               printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!aggregated ke liye broadcast bhejna padega !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
                                                  
                                               
                                               sr_g = sr;
                                               len_g = len;
                                               gateway_g = ip_header->ip_dst.s_addr;
                                               ip_header_g = ip_header;
                                               printf("ip_header_g at waiting at agg is>>>>>>>>>>>>>>>>>>>>>>>  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_g->ip_dst.s_addr));
                                               packet_g = packet;
                                               ARP_needed_for_iface = interface;
                                               if(thread_started == 0){ //To check if thread is currently running
                                                  pthread_create(&t3, NULL, arp_broadcast, (void *)gateway_g);// ARP broadcasting thread for finding out the server
                                                  thread_started = 1;
                                               }
                                               break;
                                            }else {
                                                   printf(" Directly sending packet as node already known from agg part.....no need for ARP.......\n");
                                                   sr_forward_packet(sr, dest);
                                                   break;
                                            }
                                         }
                                   }else{ /********************** when directly connected on interfaces ***************************/
                                         printf("+++++++++++++++++++++code arrived in gateway 0 check +++++++++++++++++++++++++++++++ \n");
                                         if((nr->addr & nr->mask.s_addr) == dest){
                                            //univ_mask = nr->mask.s_addr;
                                            printf(" ```````````````````````````````` code inside succesfully ```````````````````````````` \n");
                                            if(nr->filled == 1){ 
                                               for (int l=0; l< ETHER_ADDR_LEN; l++){
                                                    ethernet_header_handle_pak->ether_shost[l] = nr->mac[l];
                                                    ethernet_header_handle_pak->ether_dhost[l] = nr->neighbour_router->mac[l];  
                                               }
                                               ip_header->ip_ttl = ip_header->ip_ttl - 1;
                                               calc_IP_cs(ip_header); 
                                               sr_send_packet(sr, packet, len, nr->name);
                                               printf ("$$$$$$$$$$$$$$$$$$$$$$$$$$$$ packet gaya re baba $$$$$$$$$$$$$$$$$$$$$$$$$$$$$ \n");
                                               //forward_pak_in_pwospf(sr, packet, ethernet_header_handle_pak, interface, nr, len, ip_header);
                                               //dont_proceed2 = 1;

                                               break;      
                                            }else{
                                                  store_pak(sr, packet, len);
                                                  printf("Caching the ICMP packet for future sending \n");
                                                  if (nt[0].ip_addr != ip_header->ip_dst.s_addr){
                                                      printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! broadcast bhejna padega !!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
                                                  
                                                     
                                                      sr_g = sr;
                                                      len_g = len;
                                                      //ARP_needed_for_iface = malloc(4);
                                                      gateway_g = ip_header->ip_dst.s_addr;
                                                      ip_header_g = ip_header;
                                                      printf("ip_header_g at waiting is >>>>>>>>>>>>>>>>>>>>>>>>>  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_g->ip_dst.s_addr));
                                                      //gateway0 = ip_header_g->ip_dst.s_addr;
                                                      //ethernet_header_host_unreach_g = ethernet_header_handle_pak;
                                                      packet_g = packet;
                                                      //memcpy(ARP_needed_for_iface,interface,5);
                                                      ARP_needed_for_iface = interface;
                                                      if(thread_started == 0){ //To check if thread is currently running
                                                         pthread_create(&t3, NULL, arp_broadcast, (void *)gateway_g);// ARP broadcasting thread for finding out the server
                                                         thread_started = 1;
                                                      }
                                                      break;
                                                  }else {
                                                         printf(" Directly sending packet as node already known.....no need for ARP.......\n");
                                                         sr_forward_packet(sr, dest);
                                                         break;
                                                  }
                                            }  
                                         }   
                                    }
                                    nr = nr->next;  
                             }
                             //pwospf_unlock(sr->ospf_subsys); 
                         }   
                         rt_walker_fu = rt_walker_fu->next;  
                    }
                    if(match == 0){
                       
                       printf("No routing table matched ???????????????????????????????? so forward on 0????????????????? \n");
                       while(rt_walker_fu2){
                             printf(" ERROR HERE 1 \n");
                             if ((ip_header->ip_dst.s_addr == ZERO_MASK) || (ip_header->ip_dst.s_addr == htonl(OSPF_AllSPFRouters))){
                                  printf("ip_header_g is 000000000000000000000000000000000 or broadcast  %s\n", inet_ntoa(*(struct in_addr *)&ip_header->ip_dst.s_addr));
                                  break;
                             }
                             if(rt_walker_fu2->mask.s_addr == ZERO_MASK){
                                printf(" ERROR HERE 2 \n");
                                //gateway_g = rt_walker_fu2->gw.s_addr;
                                
                                //pwospf_lock(sr->ospf_subsys); 
                                //nr0 = sr->ospf_subsys->nei_ifaces;
                                printf(" ERROR HERE 3 \n");
                                while(nr0){
                                      printf(" ERROR HERE 4 \n");
                                      if(nr0->filled == 1){
                                         printf(" ERROR HERE 5 \n");
                                         if(nr0->neighbour_router->ipa.s_addr == rt_walker_fu2->gw.s_addr){
                                            for (int l=0; l< ETHER_ADDR_LEN; l++){
                                                 ethernet_header_handle_pak->ether_shost[l] = nr0->mac[l];
                                                 ethernet_header_handle_pak->ether_dhost[l] = nr0->neighbour_router->mac[l];  
                                            }
                                            ip_header->ip_ttl = ip_header->ip_ttl - 1;
                                            calc_IP_cs(ip_header); 
                                            sr_send_packet(sr, packet, len, nr0->name);
                                            printf ("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ ZER000000 packet gaya re baba @@@@@@@@@@@@@@@@@@@@@@@@@ \n");
                                            zero_made = 1;
                                            //forward_pak_in_pwospf(sr, packet, ethernet_header_handle_pak, interface, nr, len, ip_header);
                                            //dont_proceed2 = 1;
                                            break;
                                         }
                                      }
                                      nr0 = nr0->next;
                                }
                                if (zero_made == 1){
                                    break;
                                }    else{  gateway_g = rt_walker_fu2->gw.s_addr;
                                            store_pak(sr, packet, len);
                                            printf("Caching the ICMP packet for future sending \n");
                                            if (nt[0].ip_addr != gateway_g){
                                            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! broadcast bhejna padega special jagah se........!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! \n");
                                            store_pak(sr, packet, len);
                                            printf("Caching the ICMP packet for future sending \n");
                                            sr_g = sr;
                                            len_g = len;
                                            //ip_header->ip_dst.s_addr = gateway0;
                                            
                                            ip_header_g = ip_header;
                                            printf("ip_header_g at waiting is >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_g->ip_dst.s_addr));
                                            //ip_header_g->ip_dst.s_addr = gateway0;
                                            
                                            printf("ip_header_g at waiting is >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_g->ip_dst.s_addr));
                                            //ethernet_header_host_unreach_g = ethernet_header_handle_pak;
                                            packet_g = packet;
                                            printf(" error cuz of memory AAA................................... \n");
                                            //ARP_needed_for_iface = malloc(4);
                                            //memcpy(ARP_needed_for_iface,interface,5);
                                            ARP_needed_for_iface = interface;
                                            
                                            if(thread_started == 0){ //To check if thread is currently running
                                               pthread_create(&t1, NULL, arp_broadcast, (void *)gateway_g);// ARP broadcasting thread for finding out the server
                                               thread_started = 1;
                                            }
                                            printf(" error cuz of memory AAA................................... \n");
                                            //zero_made = 0;
                                            break;
                                            }else {
                                                   printf(" Directly sending packet as node already known.....no need for ARP.......\n");
                                                   sr_forward_packet(sr, gateway_g);
                                                   break;
                                            }
                                 }
                                        
                                      //nr0 = nr0->next;
                                //}
                                //pwospf_unlock(sr->ospf_subsys); 
                             } 
                             rt_walker_fu2 = rt_walker_fu2->next; 
                      }
                   }   
                }
            }                                      
        }
     }

     if (ntohs(ethernet_header_handle_pak->ether_type) == ETHERTYPE_ARP){
        arp_handler(sr, len, interface, packet); // To check if the received packet is ARP type
     }
}

void arp_handler(struct sr_instance* sr, unsigned int len, char* interface, uint8_t* packet) {
    struct sr_ethernet_hdr* ethernet_header = 0;
    struct sr_arphdr* header_arp = 0;
    unsigned short opcode_arp;
    uint32_t address;
    int ac, ps;
    struct sr_if * if_walker = 0;


    header_arp = (struct sr_arphdr *) (packet + sizeof (struct sr_ethernet_hdr));
    ethernet_header = (struct sr_ethernet_hdr *) packet;
    opcode_arp = header_arp->ar_op;
    if (opcode_arp == ntohs(ARP_REQUEST)) { //To send ARP reply for a ARP request obtained
       for( ac=0; ac<ETHER_ADDR_LEN ; ac++){ // Creating Ethernet header
           ethernet_header->ether_dhost[ac] = ethernet_header->ether_shost[ac];
       }
       if_walker = sr_get_interface(sr, interface);
       header_arp = construct_arp_header(sr, packet, 2, interface);// Creating ARP header   
       for(ps=0; ps < 6; ps++){
           ethernet_header->ether_shost[ps] = if_walker->addr[ps];
       }
       ethernet_header->ether_type = htons(ETHERTYPE_ARP);
       
       sr_send_packet(sr, packet, len, interface); //Send ARP reply
       printf("ARP Reply sent \n");
    } 

    if (opcode_arp == ntohs(ARP_REPLY)) {// To forward IP packet to server or gateway
    	printf("ARP Reply obtained \n");
        reply_obtained = 1;// To stop the broadcasting thread
        thread_started = 0;// To allow a new broadcast

    	address = header_arp->ar_sip;
        store_node_first_time_ARP(sr, packet, interface);// Storing the server
        sr_forward_packet(sr, address); // Forwarding the packet that has been cached earlier 	
    }
}


//void arp_broadcast (void * iface_new) {    
void arp_broadcast (void * gateway) {  
    int i, ac, len_eth_header, len_arp_header, broadcast_length, tim;
    time_t now;
    double timediff;
    struct sr_ethernet_hdr* ethernet_header = 0;
    struct sr_ethernet_hdr* ethernet_header_host_unreach_g = 0;
    struct sr_arphdr * header_arp = 0;
    struct sr_if * if_walker = 0;
    struct ip * ip_header_here = 0;
    char * iface_new = malloc(4);
    uint32_t gateway_k;
    int sent;

    memcpy(iface_new, ARP_needed_for_iface, 5);
    //iface_new = ARP_needed_for_iface;
    gateway_k = (uint32_t)gateway;
    printf("GATEWAY KKKKKKKK ********* !!!!!!!!!!!!!! **********  %s\n", inet_ntoa(*(struct in_addr *)&gateway_k));
    //ip_header_here = (struct ip *) (packet_g + sizeof (struct sr_ethernet_hdr));
    ip_header_here = ip_header_g;
    printf("Processing broadcast \n");
    len_eth_header = sizeof (struct sr_ethernet_hdr);// Calculating the length of ethernet header
    len_arp_header = sizeof (struct sr_arphdr);// Calculating the length of ARP header
    broadcast_length = len_eth_header + len_arp_header;// Creating the Broadcasting length

    uint8_t * arp_broadcast_p = malloc(broadcast_length);
    ethernet_header = (struct sr_ethernet_hdr *) arp_broadcast_p;
    header_arp = (struct sr_arphdr *) (arp_broadcast_p + sizeof (struct sr_ethernet_hdr));
    ethernet_header->ether_type = ntohs(ETHERTYPE_ARP);
    for (i=0; i< ETHER_ADDR_LEN; i++){
         ethernet_header->ether_dhost[i] = MAX_LEN;
    }
    header_arp = construct_arp_header(sr_g, arp_broadcast_p, 1, iface_new);
    printf("DEST IP HEADER ********* !!!!!!!!!!!!!! *********AAAAAAAA  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_here->ip_dst.s_addr));
    /*if (ip_header_here->ip_dst.s_addr != gateway0){
        printf("DEST IP HEADER ********* !!!!!!!!!!!!!! **************  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_here->ip_dst.s_addr));
        thread_started = 0;
        pthread_exit(NULL);
    }*/
    //if_walker = sr_g->if_list;
    printf("DEST IP HEADER ********* !!!!!!!!!!!!!! **********BBBBBBB  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_here->ip_dst.s_addr));
    for(tim = 0; tim < 5; tim++){ // Broadcast 5 times on an interval of 5 seconds
        if (reply_obtained == 1){ //Break out of the broadcast if reply is received
            reply_obtained = 0;
            pthread_exit(NULL);
            }else{
                 //if_walker = sr_get_interface(sr_g, iface_new);
                 printf("DEST IP HEADER ********* !!!!!!!!!!!!!! ******CCCCCC  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_here->ip_dst.s_addr));
                 if_walker = sr_g->if_list;
                 for (i = 0; i< 3; i++){
                      //if (strcmp(iface_new, if_walker->name)){ //Obtain the correct Interface to broadcast
                          printf("GATEWAY KKKKKKKK ********* !!!!!!!!!!!!!! **********  %s\n", inet_ntoa(*(struct in_addr *)&gateway_k));
                          ip_header_here->ip_dst.s_addr = gateway_k;
                          printf("DEST IP HEADER ********* !!!!!!!!!!!!!! **************  %s\n", inet_ntoa(*(struct in_addr *)&ip_header_here->ip_dst.s_addr));
                          printf("IF WALKER IP ********* !!!!!!!!!!!!!! **************  %s\n", inet_ntoa(*(struct in_addr *)&if_walker->ip));
                          univ_mask = if_walker->mask;
                          if((ip_header_here->ip_dst.s_addr & univ_mask) == (if_walker->ip & univ_mask)){
                             printf(" Code passed final broadcast check {{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{{\n");
                             for(ac=0; ac < ETHER_ADDR_LEN; ac++){
                                 header_arp->ar_sha[ac] = if_walker->addr[ac];
                             }
                             for(ac=0; ac < ETHER_ADDR_LEN; ac++){
                                 ethernet_header->ether_shost[ac] = ((uint8_t)(header_arp->ar_sha[ac]));
                             }
                             header_arp->ar_sip = if_walker->ip;
                             header_arp->ar_tip = ip_header_here->ip_dst.s_addr;
                             sr_send_packet(sr_g, arp_broadcast_p, broadcast_length, if_walker->name);
                             printf("Making a broadcast %d through %s\n", tim, inet_ntoa(*(struct in_addr *)&header_arp->ar_sip));
                             sent = 1;
                             break;
                          }
                       printf("ERROR here 12.................................. \n");//}
                       if_walker = if_walker->next;
                       printf("ERROR here 13.................................. \n");
                  }
                  printf("ERROR here 14.................................. \n");//printf("Making a broadcast %d\n", tim);
                  sleep(1);
             }
    }
    free(arp_broadcast_p);
    printf("ERROR here 15.................................. \n");
    //free(ARP_needed_for_iface);
    
   /*if((tim == 5) && (sent == 1)){ //If 5 ARP request have been sent without any reply, send Host uneachable message
      printf("Host not found \n");
      not_known_server = ip_header_here->ip_dst.s_addr;
      ethernet_header_host_unreach_g = (struct sr_ethernet_hdr *) packet_g; 
    
      for (count = 0; count < 11; count++){
           if(tp[count].ip_addr == ip_header_here->ip_dst.s_addr){
              for (pakcnt = 0; tp[count].pak[pakcnt] != 0; pakcnt++){
                   tp[count].pak[pakcnt] = 0;
              }     
              tp[count].ip_addr = 0;
              tp[count].length = 0;
              printf("Dropping all the cached packets \n");
           }
      }
      toe = 3;
      sr_send_GW_ICMP_packet(sr_g, 0, ethernet_header_host_unreach_g, ip_header_here, packet_g, len_g, iface_new, toe); 
   }*/    
}
         
void store_node_first_time_ARP(struct sr_instance * sr, uint8_t* packet, char * interface){
    
    int flag = 0;
    int ac, count;
    uint32_t ipt_address;
    struct sr_ethernet_hdr * ethernet_header = 0;
    struct sr_arphdr* header_arp = 0;

    ethernet_header = (struct sr_ethernet_hdr *) packet;
    header_arp = (struct sr_arphdr *) (packet + sizeof (struct sr_ethernet_hdr));
    ipt_address = header_arp->ar_sip;	
    printf("intermediate destination at time of storing from ARP: %s\n", inet_ntoa(*(struct in_addr *)&ipt_address));
    for (count = 0; count < 11; count++) {
        if (nt[count].ip_addr == ipt_address) { //If the node is already cached don't do anything
           printf("Server already stored \n");
           flag = 1;
        }
    }
    if (flag != 1) {
        for (count = 0; count < 11; count++) {
             if (nt[count].ip_addr == 0) { // Store the node for the first time
                printf("Storing %d new host who sent ARP \n", count+1);
                nt[count].ip_addr = header_arp->ar_sip;
                nt[count].time_in_q = time(0);
                for (ac = 0; ac < ETHER_ADDR_LEN; ac++) {
                    nt[count].node_adr[ac] = ethernet_header->ether_shost[ac];
                }
                nt[count].interface = sr_get_interface(sr,interface);
                printf("Store successful \n");
                break;
             }
         }
    }     
}

void store_pak(struct sr_instance * sr, uint8_t * packet, unsigned int len){
        
    int count, pakcnt;
    int flag2 = 0;
    uint32_t ip_address;
    struct ip* ip_header = 0;
    uint8_t * temp_packet = malloc(len + 1);
    uint8_t * temp_packet2 = malloc(len + 1);

    ip_header = (struct ip *) (packet + sizeof (struct sr_ethernet_hdr));
    ip_address = ip_header->ip_dst.s_addr;

    for (count = 0; count < 11; count++){
        if(tp[count].ip_addr == ip_header->ip_dst.s_addr){// Storing the packet for an already cached node             
           for (pakcnt = 0; pakcnt < 100; pakcnt++){
               if (tp[count].pak[pakcnt] == 0){
                   memcpy(temp_packet2, packet, len);
                   tp[count].pak[pakcnt] = temp_packet2;
                   break;
               }
           }
           tp[count].length = len;
           flag2 = 1;
           printf("Storing old node %d packet \n", pakcnt+1);
           break;
        }
    }
        
    if (flag2 != 1){           
        for (count = 0; count < 11; count++) {
            if (tp[count].ip_addr == 0) { // To store the first packet from a node
               printf("Storing very first packet from a node \n");
               memcpy(temp_packet, packet, len);
               tp[count].pak[0] = temp_packet;
               //ip_header_chk = (struct ip *) (temp_packet + sizeof (struct sr_ethernet_hdr));
               //printf("Gateway ka address %s\n",inet_ntoa(ip_header_chk->ip_src));
               tp[count].ip_addr = ip_header->ip_dst.s_addr;
               tp[count].length = len;
               break;
            }      
        }
    }
} 

void sr_forward_packet(struct sr_instance * sr, uint32_t rec_addr){

    int j, pakcnt;
    int flag = 0;
    uint16_t length;
    struct sr_ethernet_hdr * ethernet_header = 0;
    struct ip* ip_header = 0;           
          
    length = tp[0].length;
    for (pakcnt = 0; pakcnt < 100; pakcnt++){
         if (tp[0].pak[pakcnt] != 0) { // Check if packet cache actually contain packet
             ip_header = (struct ip *) (tp[0].pak[pakcnt] + sizeof (struct sr_ethernet_hdr));
             ip_header->ip_ttl = ip_header->ip_ttl - 1;
             calc_IP_cs(ip_header);
             ethernet_header = (struct sr_ethernet_hdr *) tp[0].pak[pakcnt];
             for (j = 0; j < ETHER_ADDR_LEN; j++) {
	          ethernet_header->ether_dhost[j] = nt[0].node_adr[j];
		  ethernet_header->ether_shost[j] = nt[0].interface->addr[j];
             }
                               
             sr_send_packet(sr, tp[0].pak[pakcnt], length, nt[0].interface->name);
             printf ("^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ packet gaya re baba ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ \n");
             printf("Forwarding the packet no. %d \n", pakcnt);
             flag = 1;
             printf("error here A \n");
             tp[0].pak[pakcnt] = 0;
             printf("error here B \n");
          }
     }
     if(flag == 0){
        printf("Cancelling the packet forwarding as no packet is cached \n");
     }else{
           printf("Forwarding Complete \n");
           tp[0].length = 0;
           printf("error in fw......................A \n");
     }                  
     tp[0].ip_addr = 0;
     printf("error in fw......................B \n");
}       

void sr_send_GW_ICMP_packet(struct sr_instance* sr, struct sr_icmphdr * icmp_header, struct sr_ethernet_hdr* ethernet_header, struct ip* ip_header, uint8_t * packet, unsigned int length, char* interface, int toe){

    int ac, mac_addr;
    int shift = 4;
    uint8_t * error_packet = (uint8_t *)malloc(sizeof (struct sr_ethernet_hdr) + sizeof (struct ip) + sizeof( struct sr_icmphdr) + (ip_header->ip_hl * shift) + 8);
    uint32_t ip_addr;
    struct sr_if * if_walker = 0;
    struct sr_icmphdr* icmp_header_e;
    struct sr_ethernet_hdr* ethernet_header_e;
    struct ip* ip_header_e;
            
    ethernet_header_e = (struct sr_ethernet_hdr *) error_packet;
    ip_header_e = (struct ip *) (error_packet + sizeof (struct sr_ethernet_hdr));
    icmp_header_e = ((struct sr_icmphdr*)(error_packet + sizeof(struct sr_ethernet_hdr) + sizeof(struct ip))); 
            
    for (ac = 0; ac< ETHER_ADDR_LEN; ac++) { //Reversing the source and destination address for ethernet header
         mac_addr = ethernet_header->ether_dhost[ac];
         ethernet_header->ether_dhost[ac] = ethernet_header->ether_shost[ac];
         ethernet_header->ether_shost[ac] = mac_addr;
    }
    ethernet_header->ether_type = htons(ETHERTYPE_IP);         

    if (toe == 1){ //Sending the ICMP error reply for time exceeded
        memcpy(ethernet_header_e, ethernet_header, sizeof(struct sr_ethernet_hdr));      
        
        if_walker = sr_get_interface(sr, interface);
        ip_header_e->ip_src.s_addr = if_walker->ip;  
        ip_header_e->ip_dst.s_addr = ip_header->ip_src.s_addr;   
     	ip_header_e->ip_v = ip_header->ip_v;
        ip_header_e->ip_ttl = 64;
        ip_header_e->ip_hl = 5;
        ip_header_e->ip_p = IPPROTO_ICMP;
        ip_header_e->ip_off = ntohs(IP_DF);
        ip_header_e->ip_tos = 0;
        ip_header_e->ip_id = ip_header->ip_id;
        ip_header_e->ip_len = ntohs(20 + 8 + (ip_header->ip_hl * shift) + 8);// Length of ip part is 56
              
        icmp_header_e->c_type = TYPE_TIME_EXCEEDED_ICMP;
        icmp_header_e->c_code = CODE_TTL_EXP_ICMP;
        icmp_header_e->c_id = 0;
        icmp_header_e->c_seqn = 0;

        memcpy(error_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip) + sizeof( struct sr_icmphdr), ip_header, (ip_header->ip_hl * shift) + 8);
        calc_IP_cs(ip_header_e);
        calc_ICMP_cs(icmp_header_e,error_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), 8 + (ip_header->ip_hl * shift) + 8);
               
        sr_send_packet(sr, error_packet, sizeof (struct sr_ethernet_hdr) + sizeof (struct ip) + sizeof( struct sr_icmphdr) + (ip_header->ip_hl * shift) + 8, interface);
        printf("Sending the ICMP error packet for time exceeded \n");
    }

    if (toe == 2){ //Sending the ICMP error reply for port unreachable
        memcpy(ethernet_header_e, ethernet_header, sizeof(struct sr_ethernet_hdr));
             
        ip_header_e->ip_src.s_addr = ip_header->ip_dst.s_addr;
        ip_header_e->ip_dst.s_addr = ip_header->ip_src.s_addr;   
        ip_header_e->ip_v = ip_header->ip_v;
        ip_header_e->ip_ttl = 64;
        ip_header_e->ip_hl = 5;
        ip_header_e->ip_p = IPPROTO_ICMP;
        ip_header_e->ip_off = ntohs(IP_DF);
        ip_header_e->ip_tos = 0;
        ip_header_e->ip_id = ip_header->ip_id;
        ip_header_e->ip_len = ntohs(20 + 8 + (ip_header->ip_hl * shift) + 8);              

        icmp_header_e->c_type = TYPE_DESTINATION_UNREACHABLE_ICMP;
        icmp_header_e->c_code = CODE_DEST_PORT_UNREACH_ICMP;
        icmp_header_e->c_id = 0;
        icmp_header_e->c_seqn = 0;
        
        memcpy(error_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip) + sizeof( struct sr_icmphdr), ip_header, (ip_header->ip_hl * shift) + 8);
        calc_IP_cs(ip_header_e);
        calc_ICMP_cs(icmp_header_e,error_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), 8 + (ip_header->ip_hl * shift) + 8);
               
        sr_send_packet(sr, error_packet, sizeof (struct sr_ethernet_hdr) + sizeof (struct ip) + sizeof( struct sr_icmphdr) + (ip_header->ip_hl * shift) + 8, interface);
        printf("Sending the ICMP error packet for port unreachable \n");
    }

    if (toe == 3){ //Sending the ICMP error reply for host unreachable
        memcpy(ethernet_header_e, ethernet_header, sizeof(struct sr_ethernet_hdr));

        ip_header_e->ip_src.s_addr = ip_header->ip_dst.s_addr;   
        ip_header_e->ip_dst.s_addr = ip_header->ip_src.s_addr;   
        ip_header_e->ip_v = ip_header->ip_v;
        ip_header_e->ip_ttl = 64;
        ip_header_e->ip_hl = 5;
        ip_header_e->ip_p = IPPROTO_ICMP;
        ip_header_e->ip_off = ntohs(IP_DF);
        ip_header_e->ip_tos = 0;
        ip_header_e->ip_id = ip_header->ip_id;
        ip_header_e->ip_len = ntohs(20 + 8 + (ip_header->ip_hl * shift) + 8);      

        icmp_header_e->c_type = TYPE_DESTINATION_UNREACHABLE_ICMP;
        icmp_header_e->c_code = CODE_DEST_HOST_UNREACH_ICMP;
        icmp_header_e->c_id = 0;
        icmp_header_e->c_seqn = 0;
        
        memcpy(error_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip) + sizeof( struct sr_icmphdr), ip_header, (ip_header->ip_hl * shift) + 8);
        calc_IP_cs(ip_header_e);
        calc_ICMP_cs(icmp_header_e,error_packet + sizeof(struct sr_ethernet_hdr) + sizeof( struct ip), 8 + (ip_header->ip_hl * shift) + 8);
               
        sr_send_packet(sr, error_packet, sizeof (struct sr_ethernet_hdr) + sizeof (struct ip) + sizeof( struct sr_icmphdr) + (ip_header->ip_hl * shift) + 8, interface);
        reply_obtained = 0;
        thread_started = 0;
        printf("Sending the ICMP error packet for host unreachable \n");
    }

    if (toe == 4){ // Sending the ICMP reply for request sent from Gateway directly to router
        
        ip_addr = ip_header->ip_src.s_addr;
        ip_header->ip_src.s_addr = ip_header->ip_dst.s_addr;
        ip_header->ip_dst.s_addr = ip_addr;

        ip_header->ip_ttl -= 1;
        calc_IP_cs(ip_header);

        icmp_header->c_type = TYPE_ECHO_REPLY_ICMP;
        icmp_header->c_code = CODE_ECHO_REPLY_ICMP;
       
        calc_ICMP_cs(icmp_header, packet + sizeof (struct sr_ethernet_hdr) + sizeof (struct ip), length - sizeof (struct sr_ethernet_hdr) - sizeof (struct ip));
        sr_send_packet(sr,packet,length,interface);
        printf("ICMP reply sent back to Gateway for its ICMP request to router \n");
    }
}

void calc_IP_cs(struct ip* ip_header){ // Calculating IP checksum
 
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

void calc_ICMP_cs(struct sr_icmphdr* icmphdr, uint8_t * packet, int len) { // Calculating ICMP checksum
    
    uint32_t total_sum = 0;
    int count;
    icmphdr->c_cs = 0;

    uint16_t* hdr_temp = (uint16_t *) packet;

    for (count = 0; count < len / 2; count++) {
         total_sum = total_sum + hdr_temp[count];
    }
    total_sum = (total_sum >> 16) + (total_sum & 0xFFFF);
    total_sum = total_sum + (total_sum >> 16);
    icmphdr->c_cs = ~total_sum;
}


struct sr_arphdr* construct_arp_header(struct sr_instance * sr, uint8_t* packet, int type, char * interface){

    int ac;    
    uint32_t temp_sip, temp_tip;
    struct sr_arphdr* header_arp = 0;
    
    if (type == 1){ // Constructing ARP for request
        header_arp = (struct sr_arphdr *) (packet + sizeof (struct sr_ethernet_hdr));
        header_arp->ar_hln = HARDWARE_ADR_LEN;
        header_arp->ar_pln = PROTOCOL_ADR_LEN;
        header_arp->ar_op = ntohs(ARP_REQUEST);
        header_arp->ar_hrd = ntohs(ARPHDR_ETHER);
        header_arp->ar_pro = ntohs(ETHERTYPE_IP);
        return(header_arp);
    }

    if (type == 2){ // Constructing ARP for reply
        // Interchanging sender and target IP Addresses
        header_arp = (struct sr_arphdr *) (packet + sizeof (struct sr_ethernet_hdr));
        temp_tip = header_arp->ar_tip;
        temp_sip = header_arp->ar_sip;
        header_arp->ar_tip = temp_sip;
        header_arp->ar_sip = temp_tip;            

        // Constructing ARP header part
        header_arp->ar_hrd = htons(ARPHDR_ETHER);
        header_arp->ar_pro = htons(ETHERTYPE_IP);
        header_arp->ar_op = htons(ARP_REPLY);
        header_arp->ar_hln = HARDWARE_ADR_LEN;
        header_arp->ar_pln = PROTOCOL_ADR_LEN; 
        for (ac = 0; ac < ETHER_ADDR_LEN; ac++){
             header_arp->ar_tha[ac] = header_arp->ar_sha[ac];
        }
        memcpy(header_arp->ar_sha, sr_get_interface(sr,interface)->addr, 6);
        return(header_arp);            
    }           
}

/*uint32_t IP2U32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  uint32_t ip = a;
  ip <<= 8;
  ip |= b;
  ip <<= 8;
  ip |= c;
  ip <<= 8;
  ip |= d;

  return ip;
}

uint32_t broadcast(uint32_t ip, uint32_t subnet){
    uint32_t bits = subnet ^ 0xffffffff; 
    uint32_t bcast = ip | bits;

    return bcast;
}*/

uint32_t ip_to_send_to(uint32_t ip, uint32_t subnet){ 
    uint32_t ipd;
    ipd = ip & subnet;    
    return ipd;
}

/*--------------------------------------------------------------------- 
 * Method:
 *

 *---------------------------------------------------------------------*/
