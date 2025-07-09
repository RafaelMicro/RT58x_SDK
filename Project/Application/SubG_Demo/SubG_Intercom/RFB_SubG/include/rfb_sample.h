/**
 * @file rfb_sample.h
 * @author
 * @date
 * @brief Brief single line description use for indexing
 *
 * More detailed description can go here
 *
 *
 * @see http://
 */
#ifndef _RFB_SAMPLE_H_
#define _RFB_SAMPLE_H_
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "radio.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

/**************************************************************************************************
*    TYPEDEFS
*************************************************************************************************/
typedef uint8_t RFB_PCI_TEST_CASE;
#define RFB_PCI_AUDIO_RELAY_TEST         ((RFB_PCI_TEST_CASE)0x01)
#define RFB_PCI_AUDIO_FIELD_TEST         ((RFB_PCI_TEST_CASE)0x02)
/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
void rfb_sample_init(uint8_t RfbPciTestCase);
void rfb_sample_entry(uint8_t RfbPciTestCase);
#endif

