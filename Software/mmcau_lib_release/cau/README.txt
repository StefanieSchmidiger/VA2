The cau optimized assembly library is contained in 5 files
(18 functions) and archived in the cau_lib.a file. The 
calling conventions for the cau functions follow:
    
cau_aes_functions.s:
   cau_aes_set_key (const unsigned int *key, const int key_size, 
                    unsigned int *key_sch)
   cau_aes_encrypt (const unsigned int *in, const unsigned int *key_sch, 
                    const int nr, unsigned int *out)
   cau_aes_decrypt (const unsigned int *in, const unsigned int *key_sch, 
                    const int nr, unsigned int *out)
    
cau_des_functions.s:
   cau_des_chk_parity (const unsigned int *key)
   cau_des_encrypt    (const unsigned int *in, const unsigned int *key, 
                       unsigned int *out)
   cau_des_decrypt    (const unsigned int *in, const unsigned int *key, 
                       unsigned int *out)
    
cau_md5_functions.s:
   cau_md5_initialize_output (const unsigned int *md5_state)
   cau_md5_hash_n (const unsigned int *msg_data, const int num_blks, 
                   unsigned int *md5_state)
   cau_md5_update (const unsigned int *msg_data, const int num_blks, 
                   unsigned int *md5_state)
   cau_md5_hash   (const unsigned int *msg_data, unsigned int *md5_state)
    
cau_sha1_functions.s:
   cau_sha1_initialize_output (const unsigned int *sha1_state)
   cau_sha1_hash_n (const unsigned int *msg_data, const int num_blks, 
                    unsigned int *sha1_state)
   cau_sha1_update (const unsigned int *msg_data, const int num_blks, 
                    unsigned int *sha1_state)
   cau_sha1_hash   (const unsigned int *msg_data, 
                    unsigned int *sha1_state)
    
cau_sha256_functions.s:
   cau_sha256_initialize_output (const unsigned int *output)
   cau_sha256_hash_n (const unsigned int *input, const int num_blks, 
                      unsigned int *output)
   cau_sha256_update (const unsigned int *input, const int num_blks, 
                      unsigned int *output)
   cau_sha256_hash   (const unsigned int *input, unsigned int *output)


Assembly Language Format and Porting Considerations

These functions are written in the ColdFire assembler used by the Core &
Platform design team. This assembler may not be compatible with the wide
assortment of other development tools, but typically can be easily translated
into the required format.

One particular item related to porting deserved mention: compare instructions.
Note that there are only three cmp instructions in the code and
they are all compare-immediate, so there should be no problem with
assemblers generating bad code. Depending on the assembler used, the
instructions may simply generate an error message, in which case the
arguments would have to be reversed to assemble.

The only time ambiguity ever arises is when the cmp instruction
compares two registers and the following branch instruction is
something other than "eq" or "ne".
