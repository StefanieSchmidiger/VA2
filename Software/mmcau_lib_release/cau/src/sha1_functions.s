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
# SHA1: Initializes the SHA1 state variables
#
#   arguments
#           *sha1_state pointer to 160-bit block of SHA1 state variables:
#                           a,b,c,d,e
#
#   calling convention
#   void    cau_sha1_initialize_output (unsigned int *sha1_state)

    global  _cau_sha1_initialize_output
    global  cau_sha1_initialize_output
    align   4
_cau_sha1_initialize_output:
cau_sha1_initialize_output:
    mov.l   (4,%sp),%a0                 # get argument: *output
    lea     (sha1_init_h.w,%pc),%a1     # pointer to initial data

# copy initial data into hash output buffer (20 bytes = 5 longwords)
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+

    rts


#*******************************************************************************
#*******************************************************************************
#
# SHA1: Perform the hash for one or more input message blocks and generate the
#       message digest output
#
#   arguments
#           *msg_data   pointer to start of input message data
#           num_blks    number of 512-bit blocks to process
#           *sha1_state pointer to 160-bit block of SHA1 state variables:
#                           a,b,c,d,e
#
#   calling convention
#   void    cau_sha1_hash_n (const unsigned char *msg_data,
#                            const int            num_blks,
#                            unsigned int        *sha1_state)

    global  _cau_sha1_hash_n
    global  cau_sha1_hash_n
    align   4
_cau_sha1_hash_n:
cau_sha1_hash_n:
    link    %a6,&-348                   # temp stack space for 7 regs + w[*]
    movm.l  &0x04fc,(%sp)               # save d2/d3/d4/d5/d6/d7/a2
    mov.l   (8,%a6),%a1                 # get 1st argument: *msg_data   = m[*]
    mov.l   (12,%a6),%d1                # get 2nd argument: num_blks
    mov.l   (16,%a6),%a2                # get 3rd argument: *sha1_state = h[*]

    lea     (sha1_k.w,%pc),%a0          # pointer to k[*]
    movm.l  (%a0),&0x00f0               # load k[i] into d4,d5,d6,d7

    movq.l  &5,%d2                      # for rotating by 5
    movq.l  &1,%d3                      # for rotating by 1

    cp0ld.l (%a2)+,&LDR+CA0             # h[0] -> CA0
    cp0ld.l (%a2)+,&LDR+CA1             # h[1] -> CA1
    cp0ld.l (%a2)+,&LDR+CA2             # h[2] -> CA2
    cp0ld.l (%a2)+,&LDR+CA3             # h[3] -> CA3
    cp0ld.l (%a2)+,&LDR+CA4             # h[4] -> CA4
    lea     (-20,%a2),%a2               # reset a2 back to h[0]

    align   4
cau_sha1_hash_n_L%0:
    cp0ld.l &MVRA+CA0                   # a
    cp0ld.l %d2,&ROTL+CAA               # rotate by 5

    lea     (28,%sp),%a0                # local stack frame pointer to w[*]

    movq.l  &16,%d0                     # set loop counter
cau_sha1_hash_n_L%1:
    mov.l   (%a1)+,(%a0)                # copy m[i] to w[i]
    cp0ld.l &HASH+HFC                   # add Ch(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d4,&ADR+CAA                # add k[0]
    cp0ld.l (%a0)+,&ADR+CAA             # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_n_L%1         # (to keep text aligned)

    movq.l  &4,%d0                      # set loop counter
cau_sha1_hash_n_L%2:
    cp0ld.l &HASH+HFC                   # add Ch(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d4,&ADR+CAA                # add k[0]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers

    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_n_L%2         # (to keep text aligned)

    movq.l  &20,%d0                     # set loop counter
cau_sha1_hash_n_L%3:
    cp0ld.l &HASH+HFP                   # add Parity(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d5,&ADR+CAA                # add k[1]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_n_L%3         # (to keep text aligned)

    movq.l  &20,%d0                     # set loop counter
cau_sha1_hash_n_L%4:
    cp0ld.l &HASH+HFM                   # add Maj(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d6,&ADR+CAA                # add k[2]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_n_L%4         # (to keep text aligned)

    movq.l  &20,%d0                     # set loop counter
cau_sha1_hash_n_L%5:
    cp0ld.l &HASH+HFP                   # add Parity(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d7,&ADR+CAA                # add k[3]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.b   cau_sha1_hash_n_L%5

    cp0ld.l (%a2)+,&ADR+CA0             # add h[0]
    cp0ld.l (%a2)+,&ADR+CA1             # add h[1]
    cp0ld.l (%a2)+,&ADR+CA2             # add h[2]
    cp0ld.l (%a2)+,&ADR+CA3             # add h[3]
    cp0ld.l (%a2)+,&ADR+CA4             # add h[4]

    cp0st.l -(%a2),&STR+CA4             # store h[4]
    cp0st.l -(%a2),&STR+CA3             # store h[3]
    cp0st.l -(%a2),&STR+CA2             # store h[2]
    cp0st.l -(%a2),&STR+CA1             # store h[1]
    cp0st.l -(%a2),&STR+CA0             # store h[0]

    subq.l  &1,%d1                      # decrement block counter
    bne.w   cau_sha1_hash_n_L%0

    movm.l  (%sp),&0x04fc               # restore d2/d3/d4/d5/d6/d7/a2
    unlk    %a6
    rts


#*******************************************************************************
#*******************************************************************************
#
# SHA1: Updates SHA1 state variables for one or more input message blocks
#
#   arguments
#           *msg_data   pointer to start of input message data
#           num_blks    number of 512-bit blocks to process
#           *sha1_state pointer to 160-bit block of SHA1 state variables:
#                           a,b,c,d,e
#
#   calling convention
#   void    cau_sha1_update (const unsigned char *msg_data,
#                            const int            num_blks,
#                            unsigned int        *sha1_state)

    global  _cau_sha1_update
    global  cau_sha1_update
    align   4
_cau_sha1_update:
cau_sha1_update:
    mov.l   (12,%sp),%a0                # get 3rd argument: *sha1_state = h[*]
    lea     (sha1_init_h.w,%pc),%a1     # pointer to initial data

# copy initial data into hash output buffer (20 bytes = 5 longwords)
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+

    bra.w   cau_sha1_hash_n


#*******************************************************************************
#*******************************************************************************
#
# SHA1: Perform the hash for one input message block and generate the
#       message digest output
#
#   arguments
#           *msg_data   pointer to start of input message data
#           *sha1_state pointer to 160-bit block of SHA1 state variables:
#                           a,b,c,d,e
#
#   calling convention
#   void    cau_sha1_hash (const unsigned char *msg_data,
#                          unsigned int        *sha1_state)

    global  _cau_sha1_hash
    global  cau_sha1_hash
    align   4
_cau_sha1_hash:
cau_sha1_hash:
    link    %a6,&-344                   # temp stack space for 6 regs + w[*]
    movm.l  &0x04f8,(%sp)               # save d3/d4/d5/d6/d7/a2
    mov.l   (8,%a6),%a1                 # get 1st argument: *msg_data   = m[*]
    mov.l   (12,%a6),%a2                # get 2nd argument: *sha1_state = h[*]

    lea     (sha1_k.w,%pc),%a0          # pointer to k[*]
    movm.l  (%a0),&0x00f0               # load k[i] into d4,d5,d6,d7

    movq.l  &5,%d1                      # for rotating by 5
    movq.l  &1,%d3                      # for rotating by 1

    cp0ld.l (%a2)+,&LDR+CA0             # h[0] -> CA0
    cp0ld.l (%a2)+,&LDR+CA1             # h[1] -> CA1
    cp0ld.l (%a2)+,&LDR+CA2             # h[2] -> CA2
    cp0ld.l (%a2)+,&LDR+CA3             # h[3] -> CA3
    cp0ld.l (%a2)+,&LDR+CA4             # h[4] -> CA4
    lea     (-20,%a2),%a2               # reset a2 back to h[0]

    cp0ld.l &MVRA+CA0                   # a
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5

    lea     (24,%sp),%a0                # local stack frame pointer to w[*]
    movq.l  &16,%d0

cau_sha1_hash_L%1:
    mov.l   (%a1)+,(%a0)                # copy m[i] to w[i]
    cp0ld.l &HASH+HFC                   # add Ch(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d4,&ADR+CAA                # add k[0]
    cp0ld.l (%a0)+,&ADR+CAA             # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_L%1           # (to keep text aligned)

    movq.l  &4,%d0                      # set loop counter
cau_sha1_hash_L%2:
    cp0ld.l &HASH+HFC                   # add Ch(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d4,&ADR+CAA                # add k[0]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers

    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_L%2           # (to keep text aligned)

    movq.l  &20,%d0                     # set loop counter
cau_sha1_hash_L%3:
    cp0ld.l &HASH+HFP                   # add Parity(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d5,&ADR+CAA                # add k[1]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_L%3           # (to keep text aligned)

    movq.l  &20,%d0                     # set loop counter
cau_sha1_hash_L%4:
    cp0ld.l &HASH+HFM                   # add Maj(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d6,&ADR+CAA                # add k[2]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.w   cau_sha1_hash_L%4           # (to keep text aligned)

    movq.l  &20,%d0                     # set loop counter
cau_sha1_hash_L%5:
    cp0ld.l &HASH+HFP                   # add Parity(b,c,d)
    cp0ld.l &ADRA+CA4                   # add e
    cp0ld.l %d7,&ADR+CAA                # add k[3]
    cp0ld.l (-64,%a0),&LDR+CA5          # w[i-16]
    cp0ld.l (-56,%a0),&XOR+CA5          # w[i-14]
    cp0ld.l (-32,%a0),&XOR+CA5          # w[i-8]
    cp0ld.l (-12,%a0),&XOR+CA5          # w[i-3]
    cp0ld.l %d3,&ROTL+CA5               # rotate by 1
    cp0st.l (%a0)+,&STR+CA5             # store w[i]
    cp0ld.l &ADRA+CA5                   # add w[i]
    cp0ld.l &SHS                        # shift registers
    
    subq.l  &1,%d0                      # decrement local loop counter
    bne.b   cau_sha1_hash_L%5

    cp0ld.l (%a2)+,&ADR+CA0             # add h[0]
    cp0ld.l (%a2)+,&ADR+CA1             # add h[1]
    cp0ld.l (%a2)+,&ADR+CA2             # add h[2]
    cp0ld.l (%a2)+,&ADR+CA3             # add h[3]
    cp0ld.l (%a2)+,&ADR+CA4             # add h[4]

    cp0st.l -(%a2),&STR+CA4             # store h[4]
    cp0st.l -(%a2),&STR+CA3             # store h[3]
    cp0st.l -(%a2),&STR+CA2             # store h[2]
    cp0st.l -(%a2),&STR+CA1             # store h[1]
    cp0st.l -(%a2),&STR+CA0             # store h[0]

    movm.l  (%sp),&0x04f8               # restore d3/d4/d5/d6/d7/a2
    unlk    %a6
    rts


#*******************************************************************************
#*******************************************************************************
#
# CAU Constant Data

    align   16
sha1_init_h:
    long    0x67452301                  # initialize h[0] = output[0]
    long    0xefcdab89                  # initialize h[1] = output[1]
    long    0x98badcfe                  # initialize h[2] = output[2]
    long    0x10325476                  # initialize h[3] = output[3]
    long    0xc3d2e1f0                  # initialize h[4] = output[4]

    align   16
sha1_k:
    long    0x5a827999, 0x6ed9eba1, 0x8f1bbcdc, 0xca62c1d6
