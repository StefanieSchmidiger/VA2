#******************************************************************************
#******************************************************************************
#
# Copyright (c) Freescale Semiconductor, Inc 2011.
#

include(cau2_defines.hdr)
#*******************************************************************************
#*******************************************************************************
#
# NOTE: Code alignment for the CAU library functions in this file is carefully
# constructed to maximize processor performance. The existing alignment should
# be maintained for best results!!
#
# The individual code segments for the functions are presented first, followed
# by a common read-only data area containing the required data constants.
#
#*******************************************************************************
#*******************************************************************************


#*******************************************************************************
#*******************************************************************************
#
# AES: Performs an AES key expansion
#   arguments
#           *key        pointer to input key (128, 192, 256 bits in length)
#           key_size    key_size in bits (128, 192, 256)
#           *key_sch    pointer to key schedule output (44, 52, 60 longwords)
#
#   calling convention
#   void    cau_aes_set_key (const unsigned char *key,
#                            const int            key_size,
#                            unsigned char       *key_sch)

    global  _cau_aes_set_key
    global  cau_aes_set_key
    align   4
_cau_aes_set_key:
cau_aes_set_key:
    link    %a6,&-28                    # temp stack space for 7 regs
    movm.l  &0x001c,(%sp)               # save d2/d3/d4
    mov.l   (8,%a6),%a0                 # get 1st argument: *key
    mov.l   (16,%a6),%a1                # get 3rd argument: *key_sch
    movm.l  (%a0),&0x000f               # load key[0-3]
    movm.l  &0x000f,(%a1)               # copy key[0-3] to key_sch[0-3]
    mov.l   (12,%a6),%d4                # load key_size
    cmpi.l  %d4,&128                    # check key_size
    beq.w   aes_set_key128_L%0          # branch if key_size = 128 bits
    cmpi.l  %d4,&192                    # check key_size
    beq.w   aes_set_key192_L%0          # branch if key_size = 192 bits

# key_size = 256 bits
    align   4
aes_set_key256_L%0:
    movm.l  &0x04e0,(12,%sp)            # save d5/d6/d7/a2
    movm.l  (16,%a0),&0x00f0            # load key[4-7]
    movm.l  &0x00f0,(16,%a1)            # copy key[4-7] to key_sch[4-7]
    mov.w   &8,%a2                      # rotate count
    lea     (32,%a1),%a1                # adjust pointer to key_sch[8]
    lea     (rcon.w,%pc),%a0            # pointer to rcon[0]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[7]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[0]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[0]
    cp0st.l %d0,&STR+CAA                # get key_sch[8]
    mov.l   %d0,(%a1)+                  # store key_sch[8]
    eor.l   %d0,%d1                     # generate key_sch[9]
    mov.l   %d1,(%a1)+                  # store key_sch[9]
    eor.l   %d1,%d2                     # generate key_sch[10]
    mov.l   %d2,(%a1)+                  # store key_sch[10]
    eor.l   %d2,%d3                     # generate key_sch[11]
    mov.l   %d3,(%a1)+                  # store key_sch[11]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[11]
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l %d4,&XOR+CAA                # XOR key_sch[4]
    cp0st.l %d4,&STR+CAA                # get key_sch[12]
    mov.l   %d4,(%a1)+                  # store key_sch[12]
    eor.l   %d4,%d5                     # generate key_sch[13]
    mov.l   %d5,(%a1)+                  # store key_sch[13]
    eor.l   %d5,%d6                     # generate key_sch[14]
    mov.l   %d6,(%a1)+                  # store key_sch[14]
    eor.l   %d6,%d7                     # generate key_sch[15]
    mov.l   %d7,(%a1)+                  # store key_sch[15]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[15]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[1]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[8]
    cp0st.l %d0,&STR+CAA                # get key_sch[16]
    mov.l   %d0,(%a1)+                  # store key_sch[16]
    eor.l   %d0,%d1                     # generate key_sch[17]
    mov.l   %d1,(%a1)+                  # store key_sch[17]
    eor.l   %d1,%d2                     # generate key_sch[18]
    mov.l   %d2,(%a1)+                  # store key_sch[18]
    eor.l   %d2,%d3                     # generate key_sch[19]
    mov.l   %d3,(%a1)+                  # store key_sch[19]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[19]
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l %d4,&XOR+CAA                # XOR key_sch[12]
    cp0st.l %d4,&STR+CAA                # get key_sch[20]
    mov.l   %d4,(%a1)+                  # store key_sch[20]
    eor.l   %d4,%d5                     # generate key_sch[21]
    mov.l   %d5,(%a1)+                  # store key_sch[21]
    eor.l   %d5,%d6                     # generate key_sch[22]
    mov.l   %d6,(%a1)+                  # store key_sch[22]
    eor.l   %d6,%d7                     # generate key_sch[23]
    mov.l   %d7,(%a1)+                  # store key_sch[23]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[23]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[2]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[16]
    cp0st.l %d0,&STR+CAA                # get key_sch[24]
    mov.l   %d0,(%a1)+                  # store key_sch[24]
    eor.l   %d0,%d1                     # generate key_sch[25]
    mov.l   %d1,(%a1)+                  # store key_sch[25]
    eor.l   %d1,%d2                     # generate key_sch[26]
    mov.l   %d2,(%a1)+                  # store key_sch[26]
    eor.l   %d2,%d3                     # generate key_sch[27]
    mov.l   %d3,(%a1)+                  # store key_sch[27]
    
    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[27]
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l %d4,&XOR+CAA                # XOR key_sch[20]
    cp0st.l %d4,&STR+CAA                # get key_sch[28]
    mov.l   %d4,(%a1)+                  # store key_sch[28]
    eor.l   %d4,%d5                     # generate key_sch[29]
    mov.l   %d5,(%a1)+                  # store key_sch[29]
    eor.l   %d5,%d6                     # generate key_sch[30]
    mov.l   %d6,(%a1)+                  # store key_sch[30]
    eor.l   %d6,%d7                     # generate key_sch[31]
    mov.l   %d7,(%a1)+                  # store key_sch[31]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[31]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[3]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[24]
    cp0st.l %d0,&STR+CAA                # get key_sch[32]
    mov.l   %d0,(%a1)+                  # store key_sch[32]
    eor.l   %d0,%d1                     # generate key_sch[33]
    mov.l   %d1,(%a1)+                  # store key_sch[33]
    eor.l   %d1,%d2                     # generate key_sch[34]
    mov.l   %d2,(%a1)+                  # store key_sch[34]
    eor.l   %d2,%d3                     # generate key_sch[35]
    mov.l   %d3,(%a1)+                  # store key_sch[35]
    
    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[35]
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l %d4,&XOR+CAA                # XOR key_sch[28]
    cp0st.l %d4,&STR+CAA                # get key_sch[36]
    mov.l   %d4,(%a1)+                  # store key_sch[36]
    eor.l   %d4,%d5                     # generate key_sch[37]
    mov.l   %d5,(%a1)+                  # store key_sch[37]
    eor.l   %d5,%d6                     # generate key_sch[38]
    mov.l   %d6,(%a1)+                  # store key_sch[38]
    eor.l   %d6,%d7                     # generate key_sch[39]
    mov.l   %d7,(%a1)+                  # store key_sch[39]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[39]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[4]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[32]
    cp0st.l %d0,&STR+CAA                # get key_sch[40]
    mov.l   %d0,(%a1)+                  # store key_sch[40]
    eor.l   %d0,%d1                     # generate key_sch[41]
    mov.l   %d1,(%a1)+                  # store key_sch[41]
    eor.l   %d1,%d2                     # generate key_sch[42]
    mov.l   %d2,(%a1)+                  # store key_sch[42]
    eor.l   %d2,%d3                     # generate key_sch[43]
    mov.l   %d3,(%a1)+                  # store key_sch[43]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[43]
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l %d4,&XOR+CAA                # XOR key_sch[36]
    cp0st.l %d4,&STR+CAA                # get key_sch[44]
    mov.l   %d4,(%a1)+                  # store key_sch[44]
    eor.l   %d4,%d5                     # generate key_sch[45]
    mov.l   %d5,(%a1)+                  # store key_sch[45]
    eor.l   %d5,%d6                     # generate key_sch[46]
    mov.l   %d6,(%a1)+                  # store key_sch[46]
    eor.l   %d6,%d7                     # generate key_sch[47]
    mov.l   %d7,(%a1)+                  # store key_sch[47]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[47]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[5]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[40]
    cp0st.l %d0,&STR+CAA                # get key_sch[48]
    mov.l   %d0,(%a1)+                  # store key_sch[48]
    eor.l   %d0,%d1                     # generate key_sch[49]
    mov.l   %d1,(%a1)+                  # store key_sch[49]
    eor.l   %d1,%d2                     # generate key_sch[50]
    mov.l   %d2,(%a1)+                  # store key_sch[50]
    eor.l   %d2,%d3                     # generate key_sch[51]
    mov.l   %d3,(%a1)+                  # store key_sch[51]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[51]
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l %d4,&XOR+CAA                # XOR key_sch[44]
    cp0st.l %d4,&STR+CAA                # get key_sch[52]
    mov.l   %d4,(%a1)+                  # store key_sch[52]
    eor.l   %d4,%d5                     # generate key_sch[53]
    mov.l   %d5,(%a1)+                  # store key_sch[53]
    eor.l   %d5,%d6                     # generate key_sch[54]
    mov.l   %d6,(%a1)+                  # store key_sch[54]
    eor.l   %d6,%d7                     # generate key_sch[55]
    mov.l   %d7,(%a1)+                  # store key_sch[55]

    align   4
    cp0ld.l %d7,&LDR+CAA                # load CAU accumulator with key_sch[55]
    cp0ld.l %a2,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[6]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[48]
    cp0st.l %d0,&STR+CAA                # get key_sch[56]
    mov.l   %d0,(%a1)+                  # store key_sch[56]
    eor.l   %d0,%d1                     # generate key_sch[57]
    mov.l   %d1,(%a1)+                  # store key_sch[57]
    eor.l   %d1,%d2                     # generate key_sch[58]
    mov.l   %d2,(%a1)+                  # store key_sch[58]
    eor.l   %d2,%d3                     # generate key_sch[59]
    mov.l   %d3,(%a1)+                  # store key_sch[59]

    movm.l  (%sp),&0x04fc               # restore d2/d3/d4/d5/d6/d7/a2
    unlk    %a6
    rts

# key_size = 192 bits
    align   4
aes_set_key192_L%0:
    movm.l  &0x0060,(12,%sp)            # save d5/d6
    movm.l  (16,%a0),&0x0030            # load key[4-5]
    movm.l  &0x0030,(16,%a1)            # copy key[4-5] to key_sch[4-5]
    movq    &8,%d6                      # rotate count
    lea     (24,%a1),%a1                # adjust pointer to key_sch[6]
    lea     (rcon.w,%pc),%a0            # pointer to rcon[0]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[5]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[0]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[0]
    cp0st.l %d0,&STR+CAA                # get key_sch[6]
    mov.l   %d0,(%a1)+                  # store key_sch[6]
    eor.l   %d0,%d1                     # generate key_sch[7]
    mov.l   %d1,(%a1)+                  # store key_sch[7]
    eor.l   %d1,%d2                     # generate key_sch[8]
    mov.l   %d2,(%a1)+                  # store key_sch[8]
    eor.l   %d2,%d3                     # generate key_sch[9]
    mov.l   %d3,(%a1)+                  # store key_sch[9]
    eor.l   %d3,%d4                     # generate key_sch[10]
    mov.l   %d4,(%a1)+                  # store key_sch[10]
    eor.l   %d4,%d5                     # generate key_sch[11]
    mov.l   %d5,(%a1)+                  # store key_sch[11]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[11]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[1]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[6]
    cp0st.l %d0,&STR+CAA                # get key_sch[12]
    mov.l   %d0,(%a1)+                  # store key_sch[12]
    eor.l   %d0,%d1                     # generate key_sch[13]
    mov.l   %d1,(%a1)+                  # store key_sch[13]
    eor.l   %d1,%d2                     # generate key_sch[14]
    mov.l   %d2,(%a1)+                  # store key_sch[14]
    eor.l   %d2,%d3                     # generate key_sch[15]
    mov.l   %d3,(%a1)+                  # store key_sch[15]
    eor.l   %d3,%d4                     # generate key_sch[16]
    mov.l   %d4,(%a1)+                  # store key_sch[16]
    eor.l   %d4,%d5                     # generate key_sch[17]
    mov.l   %d5,(%a1)+                  # store key_sch[17]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[17]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[2]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[12]
    cp0st.l %d0,&STR+CAA                # get key_sch[18]
    mov.l   %d0,(%a1)+                  # store key_sch[18]
    eor.l   %d0,%d1                     # generate key_sch[19]
    mov.l   %d1,(%a1)+                  # store key_sch[19]
    eor.l   %d1,%d2                     # generate key_sch[20]
    mov.l   %d2,(%a1)+                  # store key_sch[20]
    eor.l   %d2,%d3                     # generate key_sch[21]
    mov.l   %d3,(%a1)+                  # store key_sch[21]
    eor.l   %d3,%d4                     # generate key_sch[22]
    mov.l   %d4,(%a1)+                  # store key_sch[22]
    eor.l   %d4,%d5                     # generate key_sch[23]
    mov.l   %d5,(%a1)+                  # store key_sch[23]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[23]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[3]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[18]
    cp0st.l %d0,&STR+CAA                # get key_sch[24]
    mov.l   %d0,(%a1)+                  # store key_sch[24]
    eor.l   %d0,%d1                     # generate key_sch[25]
    mov.l   %d1,(%a1)+                  # store key_sch[25]
    eor.l   %d1,%d2                     # generate key_sch[26]
    mov.l   %d2,(%a1)+                  # store key_sch[26]
    eor.l   %d2,%d3                     # generate key_sch[27]
    mov.l   %d3,(%a1)+                  # store key_sch[27]
    eor.l   %d3,%d4                     # generate key_sch[28]
    mov.l   %d4,(%a1)+                  # store key_sch[28]
    eor.l   %d4,%d5                     # generate key_sch[29]
    mov.l   %d5,(%a1)+                  # store key_sch[29]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[29]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[4]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[24]
    cp0st.l %d0,&STR+CAA                # get key_sch[30]
    mov.l   %d0,(%a1)+                  # store key_sch[30]
    eor.l   %d0,%d1                     # generate key_sch[31]
    mov.l   %d1,(%a1)+                  # store key_sch[31]
    eor.l   %d1,%d2                     # generate key_sch[32]
    mov.l   %d2,(%a1)+                  # store key_sch[32]
    eor.l   %d2,%d3                     # generate key_sch[33]
    mov.l   %d3,(%a1)+                  # store key_sch[33]
    eor.l   %d3,%d4                     # generate key_sch[34]
    mov.l   %d4,(%a1)+                  # store key_sch[34]
    eor.l   %d4,%d5                     # generate key_sch[35]
    mov.l   %d5,(%a1)+                  # store key_sch[35]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[35]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[5]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[30]
    cp0st.l %d0,&STR+CAA                # get key_sch[36]
    mov.l   %d0,(%a1)+                  # store key_sch[36]
    eor.l   %d0,%d1                     # generate key_sch[37]
    mov.l   %d1,(%a1)+                  # store key_sch[37]
    eor.l   %d1,%d2                     # generate key_sch[38]
    mov.l   %d2,(%a1)+                  # store key_sch[38]
    eor.l   %d2,%d3                     # generate key_sch[39]
    mov.l   %d3,(%a1)+                  # store key_sch[39]
    eor.l   %d3,%d4                     # generate key_sch[40]
    mov.l   %d4,(%a1)+                  # store key_sch[40]
    eor.l   %d4,%d5                     # generate key_sch[41]
    mov.l   %d5,(%a1)+                  # store key_sch[41]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[41]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[6]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[36]
    cp0st.l %d0,&STR+CAA                # get key_sch[42]
    mov.l   %d0,(%a1)+                  # store key_sch[42]
    eor.l   %d0,%d1                     # generate key_sch[43]
    mov.l   %d1,(%a1)+                  # store key_sch[43]
    eor.l   %d1,%d2                     # generate key_sch[44]
    mov.l   %d2,(%a1)+                  # store key_sch[44]
    eor.l   %d2,%d3                     # generate key_sch[45]
    mov.l   %d3,(%a1)+                  # store key_sch[45]
    eor.l   %d3,%d4                     # generate key_sch[46]
    mov.l   %d4,(%a1)+                  # store key_sch[46]
    eor.l   %d4,%d5                     # generate key_sch[47]
    mov.l   %d5,(%a1)+                  # store key_sch[47]

    align   4
    cp0ld.l %d5,&LDR+CAA                # load CAU accumulator with key_sch[47]
    cp0ld.l %d6,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[7]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[42]
    cp0st.l %d0,&STR+CAA                # get key_sch[48]
    mov.l   %d0,(%a1)+                  # store key_sch[48]
    eor.l   %d0,%d1                     # generate key_sch[49]
    mov.l   %d1,(%a1)+                  # store key_sch[49]
    eor.l   %d1,%d2                     # generate key_sch[50]
    mov.l   %d2,(%a1)+                  # store key_sch[50]
    eor.l   %d2,%d3                     # generate key_sch[51]
    mov.l   %d3,(%a1)+                  # store key_sch[51]

    movm.l  (%sp),&0x007c               # restore d2/d3/d4/d5/d6
    unlk    %a6
    rts

# key_size = 128 bits
    align   4
aes_set_key128_L%0:
    movq    &8,%d4                      # rotate count
    lea     (16,%a1),%a1                # adjust pointer to key_sch[4]
    lea     (rcon.w,%pc),%a0            # pointer to rcon[0]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[3]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[0]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[0]
    cp0st.l %d0,&STR+CAA                # get key_sch[4]
    mov.l   %d0,(%a1)+                  # store key_sch[4]
    eor.l   %d0,%d1                     # generate key_sch[5]
    mov.l   %d1,(%a1)+                  # store key_sch[5]
    eor.l   %d1,%d2                     # generate key_sch[6]
    mov.l   %d2,(%a1)+                  # store key_sch[6]
    eor.l   %d2,%d3                     # generate key_sch[7]
    mov.l   %d3,(%a1)+                  # store key_sch[7]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[7]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[1]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[4]
    cp0st.l %d0,&STR+CAA                # get key_sch[8]
    mov.l   %d0,(%a1)+                  # store key_sch[8]
    eor.l   %d0,%d1                     # generate key_sch[9]
    mov.l   %d1,(%a1)+                  # store key_sch[9]
    eor.l   %d1,%d2                     # generate key_sch[10]
    mov.l   %d2,(%a1)+                  # store key_sch[10]
    eor.l   %d2,%d3                     # generate key_sch[11]
    mov.l   %d3,(%a1)+                  # store key_sch[11]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[11]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[2]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[8]
    cp0st.l %d0,&STR+CAA                # get key_sch[12]
    mov.l   %d0,(%a1)+                  # store key_sch[12]
    eor.l   %d0,%d1                     # generate key_sch[13]
    mov.l   %d1,(%a1)+                  # store key_sch[13]
    eor.l   %d1,%d2                     # generate key_sch[14]
    mov.l   %d2,(%a1)+                  # store key_sch[14]
    eor.l   %d2,%d3                     # generate key_sch[15]
    mov.l   %d3,(%a1)+                  # store key_sch[15]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[15]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[3]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[12]
    cp0st.l %d0,&STR+CAA                # get key_sch[16]
    mov.l   %d0,(%a1)+                  # store key_sch[16]
    eor.l   %d0,%d1                     # generate key_sch[17]
    mov.l   %d1,(%a1)+                  # store key_sch[17]
    eor.l   %d1,%d2                     # generate key_sch[18]
    mov.l   %d2,(%a1)+                  # store key_sch[18]
    eor.l   %d2,%d3                     # generate key_sch[19]
    mov.l   %d3,(%a1)+                  # store key_sch[19]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[19]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[4]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[16]
    cp0st.l %d0,&STR+CAA                # get key_sch[20]
    mov.l   %d0,(%a1)+                  # store key_sch[20]
    eor.l   %d0,%d1                     # generate key_sch[21]
    mov.l   %d1,(%a1)+                  # store key_sch[21]
    eor.l   %d1,%d2                     # generate key_sch[22]
    mov.l   %d2,(%a1)+                  # store key_sch[22]
    eor.l   %d2,%d3                     # generate key_sch[23]
    mov.l   %d3,(%a1)+                  # store key_sch[23]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[23]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[5]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[20]
    cp0st.l %d0,&STR+CAA                # get key_sch[24]
    mov.l   %d0,(%a1)+                  # store key_sch[24]
    eor.l   %d0,%d1                     # generate key_sch[25]
    mov.l   %d1,(%a1)+                  # store key_sch[25]
    eor.l   %d1,%d2                     # generate key_sch[26]
    mov.l   %d2,(%a1)+                  # store key_sch[26]
    eor.l   %d2,%d3                     # generate key_sch[27]
    mov.l   %d3,(%a1)+                  # store key_sch[27]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[27]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[6]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[24]
    cp0st.l %d0,&STR+CAA                # get key_sch[28]
    mov.l   %d0,(%a1)+                  # store key_sch[28]
    eor.l   %d0,%d1                     # generate key_sch[29]
    mov.l   %d1,(%a1)+                  # store key_sch[29]
    eor.l   %d1,%d2                     # generate key_sch[30]
    mov.l   %d2,(%a1)+                  # store key_sch[30]
    eor.l   %d2,%d3                     # generate key_sch[31]
    mov.l   %d3,(%a1)+                  # store key_sch[31]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[31]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[7]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[28]
    cp0st.l %d0,&STR+CAA                # get key_sch[32]
    mov.l   %d0,(%a1)+                  # store key_sch[32]
    eor.l   %d0,%d1                     # generate key_sch[33]
    mov.l   %d1,(%a1)+                  # store key_sch[33]
    eor.l   %d1,%d2                     # generate key_sch[34]
    mov.l   %d2,(%a1)+                  # store key_sch[34]
    eor.l   %d2,%d3                     # generate key_sch[35]
    mov.l   %d3,(%a1)+                  # store key_sch[35]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[35]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[8]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[32]
    cp0st.l %d0,&STR+CAA                # get key_sch[36]
    mov.l   %d0,(%a1)+                  # store key_sch[36]
    eor.l   %d0,%d1                     # generate key_sch[37]
    mov.l   %d1,(%a1)+                  # store key_sch[37]
    eor.l   %d1,%d2                     # generate key_sch[38]
    mov.l   %d2,(%a1)+                  # store key_sch[38]
    eor.l   %d2,%d3                     # generate key_sch[39]
    mov.l   %d3,(%a1)+                  # store key_sch[39]

    align   4
    cp0ld.l %d3,&LDR+CAA                # load CAU accumulator with key_sch[39]
    cp0ld.l %d4,&ROTL+CAA               # rotate left by 8
    cp0ld.l &AESS+CAA                   # SubBytes
    cp0ld.l (%a0)+,&XOR+CAA             # XOR rcon[9]
    cp0ld.l %d0,&XOR+CAA                # XOR key_sch[36]
    cp0st.l %d0,&STR+CAA                # get key_sch[40]
    mov.l   %d0,(%a1)+                  # store key_sch[40]
    eor.l   %d0,%d1                     # generate key_sch[41]
    mov.l   %d1,(%a1)+                  # store key_sch[41]
    eor.l   %d1,%d2                     # generate key_sch[42]
    mov.l   %d2,(%a1)+                  # store key_sch[42]
    eor.l   %d2,%d3                     # generate key_sch[43]
    mov.l   %d3,(%a1)+                  # store key_sch[43]

    movm.l  (%sp),&0x001c               # restore d2/d3/d4
    unlk    %a6
    rts
    

#*******************************************************************************
#*******************************************************************************
#
# AES: Encrypts a single 16-byte block
#   arguments
#           *in         pointer to 16-byte block of input plaintext
#           *key_sch    pointer to key schedule (44, 52, 60 longwords)
#           nr          number of AES rounds (10, 12, 14 = f(key_schedule))
#           *out        pointer to 16-byte block of output ciphertext
#
#   NOTE    Input and output blocks may overlap
#
#   calling convention
#   void    cau_aes_encrypt (const unsigned char *in,
#                            const unsigned char *key_sch,
#                            const int            nr,
#                            unsigned char       *out)

    global  _cau_aes_encrypt
    global  cau_aes_encrypt
    align   4
_cau_aes_encrypt:
cau_aes_encrypt:
    mov.l   (4,%sp),%a1                 # get 1st argument: *in
    mov.l   (8,%sp),%a0                 # get 2nd argument: *key_sch

    cp0ld.l (%a1)+,&LDR+CA0             # load plain[0] -> CA0
    cp0ld.l (%a1)+,&LDR+CA1             # load plain[1] -> CA1
    cp0ld.l (%a1)+,&LDR+CA2             # load plain[2] -> CA2
    cp0ld.l (%a1)+,&LDR+CA3             # load plain[3] -> CA3
    cp0ld.l (%a0)+,&XOR+CA0             # XOR keys
    cp0ld.l (%a0)+,&XOR+CA1
    cp0ld.l (%a0)+,&XOR+CA2
    cp0ld.l (%a0)+,&XOR+CA3
    mov.l   (12,%sp),%d0                # get 3rd argument: nr
    subq.l  &1,%d0                      # decrement the nr count

    align   4
aes_encrypt_key128_L%0:
    cp0ld.l &AESS+CA0                   # SubBytes
    cp0ld.l &AESS+CA1
    cp0ld.l &AESS+CA2
    cp0ld.l &AESS+CA3
    cp0ld.l &AESR                       # ShiftRows
    cp0ld.l (%a0)+,&AESC+CA0            # MixColumns
    cp0ld.l (%a0)+,&AESC+CA1
    cp0ld.l (%a0)+,&AESC+CA2
    cp0ld.l (%a0)+,&AESC+CA3
    subq.l  &1,%d0
    bne.b   aes_encrypt_key128_L%0

    cp0ld.l &AESS+CA0                   # SubBytes
    cp0ld.l &AESS+CA1
    cp0ld.l &AESS+CA2
    cp0ld.l &AESS+CA3
    cp0ld.l &AESR                       # ShiftRows
    cp0ld.l (%a0)+,&XOR+CA0             # XOR keys
    cp0ld.l (%a0)+,&XOR+CA1
    cp0ld.l (%a0)+,&XOR+CA2
    cp0ld.l (%a0)+,&XOR+CA3

    mov.l   (16,%sp),%a1                # get 4th argument: *out
    cp0st.l %d0,&STR+CA0                # get ciphertext results
    cp0st.l %d1,&STR+CA1
    mov.l   %d0,(%a1)+                  # store ciphertext[0]
    mov.l   %d1,(%a1)+                  # store ciphertext[1]
    cp0st.l %d0,&STR+CA2                # get more ciphertext results
    cp0st.l %d1,&STR+CA3
    mov.l   %d0,(%a1)+                  # store ciphertext[2]
    mov.l   %d1,(%a1)+                  # store ciphertext[3]
    rts


#*******************************************************************************
#*******************************************************************************
#
# AES: Decrypts a single 16-byte block
#   arguments
#           *in         pointer to 16-byte block of input chiphertext
#           *key_sch    pointer to key schedule (44, 52, 60 longwords)
#           nr          number of AES rounds (10, 12, 14 = f(key_schedule))
#           *out        pointer to 16-byte block of output plaintext
#
#   NOTE    Input and output blocks may overlap
#
#   calling convention
#   void    cau_aes_decrypt (const unsigned char *in,
#                            const unsigned char *key_sch,
#                            const int            nr,
#                            unsigned char       *out)

    global  _cau_aes_decrypt
    global  cau_aes_decrypt
    align   4
_cau_aes_decrypt:
cau_aes_decrypt:
    mov.l   (4,%sp),%a1                 # get 1st argument: *in
    mov.l   (8,%sp),%a0                 # get 2nd argument: *key_sch
    mov.l   (12,%sp),%d0                # get 3rd argument: nr
    mov.l   %d0,%d1                     # copy nr into d1
    asl.l   &4,%d1                      # form 16*nr 

    cp0ld.l (%a1)+,&LDR+CA0             # load cipher[0] -> CA0
    cp0ld.l (%a1)+,&LDR+CA1             # load cipher[1] -> CA1
    cp0ld.l (%a1)+,&LDR+CA2             # load cipher[2] -> CA2
    cp0ld.l (%a1)+,&LDR+CA3             # load cipher[3] -> CA3

# the key_sch pointer (%a0) is adjusted to define the end of the elements
# the adjustment factor = f(nr) and defined by the expression:
#   end of key_sch = (4*nr + 4) * 4 = 16*nr + 16 for nr = {10, 12, 14}
    lea     (16,%a0,%d1.l),%a0          # adjust key_sch ptr to end of elements
    cp0ld.l -(%a0),&XOR+CA3             # XOR keys
    cp0ld.l -(%a0),&XOR+CA2
    cp0ld.l -(%a0),&XOR+CA1
    cp0ld.l -(%a0),&XOR+CA0
    subq.l  &1,%d0                      # form nr - 1 as loop count

    align   4
aes_decrypt_key128_L%0:
    cp0ld.l &AESIR                      # InvShiftRows
    cp0ld.l &AESIS+CA3                  # InvSubBytes
    cp0ld.l &AESIS+CA2
    cp0ld.l &AESIS+CA1
    cp0ld.l &AESIS+CA0
    cp0ld.l -(%a0),&AESIC+CA3           # InvMixColumns
    cp0ld.l -(%a0),&AESIC+CA2
    cp0ld.l -(%a0),&AESIC+CA1
    cp0ld.l -(%a0),&AESIC+CA0
    subq.l  &1,%d0
    bne.b   aes_decrypt_key128_L%0

    cp0ld.l &AESIR                      # InvShiftRows
    cp0ld.l &AESIS+CA3                  # InvSubBytes
    cp0ld.l &AESIS+CA2
    cp0ld.l &AESIS+CA1
    cp0ld.l &AESIS+CA0
    cp0ld.l -(%a0),&XOR+CA3             # XOR keys
    cp0ld.l -(%a0),&XOR+CA2
    cp0ld.l -(%a0),&XOR+CA1
    cp0ld.l -(%a0),&XOR+CA0

    mov.l   (16,%sp),%a1                # get 4th argument: *out
    cp0st.l %d0,&STR+CA0                # get plaintext results
    cp0st.l %d1,&STR+CA1
    mov.l   %d0,(%a1)+                  # store plaintext[0]
    mov.l   %d1,(%a1)+                  # store plaintext[1]
    cp0st.l %d0,&STR+CA2                # get more plaintext results
    cp0st.l %d1,&STR+CA3
    mov.l   %d0,(%a1)+                  # store plaintext[2]
    mov.l   %d1,(%a1)+                  # store plaintext[3]
    rts


#*******************************************************************************
#*******************************************************************************
#
# CAU Constant Data

    align   16
rcon:
    long    0x01000000, 0x02000000, 0x04000000, 0x08000000
    long    0x10000000, 0x20000000, 0x40000000, 0x80000000
    long    0x1b000000, 0x36000000
