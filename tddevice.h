#pragma once


#define TDDEV1_CMD_INIT					0x82

// TDDEV2 Std. inpacket
#define INPACKET_DEVREG					0xF3
#define INPACKET_DUMP					0xF6
#define INPACKET_ACK					0xFF

// TDDEV2 Std. outpacket
#define OUTPACKET_SET					0xF0
#define OUTPACKET_SAVE					0xF1
#define OUTPACKET_GET					0xF2
#define OUTPACKET_ERASE					0xF6


// Wait up to DEFAULT_TIMEOUT for a report matching expected (excluding the report ID).
// Unrelated reports are discarded without extending the deadline.
int tddev2_wait_response(td_context_t* context, uint8_t* report, const uint8_t* expected, int expected_len);

int tddev1_init_operation(td_context_t* context);

int tddev2_write_devreg(td_context_t* context, uint16_t addr, uint32_t value);
int tddev2_destroy_firmware(td_context_t* context);
int tddev2_save_to_flash(td_context_t* context);
uint32_t tddev2_read_devreg(td_context_t* context, uint16_t addr);
