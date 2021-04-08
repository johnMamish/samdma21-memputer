/**
 * This file contains utility functions for making samdma programs.
 *
 * It primarily consists of 2 types of functions:
 *     1. functions that setup LUTs
 *     2. functions that setup chains of DMA descriptors to implement instructions
 *
 * All of the functions in this file can be run at "compile time", but for ease of implementation
 * right now, we're running them on the microcontroller.
 */

#include "dmainstrs.h"

////////////////////////////////////////////////////////////////////////////////
// LUT building functions

/**
 * What are some basic LUT operations that we want to implement?
 *
 *   * Nybble manipulation utility LUTs
 *     I think that an important part of this system - to save memory - will be the breaking of
 *     32-bit operations up into 4-bit operations. Using these functions, you can
 *       * low_nybble_low_nybble_to_byte
 *         16x256 LUT that maps 0000_yyyy X zzzz_xxxx to yyyy_xxxx. Used for combining 2 nybbles
 *         into a single byte so that they can be used to index into 16x16 tables.
 *         Because the DMA can concatenate bytes but not nybbles, no matter what we do, a table to
 *         combine 2 nybbles will always take at least 16 x 256 entries.
 *       * low_nybble_to_low_nybble
 *         256x1 LUT that maps yyyy_xxxx to 0000_xxxx
 *       * low_nybble_to_high_nybble
 *         256x1 LUT that maps yyyy_xxxx to xxxx_0000
 *       * high_nybble_to_high_nybble
 *         256x1 LUT that maps yyyy_xxxx to yyyy_0000
 *       * high_nybble_to_low_nybble
 *         256x1 LUT that maps yyyy_xxxx to 0000_yyyy
 *
 *    * Arithmeic LUTs
 *       * nybble_add_no_carryin
 *         16x16 LUT that maps xxxx_yyyy to 4-bit result xxxx + yyyy (without carry bit). Note that
 *         we could optionally choose to have the carry bit in bit 4 and then have another LUT that
 *         masks out bit 4.
 *       * nybble_add_with_carryin
 *         16x16 LUT that maps xxxx_yyyy to 4-bit result 1 + xxxx + yyyy (without carry bit). Note that
 *         we could optionally choose to have the carry bit in bit 4 and then have another LUT that
 *         masks out bit 4.
 *       * nybble_carryout_no_carryin
 *         16x16 LUT that maps xxxx_yyyy to 1-bit result that's the carry bit of xxxx + yyyy.
 *       * nybble_carryout_no_carryin
 *         16x16 LUT that maps xxxx_yyyy to 1-bit result that's the carry bit of 1 + xxxx + yyyy.
 *
 *       * nybble_compare_equal
 *         16x16 LUT that maps xxxx_yyyy to one 8-bit value if xxxx == yyyy and a different 8-bit
 *         value if xxxx != yyyy.
 *
 *       * nybble_
 *
 * We can build up larger operations from microcoded LUT lookups.
 */

/**
 * 16x256 table.
 *
 * table[0b0000_xxxx][yyyy_zzzz] maps to yyyy_xxxxx
 *
 * Table looks like
 * {0b0000_0000, 0b0000_0000, .... , 0b0000_0000,
 *  0b0001_0000, 0b0001_0000, .... , 0b0001_0000,
 *  ...
 *  0b1111_1110, 0b1111_1110, .... , 0b1111_1110,
 *  0b1111_1111, 0b1111_1111, .... , 0b1111_1111}
 */
void setup_low_nybble_low_nybble_to_byte(uint8_t* base)
{
    for (uint32_t high_nybble = 0; high_nybble < 16; high_nybble++) {
        for (uint32_t low_nybble = 0; low_nybble < 16; low_nybble++) {
            for (uint32_t dup_count = 0; dup_count < 16; dup_count++) {
                int idx = (dup_count * 16) + (low_nybble * 1) + (high_nybble * 256);
                uint8_t target_value = (uint8_t)((high_nybble << 4) | (low_nybble));
                base[idx] = target_value;
            }
        }
    }
}

/**
 * 1x256 table.
 * table[0byyyy_xxxx] maps to 0000_xxxx
 */
void setup_low_nybble_to_low_nybble(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) { base[count] = (count << 0) & 0x0f; }
}

/**
 * 1x256 table.
 * table[0byyyy_xxxx] maps to xxxx_0000
 */
void setup_low_nybble_to_high_nybble(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) { base[count] = (count << 4) & 0xf0; }
}

/**
 * 1x256 table.
 * table[0byyyy_xxxx] maps to yyyy_0000
 */
void setup_high_nybble_to_high_nybble(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) { base[count] = (count << 0) & 0xf0; }
}

/**
 * 1x256 table.
 * table[0byyyy_xxxx] maps to 0000_yyyy
 */
void setup_high_nybble_to_low_nybble(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) { base[count] = (count >> 4) & 0x0f; }
}

/**
 * 16x16 table.
 * table[0byyyy_xxxx] maps to xxxx + yyyy without carry bit (bits 7:4 all set to 0).
 */
void setup_nybble_add_no_carryin(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) {
        base[count] = (((count >> 4) & 0x0f) + (count & 0x0f)) & 0x0f;
    }
}

/**
 * 16x16 table.
 * table[0byyyy_xxxx] maps to 1 + xxxx + yyyy without carry bit (bits 7:4 all set to 0).
 */
void setup_nybble_add_with_carryin(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) {
        base[count] = (((count >> 4) & 0x0f) + (count & 0x0f) + 1) & 0x0f;
    }
}

/**
 * 16x16 table.
 */
void setup_nybble_add_no_carryin(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) {
        base[count] = ((((count >> 4) & 0x0f) + (count & 0x0f)) & 0x10) >> 4;
    }
}

/**
 * 16x16 table.
 */
void setup_nybble_add_with_carryin(uint8_t* base)
{
    for (uint8_t count = 0; count < 256; count++) {
        base[count] = ((((count >> 4) & 0x0f) + (count & 0x0f) + 1) & 0x10) >> 4;
    }
}

/**
 * 16x16 table.
 * table[0byyyy_xxxx] maps to 'a' if xxxx == yyyy, else maps to 'b'.
 */
void setup_nybble_compare_equal(uint8_t* base, uint8_t a, uint8_t b)
{
    for (uint8_t count = 0; count < 256; count++) {
        base[count] = ((count & 0x0f) == ((count >> 4) & 0x0f)) ? a : b;
    }
}

/**
 * Builds a 65,536 entry table that holds the results of additions.
 *
 * base[a][b] will contain the result a + b.
 *
 * Issue when it comes to dealing with carry: we need a total of 4 tables:
 *   * sum table when carry-in is 0
 *   * sum table when carry-in is 1
 *   * carry-out table when carry-in is 0
 *   * carry-out table when carry-in is 1.
 *
 * One quick way that we can get around this: make carry-out table bit-packed; use another LUT to
 * decode individual bits.
 *
 * Question: because a + b == b + a, can we collapse the add8 LUT with and without carry into
 * an upper- and lower- triangular LUT? We would need a way to find the max of a and b...
 */
void build_lut8_add(uint8_t* base)
{
    // unimplemented
    while(1);
}


/**
 * Setup a chain of dma ucode instructions to add 2 8-bit memory locations.
 * Once the first DMA transfer starts executing, the result location will contain the result
 * after the DMA transfer is complete
 *
 * descs is the target location where the 128 Bytes(?) worth of DMA descriptors will be dumped.
 */
void build_add8_using_nybbles(DmacDescriptor* descs,
                              uint8_t* opa,
                              uint8_t* opb,
                              uint8_t* result)
{
    // default
    const uint16_t default_btctrl = ((0 << 13) |  // addr increment long step size: don't care
                                     (0 << 12) |  // src/dest select for addr inc step: don't care
                                     (0 << 11) |  // dest increment: disable
                                     (0 << 10) |  // src  increment: disable
                                     (0 <<  8) |  // beat size: byte
                                     (0 <<  3) |  // action on block xfer complete: none
                                     (0 <<  1) |  // no event on xfer complt
                                     (1 <<  0));  // descriptor valid

    static uint8_t scratch[4];

    ////////////////////////////////////////
    // do a single 4-bit add, storing the result in byte nibs_result[0] and carry_result
    // need to perform an index low_nybble_to_low_nybble[*opa] --> 'nibs_a'
    descs[0].BTCTRL = default_btctrl;
    descs[0].BTCNT  = 1;
    descs[0].SRCADDR = *opa;          // <<< watch out for this one... need another xfer to set ths up.
    //descs[0].DSTADDR = &nibs_a;
    descs[0].DSTADDR = &descs[2].SRCADDR;
    descs[0].DESCADDR = &descs[1];

    // need to perform an index low_nybble_to_low_nybble[*opb] --> 'nibs_b'
    descs[1].BTCTRL = default_btctrl;
    descs[1].BTCNT  = 1;
    descs[1].SRCADDR = *opb;          // <<< need another xfer to set this one up.
    descs[1].DSTADDR = &descs[2].SRCADDR + 1;
    descs[1].DESCADDR = &descs[2];

    // need to perform an index low_nybble_low_nybble_to_byte[nibs_a][nibs_b] --> nibs_a_b.
    // SRCADDR field is filled out by previous xfers. Assumes that low_nybble_low_nybble_to_byte is
    // located on a 64k boundary.
    descs[2].BTCTRL = default_btctrl;
    descs[2].BTCNT  = 2;
    //descs[2].SRCADDR = ();
    descs[2].DSTADDR = &scratch[2];
    descs[2].DESCADDR = &descs[2];


    // need to perform an index nybble_add_no_carryin[nibs_a_b] --> nibs_result[0]


    // need to perform an index nybble_carryout_no_carryin[nibs_a_b] --> carry_result


    ////////////////////////////////////////
    // do a single 4-bit add, storing the result in byte nibs_result[1] and carry_result
    // need to perform an index low_nybble_to_low_nybble[*opa] --> 'nibs_a'


    // need to perform an index low_nybble_to_low_nybble[*opb] --> 'nibs_b'


    // need to perform an index low_nybble_low_nybble_to_byte[nibs_a][nibs_b] --> nibs_a_b.


    // need to perform an index nybble_add[carry_result][nibs_a_b] --> nibs_result[1]


    // need to perform an index nybble_carryout[carry_result][nibs_a_b] --> carry_result


    ////////////////////////////////////////
    // combine nibs_result[0] and nibs_result[1].
    // low_nybble_low_nybble_to_byte[nibs_result[1]][nibs_result[0]] --> *result


}


void nor()
{
// nor

}
    // lw

    // sw

    // beq

    // jalr
