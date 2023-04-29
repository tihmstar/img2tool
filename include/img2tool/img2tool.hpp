//
//  img2tool.hpp
//  img2tool
//
//  Created by tihmstar on 29.04.23.
//

#ifndef img2tool_hpp
#define img2tool_hpp

#include <stdlib.h>
#include <stdint.h>
#include <vector>

namespace tihmstar {
    namespace img2tool {
        const char *version();

        struct Img2Extension{
            uint32_t tag;
            std::vector<uint8_t> data;
        };
    
        void printIMG2(const void *buf, size_t size);
        std::vector<uint8_t> getPayloadFromIMG2(const void *buf, size_t size);
        std::vector<uint8_t> createIMG2FromPayloadWithType(const std::vector<uint8_t> &payload, uint32_t type, std::vector<Img2Extension> extensions = {});
    };
};


#endif /* img2tool_hpp */
