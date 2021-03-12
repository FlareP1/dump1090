// Part of dump1090, a Mode S message decoder for RTLSDR devices.
//
// demod_2400.c: 2.4MHz Mode S demodulator.
//
// Copyright (c) 2014,2015 Oliver Jowett <oliver@mutability.co.uk>
//
// This file is free software: you may copy, redistribute and/or modify it  
// under the terms of the GNU General Public License as published by the
// Free Software Foundation, either version 2 of the License, or (at your  
// option) any later version.  
//
// This file is distributed in the hope that it will be useful, but  
// WITHOUT ANY WARRANTY; without even the implied warranty of  
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU  
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License  
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "dump1090.h"

// 2.4MHz sampling rate version
//
// When sampling at 2.4MHz we have exactly 6 samples per 5 symbols.
// Each symbol is 500ns wide, each sample is 416.7ns wide
//
// We maintain a phase offset that is expressed in units of 1/5 of a sample i.e. 1/6 of a symbol, 83.333ns
// Each symbol we process advances the phase offset by 6 i.e. 6/5 of a sample, 500ns
//
// The correlation functions below correlate a 1-0 pair of symbols (i.e. manchester encoded 1 bit)
// starting at the given sample, and assuming that the symbol starts at a fixed 0-5 phase offset within
// m[0]. They return a correlation value, generally interpreted as >0 = 1 bit, <0 = 0 bit

// TODO check if there are better (or more balanced) correlation functions to use here

// nb: the correlation functions sum to zero, so we do not need to adjust for the DC offset in the input signal
// (adding any constant value to all of m[0..3] does not change the result)

#if 0
static inline int slice_phase0(uint16_t *m) {
    return 5 * m[0] - 3 * m[1] - 2 * m[2];
}
static inline int slice_phase1(uint16_t *m) {
    return 4 * m[0] - m[1] - 3 * m[2];
}
static inline int slice_phase2(uint16_t *m) {
    return 3 * m[0] + m[1] - 4 * m[2];
}
static inline int slice_phase3(uint16_t *m) {
    return 2 * m[0] + 3 * m[1] - 5 * m[2];
}
static inline int slice_phase4(uint16_t *m) {
    return m[0] + 5 * m[1] - 5 * m[2] - m[3];
}
#endif

static inline int slice_phase0(uint16_t *m) {
    return m[0] + 2*m[1] + 2*m[2] + m[3] - m[4] - 2*m[5] - 2*m[6] -m[7];
}
static inline int slice_phase1(uint16_t *m) {
    return -m[0] + m[1] + 2*m[2] + 2*m[3] + m[4] - m[5] - 2*m[6] - 2*m[7];
}
static inline int slice_phase2(uint16_t *m) {
    return -2*m[0] - m[1] + m[2] + 2*m[3] + 2*m[4] + m[5] - m[6] - 2*m[7];
}
static inline int slice_phase3(uint16_t *m) {
    return -2*m[0] - 2*m[1] - m[2] + m[3] + 2*m[4] + 2*m[5] + m[6] - m[7];
}
static inline int slice_phase4(uint16_t *m) {
    return -m[0] - 2*m[1] - 2*m[2] - m[3] + m[4] + 2*m[5] + 2*m[6] + m[7];
}
static inline int slice_phase5(uint16_t *m) {
    return m[0] - m[1] - 2*m[2] - 2*m[3] - m[4] + m[5] + 2*m[6] + 2*m[7];
}
static inline int slice_phase6(uint16_t *m) {
    return 2*m[0] + m[1] - m[2] - 2*m[3] - 2*m[4] - m[5] + m[6] + 2*m[7];
}
static inline int slice_phase7(uint16_t *m) {
    return 2*m[0] + 2*m[1] + m[2] - m[3] - 2*m[4] - 2*m[5] - m[6] + m[7];
}



static inline int correlate_phase0(uint16_t *m) {
    return slice_phase0(m) * 20;
}
static inline int correlate_phase1(uint16_t *m) {
    return slice_phase1(m) * 20;
}
static inline int correlate_phase2(uint16_t *m) {
    return slice_phase2(m) * 20;
}
static inline int correlate_phase3(uint16_t *m) {
    return slice_phase3(m) * 20;
}
static inline int correlate_phase4(uint16_t *m) {
    return slice_phase4(m) * 20;
}
static inline int correlate_phase5(uint16_t *m) {
    return slice_phase5(m) * 20;
}
static inline int correlate_phase6(uint16_t *m) {
    return slice_phase6(m) * 20;
}
static inline int correlate_phase7(uint16_t *m) {
    return slice_phase7(m) * 20;
}

//
// These functions work out the correlation quality for the 10 symbols (5 bits) starting at m[0] + given phase offset.
// This is used to find the right phase offset to use for decoding.
//

static inline int correlate_check_0(uint16_t *m) {
    return
        //abs(correlate_phase0(&m[0])) +
        //abs(correlate_phase2(&m[2])) +
        //abs(correlate_phase4(&m[4])) +
        //abs(correlate_phase1(&m[7])) +
        //abs(correlate_phase3(&m[9]));

        abs(correlate_phase0(&m[0])) +
        abs(correlate_phase0(&m[8])) +
        abs(correlate_phase0(&m[16])) +
        abs(correlate_phase0(&m[24])) +
        abs(correlate_phase0(&m[32]));
}

static inline int correlate_check_1(uint16_t *m) {
    return
        //abs(correlate_phase1(&m[0])) +
        //abs(correlate_phase3(&m[2])) +
        //abs(correlate_phase0(&m[5])) +
        //abs(correlate_phase2(&m[7])) +
        //abs(correlate_phase4(&m[9]));

        abs(correlate_phase1(&m[0])) +
        abs(correlate_phase1(&m[8])) +
        abs(correlate_phase1(&m[16])) +
        abs(correlate_phase1(&m[24])) +
        abs(correlate_phase1(&m[32]));
}

static inline int correlate_check_2(uint16_t *m) {
    return
        //abs(correlate_phase2(&m[0])) +
        //abs(correlate_phase4(&m[2])) +
        //abs(correlate_phase1(&m[5])) +
        //abs(correlate_phase3(&m[7])) +
        //abs(correlate_phase0(&m[10]));

        abs(correlate_phase2(&m[0])) +
        abs(correlate_phase2(&m[8])) +
        abs(correlate_phase2(&m[16])) +
        abs(correlate_phase2(&m[24])) +
        abs(correlate_phase2(&m[32]));
}

static inline int correlate_check_3(uint16_t *m) {
    return
        //abs(correlate_phase3(&m[0])) +
        //abs(correlate_phase0(&m[3])) +
        //abs(correlate_phase2(&m[5])) +
        //abs(correlate_phase4(&m[7])) +
        //abs(correlate_phase1(&m[10]));

        abs(correlate_phase3(&m[0])) +
        abs(correlate_phase3(&m[8])) +
        abs(correlate_phase3(&m[16])) +
        abs(correlate_phase3(&m[24])) +
        abs(correlate_phase3(&m[32]));
}

static inline int correlate_check_4(uint16_t *m) {
    return
        //abs(correlate_phase4(&m[0])) +
        //abs(correlate_phase1(&m[3])) +
        //abs(correlate_phase3(&m[5])) +
        //abs(correlate_phase0(&m[8])) +
        //abs(correlate_phase2(&m[10]));

        abs(correlate_phase4(&m[0])) +
        abs(correlate_phase4(&m[8])) +
        abs(correlate_phase4(&m[16])) +
        abs(correlate_phase4(&m[24])) +
        abs(correlate_phase4(&m[32]));
}

static inline int correlate_check_5(uint16_t *m) {
    return
        //abs(correlate_phase4(&m[0])) +
        //abs(correlate_phase1(&m[3])) +
        //abs(correlate_phase3(&m[5])) +
        //abs(correlate_phase0(&m[8])) +
        //abs(correlate_phase2(&m[10]));

        abs(correlate_phase5(&m[0])) +
        abs(correlate_phase5(&m[8])) +
        abs(correlate_phase5(&m[16])) +
        abs(correlate_phase5(&m[24])) +
        abs(correlate_phase5(&m[32]));
}

static inline int correlate_check_6(uint16_t *m) {
    return
        //abs(correlate_phase4(&m[0])) +
        //abs(correlate_phase1(&m[3])) +
        //abs(correlate_phase3(&m[5])) +
        //abs(correlate_phase0(&m[8])) +
        //abs(correlate_phase2(&m[10]));

        abs(correlate_phase6(&m[0])) +
        abs(correlate_phase6(&m[8])) +
        abs(correlate_phase6(&m[16])) +
        abs(correlate_phase6(&m[24])) +
        abs(correlate_phase6(&m[32]));
}

static inline int correlate_check_7(uint16_t *m) {
    return
        //abs(correlate_phase4(&m[0])) +
        //abs(correlate_phase1(&m[3])) +
        //abs(correlate_phase3(&m[5])) +
        //abs(correlate_phase0(&m[8])) +
        //abs(correlate_phase2(&m[10]));

        abs(correlate_phase7(&m[0])) +
        abs(correlate_phase7(&m[8])) +
        abs(correlate_phase7(&m[16])) +
        abs(correlate_phase7(&m[24])) +
        abs(correlate_phase7(&m[32]));
}

// Work out the best phase offset to use for the given message.
static int best_phase(uint16_t *m) {
    int test;
    int best = -1;
    //int bestval = (m[0] + m[1] + m[2] + m[3] + m[4] + m[5]); // minimum correlation quality we will accept
    int bestval = (m[0] + m[1] + m[2] + m[3] + m[4] + m[5] + m[6] + m[7] + m[8]); // minimum correlation quality we will accept

    // empirical testing suggests that 4..8 is the best range to test for here
    // (testing a wider range runs the danger of picking the wrong phase for
    // a message that would otherwise be successfully decoded - the correlation
    // functions can match well with a one symbol / half bit offset)

    // this is consistent with the peak detection which should produce
    // the first data symbol with phase offset 4..8

    //test = correlate_check_4(&m[0]);
    //if (test > bestval) { bestval = test; best = 4; }
    //test = correlate_check_0(&m[1]);
    //if (test > bestval) { bestval = test; best = 5; }
    //test = correlate_check_1(&m[1]);
    //if (test > bestval) { bestval = test; best = 6; }
    //test = correlate_check_2(&m[1]);
    //if (test > bestval) { bestval = test; best = 7; }
    //test = correlate_check_3(&m[1]);
    //if (test > bestval) { bestval = test; best = 8; }
    //return best;

    test = correlate_check_4(&m[0]);
    if (test > bestval) { bestval = test; best = 4; }
    test = correlate_check_5(&m[0]);
    if (test > bestval) { bestval = test; best = 5; }
    test = correlate_check_6(&m[0]);
    if (test > bestval) { bestval = test; best = 6; }
    test = correlate_check_7(&m[0]);
    if (test > bestval) { bestval = test; best = 7; }
    test = correlate_check_0(&m[1]);
    if (test > bestval) { bestval = test; best = 8; }
    test = correlate_check_1(&m[1]);
    if (test > bestval) { bestval = test; best = 9; }
    test = correlate_check_2(&m[1]);
    if (test > bestval) { bestval = test; best = 10; }
    test = correlate_check_3(&m[1]);
    if (test > bestval) { bestval = test; best = 11; }
    return best;
}

static int8_t testPreamble(uint16_t *preamble, int *high, uint32_t *base_signal, uint32_t *base_noise)
{
    // Create a matched filter to look for preamble sequence
    // Measure high level in 1's, noise in 0's and average in base_signal

    // Look for a message starting at around sample 0 with phase offset 3..7
    // The preamble is the follow sequence samples at 2MSPS
    // 0   1   2   3   4   5   6   7   
    // 1 0 1 0 0 0 0 1 0 1 0 0 0 0 0 0

    // At 8MSPS with phase of 0 it should look like this
    // 0         1         2         3         4         5         6         7
    // 1111 0000 1111 0000 0000 0000 0000 1111 0000 1111 0000 0000 0000 0000 0000 0000

    if (preamble[0] > preamble[4] &&                                        // 0
        preamble[4] < preamble[8] && preamble[8] > preamble[12] &&          // 1
        preamble[24] < preamble[28] && preamble[28] > preamble[32] &&       // 3.5
        preamble[32] < preamble[36])                                        // 4.5
        {      
            *high = (preamble[0] + preamble[8] + preamble[28] + preamble[32]) / 4;
            *base_signal = preamble[0] + preamble[8] + preamble[28];
            *base_noise = preamble[16] + preamble[20] + preamble[24];

            return 1;
        }
        else
        {
          //fprintf(stderr,"preamble rejected \n");
          //fprintf(stderr,"%i %i %i %i %i %i %i %i %i %i %i %i %i %i\n", 
          //preamble[0], preamble[4], preamble[8], preamble[12],
          //preamble[16], preamble[20], preamble[24], preamble[28],
          //preamble[32], preamble[36], preamble[40], preamble[44],
          //preamble[48], preamble[52]);

          //fprintf(stderr,"%i %i %i %i %i %i\n", 
          //preamble[0] > preamble[4],
          //preamble[4] < preamble[8],
          //preamble[8] > preamble[12],
          //preamble[24] < preamble[28],
          //preamble[28] > preamble[32],
          //preamble[32] < preamble[36] );



          return 0;
        }
}

static uint8_t DecodeByte(uint16_t *pPtr, int phase)
{
    uint16_t *pPtrInternal;  
    uint8_t theByte;

    pPtrInternal = pPtr + phase;
    theByte = 
    (slice_phase0(pPtrInternal) > 0 ? 0x80 : 0) |
    (slice_phase0(pPtrInternal+8) > 0 ? 0x40 : 0) |
    (slice_phase0(pPtrInternal+16) > 0 ? 0x20 : 0) |
    (slice_phase0(pPtrInternal+24) > 0 ? 0x10 : 0) |
    (slice_phase0(pPtrInternal+32) > 0 ? 0x08 : 0) |
    (slice_phase0(pPtrInternal+40) > 0 ? 0x04 : 0) |
    (slice_phase0(pPtrInternal+48) > 0 ? 0x02 : 0) |
    (slice_phase0(pPtrInternal+56) > 0 ? 0x01 : 0);

    return theByte;
}

//
// Given 'mlen' magnitude samples in 'm', sampled at 8.0MHz,
// try to demodulate some Mode S messages.
//
void demodulate8000_cdh(struct mag_buf *mag)
{
    struct modesMessage mm;
    unsigned char msg1[MODES_LONG_MSG_BYTES], msg2[MODES_LONG_MSG_BYTES], *msg;
    uint32_t j;

    unsigned char *bestmsg;
    int bestscore, bestphase;

    uint16_t *m = mag->data;
    uint32_t mlen = mag->length;

    uint64_t sum_scaled_signal_power = 0;

    memset(&mm, 0, sizeof(mm));
    msg = msg1;

    for (j = 0; j < mlen; j++) {
        uint16_t *preamble = &m[j];
        int high;
        uint32_t base_signal, base_noise;
        int initial_phase, first_phase, last_phase, try_phase;
        int msglen;

        // Look for a message starting at around sample 0 with phase offset 3..7
        // The preamble is the follow sequence samples at 2MSPS
        // 0   1   2   3   4   5   6   7   
        // 1 0 1 0 0 0 0 1 0 1 0 0 0 0 0 0

        // At 8MSPS with phase of 0 it should look like this
        // 0         1         2         3         4         5         6         7
        // 1111 0000 1111 0000 0000 0000 0000 1111 0000 1111 0000 0000 0000 0000 0000 0000

        // Ideal sample values for preambles with different phase
        // Xn is the first data symbol with phase offset N
        //
        // sample#: 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
        // phase 3: 2/4\0/5\1 0 0 0 0/5\1/3 3\0 0 0 0 0 0 X4
        // phase 4: 1/5\0/4\2 0 0 0 0/4\2 2/4\0 0 0 0 0 0 0 X0
        // phase 5: 0/5\1/3 3\0 0 0 0/3 3\1/5\0 0 0 0 0 0 0 X1
        // phase 6: 0/4\2 2/4\0 0 0 0 2/4\0/5\1 0 0 0 0 0 0 X2
        // phase 7: 0/3 3\1/5\0 0 0 0 1/5\0/4\2 0 0 0 0 0 0 X3
        //

        // sample#: 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0
        // phase 3: 2/4\0/5\1 0 0 0 0/5\1/3 3\0 0 0 0 0 0 X4
        // phase 4: 1/5\0/4\2 0 0 0 0/4\2 2/4\0 0 0 0 0 0 0 X0
        // phase 5: 0/5\1/3 3\0 0 0 0/3 3\1/5\0 0 0 0 0 0 0 X1
        // phase 6: 0/4\2 2/4\0 0 0 0 2/4\0/5\1 0 0 0 0 0 0 X2
        // phase 7: 0/3 3\1/5\0 0 0 0 1/5\0/4\2 0 0 0 0 0 0 X3
        //
        
        // quick check: we must have a rising edge 0->1 and a falling edge 12->13
        if (! (preamble[0] > preamble[4] && preamble[36] > preamble[40]) )
           continue;

        //fprintf(stderr,"Found preamble #1\n");

#if 0        
        if (preamble[1] > preamble[2] &&                                       // 1
            preamble[2] < preamble[3] && preamble[3] > preamble[4] &&          // 3
            preamble[8] < preamble[9] && preamble[9] > preamble[10] &&         // 9
            preamble[10] < preamble[11]) {                                     // 11-12
            // peaks at 1,3,9,11-12: phase 3
            high = (preamble[1] + preamble[3] + preamble[9] + preamble[11] + preamble[12]) / 4;
            base_signal = preamble[1] + preamble[3] + preamble[9];
            base_noise = preamble[5] + preamble[6] + preamble[7];

        } else if (preamble[1] > preamble[2] &&                                // 1
                   preamble[2] < preamble[3] && preamble[3] > preamble[4] &&   // 3
                   preamble[8] < preamble[9] && preamble[9] > preamble[10] &&  // 9
                   preamble[11] < preamble[12]) {                              // 12
            // peaks at 1,3,9,12: phase 4
            high = (preamble[1] + preamble[3] + preamble[9] + preamble[12]) / 4;
            base_signal = preamble[1] + preamble[3] + preamble[9] + preamble[12];
            base_noise = preamble[5] + preamble[6] + preamble[7] + preamble[8];
        } else if (preamble[1] > preamble[2] &&                                // 1
                   preamble[2] < preamble[3] && preamble[4] > preamble[5] &&   // 3-4
                   preamble[8] < preamble[9] && preamble[10] > preamble[11] && // 9-10
                   preamble[11] < preamble[12]) {                              // 12
            // peaks at 1,3-4,9-10,12: phase 5
            high = (preamble[1] + preamble[3] + preamble[4] + preamble[9] + preamble[10] + preamble[12]) / 4;
            base_signal = preamble[1] + preamble[12];
            base_noise = preamble[6] + preamble[7];
        } else if (preamble[1] > preamble[2] &&                                 // 1
                   preamble[3] < preamble[4] && preamble[4] > preamble[5] &&    // 4
                   preamble[9] < preamble[10] && preamble[10] > preamble[11] && // 10
                   preamble[11] < preamble[12]) {                               // 12
            // peaks at 1,4,10,12: phase 6
            high = (preamble[1] + preamble[4] + preamble[10] + preamble[12]) / 4;
            base_signal = preamble[1] + preamble[4] + preamble[10] + preamble[12];
            base_noise = preamble[5] + preamble[6] + preamble[7] + preamble[8];
        } else if (preamble[2] > preamble[3] &&                                 // 1-2
                   preamble[3] < preamble[4] && preamble[4] > preamble[5] &&    // 4
                   preamble[9] < preamble[10] && preamble[10] > preamble[11] && // 10
                   preamble[11] < preamble[12]) {                               // 12
            // peaks at 1-2,4,10,12: phase 7
            high = (preamble[1] + preamble[2] + preamble[4] + preamble[10] + preamble[12]) / 4;
            base_signal = preamble[4] + preamble[10] + preamble[12];
            base_noise = preamble[6] + preamble[7] + preamble[8];
        } else {
            // no suitable peaks
            continue;
        }
#endif

        if(!testPreamble(preamble, &high, &base_signal, &base_noise))
        {
            //no suitable peaks

            continue;
        }

        //fprintf(stderr,"Found preamble #2\n");

        // Check for enough signal
        if (base_signal * 2 < 3 * base_noise) // about 3.5dB SNR
            continue;

        //fprintf(stderr,"Found preamble #3\n");

#if 0
        // Check that the "quiet" bits 6,7,15,16,17 are actually quiet
        if (preamble[5] >= high ||
            preamble[6] >= high ||
            preamble[7] >= high ||
            preamble[8] >= high ||
            preamble[14] >= high ||
            preamble[15] >= high ||
            preamble[16] >= high ||
            preamble[17] >= high ||
            preamble[18] >= high) {
            continue;
        }
#endif
        // Check that the "quiet" bits 6,7,15,16,17 are actually quiet
        if (preamble[16] >= high ||
            preamble[20] >= high ||
            preamble[24] >= high ||
            preamble[32] >= high ||
            preamble[40] >= high ||
            preamble[44] >= high ||
            preamble[48] >= high ||
            preamble[52] >= high ||
            preamble[56] >= high) {
            continue;
        }

        //fprintf(stderr,"Found preamble #4\n");
        
        if (Modes.phase_enhance) {
            first_phase = 4;
            //last_phase = 8;           // try all phases
            last_phase = 11;           // try all phases
        } else {
            // Crosscorrelate against the first few bits to find a likely phase offset
            //initial_phase = best_phase(&preamble[19]);
            initial_phase = best_phase(&preamble[60]);
            if (initial_phase < 0) {
                continue; // nothing satisfactory
            }
            
            first_phase = last_phase = initial_phase;  // try only the phase we think it is
        }

        //fprintf(stderr,"Found preamble #5\n");

        Modes.stats_current.demod_preambles++;
        bestmsg = NULL; bestscore = -2; bestphase = -1;
        for (try_phase = first_phase; try_phase <= last_phase; ++try_phase) {
            uint16_t *pPtr;
            int phase, i, score, bytelen;

            // Decode all the next 112 bits, regardless of the actual message
            // size. We'll check the actual message type later
            
//            pPtr = &m[j+19] + (try_phase/5);
//            phase = try_phase % 5;
            pPtr = &m[j+60] + (try_phase/8);
            phase = try_phase % 8;

            bytelen = MODES_LONG_MSG_BYTES;
            for (i = 0; i < bytelen; ++i) {
                uint8_t theByte = 0;

                //Decode a byte at the give phase offset
                theByte = DecodeByte(pPtr, phase);
                //Move to the next byte (8 bits 8x oversampling)
                pPtr += 64;

#if 0
                //Decode a byte at the give phase offset
                switch (phase) {
                case 0:
                    theByte = 
                        (slice_phase0(pPtr) > 0 ? 0x80 : 0) |
                        (slice_phase0(pPtr+8) > 0 ? 0x40 : 0) |
                        (slice_phase0(pPtr+16) > 0 ? 0x20 : 0) |
                        (slice_phase0(pPtr+24) > 0 ? 0x10 : 0) |
                        (slice_phase0(pPtr+32 > 0 ? 0x08 : 0) |
                        (slice_phase0(pPtr+40) > 0 ? 0x04 : 0) |
                        (slice_phase0(pPtr+48) > 0 ? 0x02 : 0) |
                        (slice_phase0(pPtr+56) > 0 ? 0x01 : 0);


                    phase = 1;
                    pPtr += 64;
                    break;
                    
                case 1:
                    theByte =
                        (slice_phase1(pPtr) > 0 ? 0x80 : 0) |
                        (slice_phase3(pPtr+2) > 0 ? 0x40 : 0) |
                        (slice_phase0(pPtr+5) > 0 ? 0x20 : 0) |
                        (slice_phase2(pPtr+7) > 0 ? 0x10 : 0) |
                        (slice_phase4(pPtr+9) > 0 ? 0x08 : 0) |
                        (slice_phase1(pPtr+12) > 0 ? 0x04 : 0) |
                        (slice_phase3(pPtr+14) > 0 ? 0x02 : 0) |
                        (slice_phase0(pPtr+17) > 0 ? 0x01 : 0);

                    phase = 2;
                    pPtr += 19;
                    break;
                    
                case 2:
                    theByte =
                        (slice_phase2(pPtr) > 0 ? 0x80 : 0) |
                        (slice_phase4(pPtr+2) > 0 ? 0x40 : 0) |
                        (slice_phase1(pPtr+5) > 0 ? 0x20 : 0) |
                        (slice_phase3(pPtr+7) > 0 ? 0x10 : 0) |
                        (slice_phase0(pPtr+10) > 0 ? 0x08 : 0) |
                        (slice_phase2(pPtr+12) > 0 ? 0x04 : 0) |
                        (slice_phase4(pPtr+14) > 0 ? 0x02 : 0) |
                        (slice_phase1(pPtr+17) > 0 ? 0x01 : 0);

                    phase = 3;
                    pPtr += 19;
                    break;
                    
                case 3:
                    theByte = 
                        (slice_phase3(pPtr) > 0 ? 0x80 : 0) |
                        (slice_phase0(pPtr+3) > 0 ? 0x40 : 0) |
                        (slice_phase2(pPtr+5) > 0 ? 0x20 : 0) |
                        (slice_phase4(pPtr+7) > 0 ? 0x10 : 0) |
                        (slice_phase1(pPtr+10) > 0 ? 0x08 : 0) |
                        (slice_phase3(pPtr+12) > 0 ? 0x04 : 0) |
                        (slice_phase0(pPtr+15) > 0 ? 0x02 : 0) |
                        (slice_phase2(pPtr+17) > 0 ? 0x01 : 0);

                    phase = 4;
                    pPtr += 19;
                    break;
                    
                case 4:
                    theByte = 
                        (slice_phase4(pPtr) > 0 ? 0x80 : 0) |
                        (slice_phase1(pPtr+3) > 0 ? 0x40 : 0) |
                        (slice_phase3(pPtr+5) > 0 ? 0x20 : 0) |
                        (slice_phase0(pPtr+8) > 0 ? 0x10 : 0) |
                        (slice_phase2(pPtr+10) > 0 ? 0x08 : 0) |
                        (slice_phase4(pPtr+12) > 0 ? 0x04 : 0) |
                        (slice_phase1(pPtr+15) > 0 ? 0x02 : 0) |
                        (slice_phase3(pPtr+17) > 0 ? 0x01 : 0);

                    phase = 0;
                    pPtr += 20;
                    break;
                }
#endif

                msg[i] = theByte;
                if (i == 0) {
                    switch (msg[0] >> 3) {
                    case 0: case 4: case 5: case 11:
                        bytelen = MODES_SHORT_MSG_BYTES; break;
                        
                    case 16: case 17: case 18: case 20: case 21: case 24:
                        break;

                    default:
                        bytelen = 1; // unknown DF, give up immediately
                        break;
                    }
                }
            }

            // Score the mode S message and see if it's any good.
            score = scoreModesMessage(msg, i*8);
            if (score > bestscore) {
                // new high score!
                bestmsg = msg;
                bestscore = score;
                bestphase = try_phase;
                
                // swap to using the other buffer so we don't clobber our demodulated data
                // (if we find a better result then we'll swap back, but that's OK because
                // we no longer need this copy if we found a better one)
                msg = (msg == msg1) ? msg2 : msg1;
            }
        }

        // Do we have a candidate?
        if (bestscore < 0) {
            if (bestscore == -1)
                Modes.stats_current.demod_rejected_unknown_icao++;
            else
                Modes.stats_current.demod_rejected_bad++;
            continue; // nope.
        }

        msglen = modesMessageLenByType(bestmsg[0] >> 3);

        // Set initial mm structure details
        //mm.timestampMsg = mag->sampleTimestamp + (j*5) + bestphase;  // 2.4*5 = 12MHz
        mm.timestampMsg = mag->sampleTimestamp + (j*12/8) + bestphase; // 8*(12/8) = 12MHz

        // compute message receive time as block-start-time + difference in the 12MHz clock
        mm.sysTimestampMsg = mag->sysTimestamp; // start of block time
        mm.sysTimestampMsg.tv_nsec += receiveclock_ns_elapsed(mag->sampleTimestamp, mm.timestampMsg);
        normalize_timespec(&mm.sysTimestampMsg);

        mm.score = bestscore;
        mm.bFlags = mm.correctedbits   = 0;

        // Decode the received message
        {
            int result = decodeModesMessage(&mm, bestmsg);
            if (result < 0) {
                if (result == -1)
                    Modes.stats_current.demod_rejected_unknown_icao++;
                else
                    Modes.stats_current.demod_rejected_bad++;
                continue;
            } else {
                Modes.stats_current.demod_accepted[mm.correctedbits]++;
            }
        }

        // measure signal power
        {
            double signal_power;
            uint64_t scaled_signal_power = 0;
 //           int signal_len = msglen*12/5;  //Sample rate = 12/5 = 2.4MSPS
            int signal_len = msglen*8;  //Sample rate = 8MSPS
            int k;

            for (k = 0; k < signal_len; ++k) {
 //               uint32_t mag = m[j+19+k];
                uint32_t mag = m[j+60+k];
                scaled_signal_power += mag * mag;
            }

            signal_power = scaled_signal_power / 65535.0 / 65535.0;
            mm.signalLevel = signal_power / signal_len;
            Modes.stats_current.signal_power_sum += signal_power;
            Modes.stats_current.signal_power_count += signal_len;
            sum_scaled_signal_power += scaled_signal_power;

            if (mm.signalLevel > Modes.stats_current.peak_signal_power)
                Modes.stats_current.peak_signal_power = mm.signalLevel;
            if (mm.signalLevel > 0.50119)
                Modes.stats_current.strong_signal_count++; // signal power above -3dBFS
        }

        // Skip over the message:
        // (we actually skip to 8 bits before the end of the message,
        //  because we can often decode two messages that *almost* collide,
        //  where the preamble of the second message clobbered the last
        //  few bits of the first message, but the message bits didn't
        //  overlap)
        
        //j += msglen*12/5;  //Sample rate = 12/5 = 2.4MSPS
        j += msglen*8;       //Sample rate = 8MSPS
            
        // Pass data to the next layer
        useModesMessage(&mm);
    }

    /* update noise power if measured */
    if (Modes.measure_noise) {
        double sum_signal_power = sum_scaled_signal_power / 65535.0 / 65535.0;
        Modes.stats_current.noise_power_sum += (mag->total_power - sum_signal_power);
        Modes.stats_current.noise_power_count += mag->length;
    }
}


