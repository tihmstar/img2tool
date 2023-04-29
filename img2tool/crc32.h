//
//  crc32.h
//  img1tool
//
//  Created by tihmstar on 31.03.23.
//

#ifndef crc32_h
#define crc32_h

//----- Type defines ----------------------------------------------------------
typedef unsigned char      byte;    // Byte is a char
typedef unsigned short int word16;  // 16-bit word is a short int
typedef unsigned int       word32;  // 32-bit word is an int
//----- Prototypes ------------------------------------------------------------
word32 update_crc(word32 crc_accum, byte *data_blk_ptr, word32 data_blk_size);
word32 crc32(byte *data_blk_ptr, word32 data_blk_size);

#endif /* crc32_h */
