//
//  img2tool.cpp
//  img2tool
//
//  Created by tihmstar on 29.04.23.
//

#include <img2tool/img2tool.hpp>
#include <libgeneral/macros.h>

extern "C"{
#include "crc32.h"
};

using namespace tihmstar;
using namespace tihmstar::img2tool;

struct Img2Ext{
    uint32_t crc32;
    uint32_t nextExtSize;
    uint32_t type;
    uint32_t options;
    uint8_t  data[0];
};

#define kIMG2OptionHasExtension (1<<30)

struct Img2{
    uint32_t magic; //'Img2'
    uint32_t identifier;
    uint16_t unknown1;
    uint16_t epoch;
    uint32_t loadaddr;
    uint32_t dataSize;
    uint32_t decSize;
    uint32_t unknown2;
    uint32_t options;
    uint32_t sig[16];
    uint32_t nextExtSize;
    uint32_t crc32;
    union {
        Img2Ext ext[0];
        uint8_t pad[0x398];
    };
};

#pragma mark private
static void DumpHex(const void* data, size_t size) {
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        printf("%02X ", ((unsigned char*)data)[i]);
        if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char*)data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i+1) % 8 == 0 || i+1 == size) {
            printf(" ");
            if ((i+1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i+1 == size) {
                ascii[(i+1) % 16] = '\0';
                if ((i+1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i+1) % 16; j < 16; ++j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

static uint32_t swap32(uint32_t v){
    return    ((v & 0xff000000) >> 24)
            | ((v & 0x00ff0000) >> 8)
            | ((v & 0x0000ff00) << 8)
            | ((v & 0x000000ff) << 24);
}

static Img2 * verifyIMG2Header(const void *buf, size_t size){
    retassure(size >= sizeof(Img2), "buf too small for header");
    Img2 *header = (Img2*)buf;
    uint32_t crc=0;

    retassure(header->magic == 'Img2', "Bad magic! Got %d but expected %d",header->magic, 'Img2');
    retassure(sizeof(Img2) <= size, "header size larger than buffer");
    retassure(header->dataSize + sizeof(Img2) <= size, "data size larger than buffer");
    retassure(header->decSize + sizeof(Img2) <= size,  "data size larger than buffer");
    crc = crc32((uint8_t*)header, offsetof(Img2, crc32));
    
    retassure(header->crc32 == crc, "header CRC32 mismatch");

    Img2Ext *ext = header->ext;
    if (header->options & kIMG2OptionHasExtension){
        uint8_t *data = (uint8_t*)ext;
        int i = 0;
        for (uint32_t extDataSize = header->nextExtSize;
             extDataSize && extDataSize != 0xffffffff;
             i++, data += extDataSize + sizeof(Img2Ext), extDataSize = ext->nextExtSize, ext = (Img2Ext*)data) {

            crc = crc32(((uint8_t*)ext) + sizeof(uint32_t), sizeof(Img2Ext)-sizeof(uint32_t)+extDataSize);
            retassure(ext->crc32 == crc, "ext %d CRC32 mismatch",i);
            if (!(ext->options & kIMG2OptionHasExtension)) break;
        }
    }
    return header;
}


#pragma mark public
const char *img2tool::version(){
    return VERSION_STRING;
}

void img2tool::printIMG2(const void *buf, size_t size){
    Img2 *header = verifyIMG2Header(buf, size);
    
    printf("IMG2:\n");
    {
        uint32_t v = 0;
        v = swap32(header->magic);      printf("magic        : %.4s\n",(char*)&v);
        v = swap32(header->identifier); printf("type         : %.4s\n",(char*)&v);
                                        printf("epoch        : 0x%x\n",header->epoch);
                                        printf("loadaddr     : 0x%08x\n",header->loadaddr);
                                        printf("dataSize     : 0x%08x\n",header->dataSize);
                                        printf("decSize      : 0x%08x\n",header->decSize);
                                        printf("options      : 0x%08x\n",header->options);
                                        printf("nextExtSize  : 0x%08x\n",header->nextExtSize);
                                        printf("crc32        : 0x%08x\n",header->crc32);
    }
    {
        const uint8_t *p = (const uint8_t*)(header->sig);
        printf("sig:");
        for (size_t i = 0; i<sizeof(header->sig); i++){
            if ((i%0x20) == 0) printf("\n\t");
            printf("%02x",p[i]);
        }
        printf("\n");
    }
    printf("-------------------------\n");

    Img2Ext *ext = header->ext;
    if (header->options & kIMG2OptionHasExtension){
        uint8_t *data = (uint8_t*)ext;
        int i = 0;
        for (uint32_t extDataSize = header->nextExtSize;
             extDataSize && extDataSize != 0xffffffff;
             i++, data += extDataSize + sizeof(Img2Ext), extDataSize = ext->nextExtSize, ext = (Img2Ext*)data) {
            
            uint32_t v = 0;
            printf("Ext %d:\n",i);
                                            printf("\tcrc32       : 0x%08x\n",ext->crc32);
                                            printf("\tnextExtSize : 0x%08x\n",ext->nextExtSize);
            v = swap32(ext->type);          printf("\ttype        : %.4s\n",(char*)&v);
                                            printf("\toptions     : 0x%08x\n",ext->options);
            DumpHex(ext->data, extDataSize);
            printf("-------------------------\n");
            if (!(ext->options & kIMG2OptionHasExtension)) break;
        }
    }
}

std::vector<uint8_t> img2tool::getPayloadFromIMG2(const void *buf, size_t size){
    Img2 *header = verifyIMG2Header(buf, size);
    return {(uint8_t*)(header+1),(uint8_t*)(header+1)+header->dataSize};
}

std::vector<uint8_t> img2tool::createIMG2FromPayloadWithType(const std::vector<uint8_t> &payload, uint32_t type, std::vector<Img2Extension> extensions){
    Img2 header = {
        .magic = 'Img2',
        .identifier = type,
        .unknown1 = 0,
        .epoch = 2,
        .loadaddr = 0,
        .dataSize = static_cast<uint32_t>(payload.size()),
        .decSize = static_cast<uint32_t>(payload.size()),
        .unknown2 = 0xffffffff,
        .options = 0,
        .sig = {},
        .nextExtSize = 0xffffffff,
    };
    header.crc32 = crc32((uint8_t*)&header, offsetof(Img2, crc32));
    
    {
        uint32_t prevExtSize = 0;
        Img2Ext *lastExt = header.ext;
        for (auto ext : extensions){
            size_t remainingExtSize = sizeof(Img2)-offsetof(Img2, ext)-((uint8_t*)lastExt-(uint8_t*)header.ext);
            retassure(sizeof(Img2Ext) + ext.data.size() < remainingExtSize, "Out of extension memory!");
            if (!(header.options & kIMG2OptionHasExtension)){
                //first extension, fixup header
                header.options |= kIMG2OptionHasExtension;
                header.nextExtSize = (uint32_t)ext.data.size();
                header.crc32 = crc32((uint8_t*)&header, offsetof(Img2, crc32));
            }else{
                //fixup previous extension
                lastExt->options |= kIMG2OptionHasExtension;
                lastExt->nextExtSize = (uint32_t)ext.data.size();
                lastExt->crc32 = crc32(((uint8_t*)lastExt)+sizeof(uint32_t), prevExtSize+sizeof(Img2Ext)-sizeof(uint32_t));
                //now move lastExt to the current Ext
                lastExt = (Img2Ext*)(((uint8_t*)lastExt)+prevExtSize+sizeof(Img2Ext));
            }
            lastExt->nextExtSize = 0xffffffff;
            lastExt->type = ext.tag;
            memcpy(lastExt->data, ext.data.data(), ext.data.size());
            ((uint8_t*)lastExt->data)[ext.data.size()] = 0xff; //end marker??
            prevExtSize = (uint32_t)ext.data.size();
            lastExt->crc32 = crc32((uint8_t*)lastExt+sizeof(uint32_t), prevExtSize+sizeof(Img2Ext)-sizeof(uint32_t));
        }
    }
    
    std::vector<uint8_t> ret{(uint8_t*)&header,((uint8_t*)&header)+sizeof(header)};
    ret.insert(ret.end(), payload.begin(),payload.end());
    
    {
        size_t retsize = ret.size();
        if (retsize & 0x3ff){
            //pad to iPhone pagesize??
            retsize &= ~0x3ff;
            retsize += 0x400;
            ret.resize(retsize);
        }
    }
    return ret;
}
