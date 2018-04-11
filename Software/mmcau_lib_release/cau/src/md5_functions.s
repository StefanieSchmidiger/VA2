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
# MD5: Initializes the MD5 state variables
#
#   arguments
#           *md5_state  pointer to 128-bit block of MD5 state variables:
#                           a,b,c,d
#
#   calling convention
#   void    cau_md5_initialize_output (unsigned char *md5_state)

    global  _cau_md5_initialize_output
    global  cau_md5_initialize_output
    align   4
_cau_md5_initialize_output:
cau_md5_initialize_output:
    mov.l   (4,%sp),%a0                 # get argument: *output
    lea     (md5_init_h.w,%pc),%a1      # pointer to initial data

# copy initial data into hash output buffer (16 bytes = 4 longwords)
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+

    rts


#*******************************************************************************
#*******************************************************************************
#
# MD5: Perform the hash for one or more input message blocks and generate the
#      message digest output
#
#   arguments
#           *msg_data   pointer to start of input message data
#           num_blks    number of 512-bit blocks to process
#           *md5_state  pointer to 128-bit block of MD5 state variables: a,b,c,d
#
#   calling convention
#   void    cau_md5_hash_n (const unsigned char *msg_data,
#                           const int            num_blks,
#                           unsigned char       *md5_state)

    global  _cau_md5_hash_n
    global  cau_md5_hash_n
    align   4
_cau_md5_hash_n:
cau_md5_hash_n:
    link    %a6,&-16                    # temp stack space for 4 regs
    movm.l  &0x04e0,(%sp)               # save d5/d6/d7/a2
    mov.l   (8,%a6),%a0                 # get 1st argument: *msg_data
    mov.l   (16,%a6),%a1                # get 3rd argument: *md5_state

    movm.l  (%a1),&0x00c3               # load a,b,c,d -> d0/d1/d6/d7
    byterev %d0
    byterev %d1
    byterev %d6
    byterev %d7
    cp0ld.l %d0,&LDR+CAA                # load byterev a -> CAA
    cp0ld.l %d1,&LDR+CA1                # load byterev b -> CA1
    cp0ld.l %d6,&LDR+CA2                # load byterev c -> CA2
    cp0ld.l %d7,&LDR+CA3                # load byterev d -> CA3

    mov.l   (12,%a6),%d0                # get 2nd argument: num_blks
    align   4
cau_md5_hash_n_L%0:
    lea     (md5_t.w,%pc),%a2           # pointer to t[*]

# 16 rounds with F()

    movq.l  &7,%d1                      # for rotating by 7
    movq.l  &12,%d5                     # for rotating by 12
    movq.l  &17,%d6                     # for rotating by 17
    movq.l  &22,%d7                     # for rotating by 22

    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[0]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[0]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[1]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[1]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[2]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[2]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[3]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[3]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[4]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[4]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[5]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[5]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[6]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[6]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[7]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[7]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[8]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[8]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[9]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[9]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[10]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[10]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[11]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[11]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[12]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[12]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[13]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[13]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[14]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[14]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[15]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[15]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
# 16 rounds with G()

    lea     (-60,%a0),%a0               # adjust pointer to x[1]: base+4
    movq.l  &5,%d1                      # for rotating by 5
    movq.l  &9,%d5                      # for rotating by 9
    movq.l  &14,%d6                     # for rotating by 14
    movq.l  &20,%d7                     # for rotating by 20

    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[1]
    lea     (20,%a0),%a0                # adjust pointer to x[6]: base+24
    cp0ld.l (%a2)+,&ADR+CAA             # add t[16]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[6]
    lea     (20,%a0),%a0                # adjust pointer to x[11]: base+44
    cp0ld.l (%a2)+,&ADR+CAA             # add t[17]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[11]
    lea     (-44,%a0),%a0               # adjust pointer to x[0]: base+0
    cp0ld.l (%a2)+,&ADR+CAA             # add t[18]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[0]
    lea     (20,%a0),%a0                # adjust pointer to x[5]: base+20
    cp0ld.l (%a2)+,&ADR+CAA             # add t[19]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[5]
    lea     (20,%a0),%a0                # adjust pointer to x[10]: base+40
    cp0ld.l (%a2)+,&ADR+CAA             # add t[20]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[10]
    lea     (20,%a0),%a0                # adjust pointer to x[15]: base+60
    cp0ld.l (%a2)+,&ADR+CAA             # add t[21]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[15]
    lea     (-44,%a0),%a0               # adjust pointer to x[4]: base+16
    cp0ld.l (%a2)+,&ADR+CAA             # add t[22]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[4]
    lea     (20,%a0),%a0                # adjust pointer to x[9]: base+36
    cp0ld.l (%a2)+,&ADR+CAA             # add t[23]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[9]
    lea     (20,%a0),%a0                # adjust pointer to x[14]: base+56
    cp0ld.l (%a2)+,&ADR+CAA             # add t[24]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[14]
    lea     (-44,%a0),%a0               # adjust pointer to x[3]: base+12
    cp0ld.l (%a2)+,&ADR+CAA             # add t[25]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[3]
    lea     (20,%a0),%a0                # adjust pointer to x[8]: base+32
    cp0ld.l (%a2)+,&ADR+CAA             # add t[26]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[8]
    lea     (20,%a0),%a0                # adjust pointer to x[13]: base+52
    cp0ld.l (%a2)+,&ADR+CAA             # add t[27]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[13]
    lea     (-44,%a0),%a0               # adjust pointer to x[2]: base+8
    cp0ld.l (%a2)+,&ADR+CAA             # add t[28]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[2]
    lea     (20,%a0),%a0                # adjust pointer to x[7]: base+28
    cp0ld.l (%a2)+,&ADR+CAA             # add t[29]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[7]
    lea     (20,%a0),%a0                # adjust pointer to x[12]: base+48
    cp0ld.l (%a2)+,&ADR+CAA             # add t[30]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[12]
    lea     (-28,%a0),%a0               # adjust pointer to x[5]: base+20
    cp0ld.l (%a2)+,&ADR+CAA             # add t[31]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
# 16 rounds with H()

    movq.l  &4,%d1                      # for rotating by 4
    movq.l  &11,%d5                     # for rotating by 11
    movq.l  &16,%d6                     # for rotating by 16
    movq.l  &23,%d7                     # for rotating by 23

    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[5]
    lea     (12,%a0),%a0                # adjust pointer to x[8]: base+32
    cp0ld.l (%a2)+,&ADR+CAA             # add t[32]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[8]
    lea     (12,%a0),%a0                # adjust pointer to x[11]: base+44
    cp0ld.l (%a2)+,&ADR+CAA             # add t[33]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[11]
    lea     (12,%a0),%a0                # adjust pointer to x[14]: base+56
    cp0ld.l (%a2)+,&ADR+CAA             # add t[34]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[14]
    lea     (-52,%a0),%a0               # adjust pointer to x[1]: base+4
    cp0ld.l (%a2)+,&ADR+CAA             # add t[35]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[1]
    lea     (12,%a0),%a0                # adjust pointer to x[4]: base+16
    cp0ld.l (%a2)+,&ADR+CAA             # add t[36]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[4]
    lea     (12,%a0),%a0                # adjust pointer to x[7]: base+28
    cp0ld.l (%a2)+,&ADR+CAA             # add t[37]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[7]
    lea     (12,%a0),%a0                # adjust pointer to x[10]: base+40
    cp0ld.l (%a2)+,&ADR+CAA             # add t[38]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[10]
    lea     (12,%a0),%a0                # adjust pointer to x[13]: base+52
    cp0ld.l (%a2)+,&ADR+CAA             # add t[39]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[13]
    lea     (-52,%a0),%a0               # adjust pointer to x[0]: base+0
    cp0ld.l (%a2)+,&ADR+CAA             # add t[40]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[0]
    lea     (12,%a0),%a0                # adjust pointer to x[3]: base+12
    cp0ld.l (%a2)+,&ADR+CAA             # add t[41]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[3]
    lea     (12,%a0),%a0                # adjust pointer to x[6]: base+24
    cp0ld.l (%a2)+,&ADR+CAA             # add t[42]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[6]
    lea     (12,%a0),%a0                # adjust pointer to x[9]: base+36
    cp0ld.l (%a2)+,&ADR+CAA             # add t[43]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[9]
    lea     (12,%a0),%a0                # adjust pointer to x[12]: base+48
    cp0ld.l (%a2)+,&ADR+CAA             # add t[44]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[12]
    lea     (12,%a0),%a0                # adjust pointer to x[15]: base+60
    cp0ld.l (%a2)+,&ADR+CAA             # add t[45]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[15]
    lea     (-52,%a0),%a0               # adjust pointer to x[2]: base+8
    cp0ld.l (%a2)+,&ADR+CAA             # add t[46]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[2]
    lea     (-8,%a0),%a0                # adjust pointer to x[0]: base+0
    cp0ld.l (%a2)+,&ADR+CAA             # add t[47]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
# 16 rounds with I()

    movq.l  &6,%d1                      # for rotating by 6
    movq.l  &10,%d5                     # for rotating by 10
    movq.l  &15,%d6                     # for rotating by 15
    movq.l  &21,%d7                     # for rotating by 21

    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[0]
    lea     (28,%a0),%a0                # adjust pointer to x[7]: base+28
    cp0ld.l (%a2)+,&ADR+CAA             # add t[48]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[7]
    lea     (28,%a0),%a0                # adjust pointer to x[14]: base+56
    cp0ld.l (%a2)+,&ADR+CAA             # add t[49]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[14]
    lea     (-36,%a0),%a0               # adjust pointer to x[5]: base+20
    cp0ld.l (%a2)+,&ADR+CAA             # add t[50]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[5]
    lea     (28,%a0),%a0                # adjust pointer to x[12]: base+48
    cp0ld.l (%a2)+,&ADR+CAA             # add t[51]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[12]
    lea     (-36,%a0),%a0               # adjust pointer to x[3]: base+12
    cp0ld.l (%a2)+,&ADR+CAA             # add t[52]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[3]
    lea     (28,%a0),%a0                # adjust pointer to x[10]: base+40
    cp0ld.l (%a2)+,&ADR+CAA             # add t[53]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[10]
    lea     (-36,%a0),%a0               # adjust pointer to x[1]: base+4
    cp0ld.l (%a2)+,&ADR+CAA             # add t[54]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[1]
    lea     (28,%a0),%a0                # adjust pointer to x[8]: base+32
    cp0ld.l (%a2)+,&ADR+CAA             # add t[55]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[8]
    lea     (28,%a0),%a0                # adjust pointer to x[15]: base+60
    cp0ld.l (%a2)+,&ADR+CAA             # add t[56]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[15]
    lea     (-36,%a0),%a0               # adjust pointer to x[6]: base+24
    cp0ld.l (%a2)+,&ADR+CAA             # add t[57]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[6]
    lea     (28,%a0),%a0                # adjust pointer to x[13]: base+52
    cp0ld.l (%a2)+,&ADR+CAA             # add t[58]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[13]
    lea     (-36,%a0),%a0               # adjust pointer to x[4]: base+16
    cp0ld.l (%a2)+,&ADR+CAA             # add t[59]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[4]
    lea     (28,%a0),%a0                # adjust pointer to x[11]: base+44
    cp0ld.l (%a2)+,&ADR+CAA             # add t[60]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[11]
    lea     (-36,%a0),%a0               # adjust pointer to x[2]: base+8
    cp0ld.l (%a2)+,&ADR+CAA             # add t[61]
    cp0ld.l %d5,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[2]
    lea     (28,%a0),%a0                # adjust pointer to x[9]: base+36
    cp0ld.l (%a2)+,&ADR+CAA             # add t[62]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[9]
    lea     (28,%a0),%a0                # adjust pointer to next msg_data block
    cp0ld.l (%a2)+,&ADR+CAA             # add t[63]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l (%a1)+,&RADR+CAA            # add byterev a
    cp0ld.l (%a1)+,&RADR+CA1            # add byterev b
    cp0ld.l (%a1)+,&RADR+CA2            # add byterev c
    cp0ld.l (%a1)+,&RADR+CA3            # add byterev d

    cp0st.l %d1,&STR+CAA                # store a
    cp0st.l %d5,&STR+CA1                # store b
    cp0st.l %d6,&STR+CA2                # store c
    cp0st.l %d7,&STR+CA3                # store d
    byterev %d1
    byterev %d5
    byterev %d6
    byterev %d7
    lea     (-16,%a1),%a1
    movm.l  &0x00e2,(%a1)               # store byterev a,b,c,d

    subq.l  &1,%d0                      # decrement block counter
    bne.w   cau_md5_hash_n_L%0

    movm.l  (%sp),&0x04e0               # restore d5/d6/d7/a2
    unlk    %a6
    rts


#*******************************************************************************
#*******************************************************************************
#
# MD5: Updates MD5 state variables for one or more input message blocks
#
#   arguments
#           *msg_data   pointer to start of input message data
#           num_blks    number of 512-bit blocks to process
#           *md5_state  pointer to 128-bit block of MD5 state variables:
#                           a,b,c,d
#
#   calling convention
#   void    cau_md5_update (const unsigned char *msg_data,
#                           const int            num_blks,
#                           unsigned char       *md5_state)

    global  _cau_md5_update
    global  cau_md5_update
    align   4
_cau_md5_update:
cau_md5_update:
    mov.l   (12,%sp),%a0                # get 3rd argument: *md5_state
    lea     (md5_init_h.w,%pc),%a1      # pointer to initial data

# copy initial data into hash output buffer (16 bytes = 4 longwords)
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+
    mov.l   (%a1)+,(%a0)+

    bra.w   cau_md5_hash_n


#*******************************************************************************
#*******************************************************************************
#
# MD5: Perform the hash for one input message block and generate the
#      message digest output
#
#   arguments
#           *msg_data   pointer to start of input message data
#           *md5_state  pointer to 128-bit block of MD5 state variables: a,b,c,d
#
#   calling convention
#   void    cau_md5_hash (const unsigned char *msg_data,
#                         unsigned char       *md5_state)

    global  _cau_md5_hash
    global  cau_md5_hash
    align   4
_cau_md5_hash:
cau_md5_hash:
    link    %a6,&-12                    # temp stack space for 3 regs
    movm.l  &0x04c0,(%sp)               # save d6/d7/a2
    mov.l   (8,%a6),%a0                 # get 1st argument: *msg_data
    mov.l   (12,%a6),%a1                # get 2nd argument: *md5_state

    movm.l  (%a1),&0x00c3               # load a,b,c,d -> d0/d1/d6/d7
    byterev %d0
    byterev %d1
    byterev %d6
    byterev %d7
    cp0ld.l %d0,&LDR+CAA                # load byterev a -> CAA
    cp0ld.l %d1,&LDR+CA1                # load byterev b -> CA1
    cp0ld.l %d6,&LDR+CA2                # load byterev c -> CA2
    cp0ld.l %d7,&LDR+CA3                # load byterev d -> CA3

# 16 rounds with F()

    lea     (md5_t.w,%pc),%a2           # pointer to t[*]
    movq.l  &7,%d1                      # for rotating by 7
    movq.l  &12,%d0                     # for rotating by 12
    movq.l  &17,%d6                     # for rotating by 17
    movq.l  &22,%d7                     # for rotating by 22

    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[0]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[0]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[1]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[1]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[2]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[2]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[3]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[3]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[4]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[4]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[5]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[5]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[6]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[6]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[7]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[7]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[8]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[8]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[9]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[9]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[10]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[10]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[11]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[11]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(b,c,d)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[12]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[12]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 7
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(a,b,c)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[13]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[13]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 12
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(d,a,b)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[14]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[14]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 17
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFF                   # add F(c,d,a)
    cp0ld.l (%a0)+,&RADR+CAA            # add byterev x[15]
    cp0ld.l (%a2)+,&ADR+CAA             # add t[15]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 22
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
# 16 rounds with G()

    lea     (-60,%a0),%a0               # adjust pointer to x[1]: base+4
    movq.l  &5,%d1                      # for rotating by 5
    movq.l  &9,%d0                      # for rotating by 9
    movq.l  &14,%d6                     # for rotating by 14
    movq.l  &20,%d7                     # for rotating by 20

    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[1]
    lea     (20,%a0),%a0                # adjust pointer to x[6]: base+24
    cp0ld.l (%a2)+,&ADR+CAA             # add t[16]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[6]
    lea     (20,%a0),%a0                # adjust pointer to x[11]: base+44
    cp0ld.l (%a2)+,&ADR+CAA             # add t[17]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[11]
    lea     (-44,%a0),%a0               # adjust pointer to x[0]: base+0
    cp0ld.l (%a2)+,&ADR+CAA             # add t[18]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[0]
    lea     (20,%a0),%a0                # adjust pointer to x[5]: base+20
    cp0ld.l (%a2)+,&ADR+CAA             # add t[19]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[5]
    lea     (20,%a0),%a0                # adjust pointer to x[10]: base+40
    cp0ld.l (%a2)+,&ADR+CAA             # add t[20]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[10]
    lea     (20,%a0),%a0                # adjust pointer to x[15]: base+60
    cp0ld.l (%a2)+,&ADR+CAA             # add t[21]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[15]
    lea     (-44,%a0),%a0               # adjust pointer to x[4]: base+16
    cp0ld.l (%a2)+,&ADR+CAA             # add t[22]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[4]
    lea     (20,%a0),%a0                # adjust pointer to x[9]: base+36
    cp0ld.l (%a2)+,&ADR+CAA             # add t[23]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[9]
    lea     (20,%a0),%a0                # adjust pointer to x[14]: base+56
    cp0ld.l (%a2)+,&ADR+CAA             # add t[24]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[14]
    lea     (-44,%a0),%a0               # adjust pointer to x[3]: base+12
    cp0ld.l (%a2)+,&ADR+CAA             # add t[25]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[3]
    lea     (20,%a0),%a0                # adjust pointer to x[8]: base+32
    cp0ld.l (%a2)+,&ADR+CAA             # add t[26]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[8]
    lea     (20,%a0),%a0                # adjust pointer to x[13]: base+52
    cp0ld.l (%a2)+,&ADR+CAA             # add t[27]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[13]
    lea     (-44,%a0),%a0               # adjust pointer to x[2]: base+8
    cp0ld.l (%a2)+,&ADR+CAA             # add t[28]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 5
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[2]
    lea     (20,%a0),%a0                # adjust pointer to x[7]: base+28
    cp0ld.l (%a2)+,&ADR+CAA             # add t[29]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 9
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[7]
    lea     (20,%a0),%a0                # adjust pointer to x[12]: base+48
    cp0ld.l (%a2)+,&ADR+CAA             # add t[30]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 14
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFG                   # add G(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[12]
    lea     (-28,%a0),%a0               # adjust pointer to x[5]: base+20
    cp0ld.l (%a2)+,&ADR+CAA             # add t[31]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 20
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
# 16 rounds with H()

    movq.l  &4,%d1                      # for rotating by 4
    movq.l  &11,%d0                     # for rotating by 11
    movq.l  &16,%d6                     # for rotating by 16
    movq.l  &23,%d7                     # for rotating by 23

    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[5]
    lea     (12,%a0),%a0                # adjust pointer to x[8]: base+32
    cp0ld.l (%a2)+,&ADR+CAA             # add t[32]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[8]
    lea     (12,%a0),%a0                # adjust pointer to x[11]: base+44
    cp0ld.l (%a2)+,&ADR+CAA             # add t[33]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[11]
    lea     (12,%a0),%a0                # adjust pointer to x[14]: base+56
    cp0ld.l (%a2)+,&ADR+CAA             # add t[34]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[14]
    lea     (-52,%a0),%a0               # adjust pointer to x[1]: base+4
    cp0ld.l (%a2)+,&ADR+CAA             # add t[35]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[1]
    lea     (12,%a0),%a0                # adjust pointer to x[4]: base+16
    cp0ld.l (%a2)+,&ADR+CAA             # add t[36]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[4]
    lea     (12,%a0),%a0                # adjust pointer to x[7]: base+28
    cp0ld.l (%a2)+,&ADR+CAA             # add t[37]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[7]
    lea     (12,%a0),%a0                # adjust pointer to x[10]: base+40
    cp0ld.l (%a2)+,&ADR+CAA             # add t[38]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[10]
    lea     (12,%a0),%a0                # adjust pointer to x[13]: base+52
    cp0ld.l (%a2)+,&ADR+CAA             # add t[39]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[13]
    lea     (-52,%a0),%a0               # adjust pointer to x[0]: base+0
    cp0ld.l (%a2)+,&ADR+CAA             # add t[40]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[0]
    lea     (12,%a0),%a0                # adjust pointer to x[3]: base+12
    cp0ld.l (%a2)+,&ADR+CAA             # add t[41]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[3]
    lea     (12,%a0),%a0                # adjust pointer to x[6]: base+24
    cp0ld.l (%a2)+,&ADR+CAA             # add t[42]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[6]
    lea     (12,%a0),%a0                # adjust pointer to x[9]: base+36
    cp0ld.l (%a2)+,&ADR+CAA             # add t[43]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[9]
    lea     (12,%a0),%a0                # adjust pointer to x[12]: base+48
    cp0ld.l (%a2)+,&ADR+CAA             # add t[44]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 4
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[12]
    lea     (12,%a0),%a0                # adjust pointer to x[15]: base+60
    cp0ld.l (%a2)+,&ADR+CAA             # add t[45]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 11
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[15]
    lea     (-52,%a0),%a0               # adjust pointer to x[2]: base+8
    cp0ld.l (%a2)+,&ADR+CAA             # add t[46]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 16
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFH                   # add H(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[2]
    lea     (-8,%a0),%a0                # adjust pointer to x[0]: base+0
    cp0ld.l (%a2)+,&ADR+CAA             # add t[47]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 23
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
# 16 rounds with I()

    movq.l  &6,%d1                      # for rotating by 6
    movq.l  &10,%d0                     # for rotating by 10
    movq.l  &15,%d6                     # for rotating by 15
    movq.l  &21,%d7                     # for rotating by 21

    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[0]
    lea     (28,%a0),%a0                # adjust pointer to x[7]: base+28
    cp0ld.l (%a2)+,&ADR+CAA             # add t[48]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[7]
    lea     (28,%a0),%a0                # adjust pointer to x[14]: base+56
    cp0ld.l (%a2)+,&ADR+CAA             # add t[49]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[14]
    lea     (-36,%a0),%a0               # adjust pointer to x[5]: base+20
    cp0ld.l (%a2)+,&ADR+CAA             # add t[50]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[5]
    lea     (28,%a0),%a0                # adjust pointer to x[12]: base+48
    cp0ld.l (%a2)+,&ADR+CAA             # add t[51]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[12]
    lea     (-36,%a0),%a0               # adjust pointer to x[3]: base+12
    cp0ld.l (%a2)+,&ADR+CAA             # add t[52]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[3]
    lea     (28,%a0),%a0                # adjust pointer to x[10]: base+40
    cp0ld.l (%a2)+,&ADR+CAA             # add t[53]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[10]
    lea     (-36,%a0),%a0               # adjust pointer to x[1]: base+4
    cp0ld.l (%a2)+,&ADR+CAA             # add t[54]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[1]
    lea     (28,%a0),%a0                # adjust pointer to x[8]: base+32
    cp0ld.l (%a2)+,&ADR+CAA             # add t[55]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[8]
    lea     (28,%a0),%a0                # adjust pointer to x[15]: base+60
    cp0ld.l (%a2)+,&ADR+CAA             # add t[56]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[15]
    lea     (-36,%a0),%a0               # adjust pointer to x[6]: base+24
    cp0ld.l (%a2)+,&ADR+CAA             # add t[57]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[6]
    lea     (28,%a0),%a0                # adjust pointer to x[13]: base+52
    cp0ld.l (%a2)+,&ADR+CAA             # add t[58]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[13]
    lea     (-36,%a0),%a0               # adjust pointer to x[4]: base+16
    cp0ld.l (%a2)+,&ADR+CAA             # add t[59]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(b,c,d)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[4]
    lea     (28,%a0),%a0                # adjust pointer to x[11]: base+44
    cp0ld.l (%a2)+,&ADR+CAA             # add t[60]
    cp0ld.l %d1,&ROTL+CAA               # rotate by 6
    cp0ld.l &ADRA+CA1                   # add b
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(a,b,c)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[11]
    lea     (-36,%a0),%a0               # adjust pointer to x[2]: base+8
    cp0ld.l (%a2)+,&ADR+CAA             # add t[61]
    cp0ld.l %d0,&ROTL+CAA               # rotate by 10
    cp0ld.l &ADRA+CA1                   # add a
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(d,a,b)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[2]
    lea     (28,%a0),%a0                # adjust pointer to x[9]: base+36
    cp0ld.l (%a2)+,&ADR+CAA             # add t[62]
    cp0ld.l %d6,&ROTL+CAA               # rotate by 15
    cp0ld.l &ADRA+CA1                   # add d
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l &HASH+HFI                   # add I(c,d,a)
    cp0ld.l (%a0),&RADR+CAA             # add byterev x[9]
    lea     (28,%a0),%a0                # adjust pointer to next msg_data block
    cp0ld.l (%a2)+,&ADR+CAA             # add t[63]
    cp0ld.l %d7,&ROTL+CAA               # rotate by 21
    cp0ld.l &ADRA+CA1                   # add c
    cp0ld.l &MDS                        # shift registers
    
    cp0ld.l (%a1)+,&RADR+CAA            # add byterev a
    cp0ld.l (%a1)+,&RADR+CA1            # add byterev b
    cp0ld.l (%a1)+,&RADR+CA2            # add byterev c
    cp0ld.l (%a1)+,&RADR+CA3            # add byterev d

    cp0st.l %d0,&STR+CAA                # store a
    cp0st.l %d1,&STR+CA1                # store b
    cp0st.l %d6,&STR+CA2                # store c
    cp0st.l %d7,&STR+CA3                # store d
    byterev %d0
    byterev %d1
    byterev %d6
    byterev %d7
    lea     (-16,%a1),%a1
    movm.l  &0x00c3,(%a1)               # store byterev a,b,c,d

    movm.l  (%sp),&0x04c0               # restore d6/d7/a2
    unlk    %a6
    rts


#*******************************************************************************
#*******************************************************************************
#
# CAU Constant Data

    align   16
md5_init_h:
    long    0x01234567                  # initialize a
    long    0x89abcdef                  # initialize b
    long    0xfedcba98                  # initialize c
    long    0x76543210                  # initialize d

    align   16
md5_t:
    long    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee
    long    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501
    long    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be
    long    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821
    long    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa
    long    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8
    long    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed
    long    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a
    long    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c
    long    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70
    long    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05
    long    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665
    long    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039
    long    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1
    long    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1
    long    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
