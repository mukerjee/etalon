/* the counters in MCP memory are pointed to by mcp_gen_header.counters_addr
   the structure has the following layout:
   uint32_t nb_counters;  // each MCP can have a different subset of all known counters
   uint32_t counters[nb_counters];  // the counters value
   uint16_t counters_ids[nb_counters]; // identification of each counter in the array,
				       // using counter id values from the list below

  this file is designed to be included several times to be able to iterate several times over ZCNT_DO 
  with different actions, see examples at the end for usage
*/   

#ifndef _ze_counters_h
#define _ze_counters_h

#define ZCNT_NUMS 34

/* defines the preferred representation for each "counter" var */
#define ZCNT_INT 0
#define ZCNT_HEXA 1

#endif /* ze_counters_h */

#ifdef ZCNT_DO

ZCNT_DO(p0_rx_bad_crc8, 1, ZCNT_INT)
ZCNT_DO(p0_rx_good_crc8, 2, ZCNT_INT)
ZCNT_DO(pcie_pad_overflow, 3, ZCNT_INT)
ZCNT_DO(pcie_bad_tlp, 4, ZCNT_INT)
ZCNT_DO(pcie_send_timeout, 5, ZCNT_INT)
ZCNT_DO(pcie_send_nacked, 6, ZCNT_INT)
ZCNT_DO(pcie_link_error, 7, ZCNT_INT)
ZCNT_DO(eeprom_id, 8, ZCNT_HEXA)
ZCNT_DO(msix_interrupt_masked, 9, ZCNT_INT)
ZCNT_DO(pcie_tx_stopped, 10, ZCNT_INT)
ZCNT_DO(ext_pio_read, 11, ZCNT_INT)
ZCNT_DO(partial_ext_write, 12, ZCNT_INT)
ZCNT_DO(ltssm_lane_resync, 13, ZCNT_INT)
ZCNT_DO(ltssm_misc, 14, ZCNT_HEXA)
ZCNT_DO(ltssm_rxdetect, 15, ZCNT_HEXA)
ZCNT_DO(pcie_fc_no_dllp, 16, ZCNT_HEXA)
ZCNT_DO(pcie_sw_naks, 17, ZCNT_INT)
ZCNT_DO(pcie_sw_naks_data_pending, 18, ZCNT_INT)
ZCNT_DO(pcie_sw_naks_gotdata, 19, ZCNT_INT)
ZCNT_DO(pcie_pretimeout0, 20, ZCNT_INT)
ZCNT_DO(pcie_pretimeout1, 21, ZCNT_INT)
ZCNT_DO(p0_unaligned, 22, ZCNT_INT)
ZCNT_DO(pcie_not_in_l0, 23, ZCNT_INT)
ZCNT_DO(pcie_lane_invalid, 24, ZCNT_INT)
ZCNT_DO(pcie_tlp_dup, 25, ZCNT_INT)
ZCNT_DO(pcie_tlp_dup_pad, 26, ZCNT_INT)
ZCNT_DO(mean_gate_delay, 27, ZCNT_INT)
ZCNT_DO(mean_gate_delay_low, 28, ZCNT_INT)
ZCNT_DO(mean_gate_delay_high, 29, ZCNT_INT)
ZCNT_DO(pcie_invalid_be, 30, ZCNT_INT)
ZCNT_DO(p0_lane_invalid, 31, ZCNT_INT)
ZCNT_DO(p1_lane_invalid, 32, ZCNT_INT)
ZCNT_DO(p0_e_received, 33, ZCNT_INT)
ZCNT_DO(p1_e_received, 34, ZCNT_INT)
     
#endif

#undef ZCNT_DO


/* Example on how this file can be used:
    1) simple mcp that just want to record one counter:
     #define ZCNT_DO(name, id, type) static const int zcntid_##name = id;
     #include "ze_counters"

     struct { uint32_t cnt; uint32_t pcie_bad_tlp; uint16_t id; } = {1,0,zcntid_pcie_bad_tlp};

   2) mcp that includes all known counters:
     struct {
       uint32_t cnt;
#define ZCNT_DO(name, id, type) uint32_t name;
#include "ze_counters.h"
       uint16_t ids[ZCNT_NUMS];
     } = { .cnt = ZCNT_NUMS;
           .ids = {
#include ZCNT_DO(name, id, type) id,
#include "ze_counters"
            }
         };


   3) host program that want to print the counters of a given mcp:
      [[x]] denotes accessing memory in the lanai ram at offset x
     char *zcnt_name[ZCNT_NUMS+2] = { "invalid",
#define ZCNT_DO(name, id, type) #name,
#include "ze_counters"
     NULL};
  
     counter_ptr = [[mcp_gen_header->counters_addr]]
     nb_cnt = [[counter_ptr]];
     ids_ptr = counter_ptr + 4 + nb_cnt * 4;
     for (i=0;i<nb_cnt;i++) {
       cnt_id = [[ids_ptr + 2 * i]]
       printf("%s=0x%x\n", zcnt_name[cnt_id], [[counter_ptr + 4 + 4 * i]]);

*/

     
     
