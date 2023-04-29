//
//  main.cpp
//  img2tool
//
//  Created by tihmstar on 29.04.23.
//

#include <libgeneral/macros.h>
#include "../include/img2tool/img2tool.hpp"

#include <iostream>
#include <getopt.h>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

using namespace tihmstar::img2tool;

#define FLAG_CREATE         (1 << 0)
#define FLAG_EXTRACT        (1 << 1)

static struct option longopts[] = {
    { "help",           no_argument,        NULL, 'h' },
    { "create",         required_argument,  NULL, 'c' },
    { "extract",        no_argument,        NULL, 'e' },
    { "extension",      required_argument,  NULL, 'E' },
    { "outfile",        required_argument,  NULL, 'o' },
    { "type",           required_argument,  NULL, 't' },
    { NULL, 0, NULL, 0 }
};

void cmd_help(){
    printf("Usage: img2tool [OPTIONS] FILE\n");
    printf("Parses img2 files\n\n");
    printf("  -h, --help\t\t\t\tprints usage information\n");
    printf("  -c, --create\t<PATH>\t\t\tcreates img3 with raw file (last argument)\n");
    printf("  -e, --extract\t\t\t\textracts payload\n");
    printf("  -E, --extension <type><s><data>\tAdd img2 extension (can be specified multiple times)\n");
    printf("               \tWhere <type> is 4 chars and <data> is either hexbytes when <s> is '-', or \"string\" when <s> is '='\n");
    printf("               \te.g -E vers=\"hello world\"\n");
    printf("               \te.g -E vers-41424344\n");
    printf("  -o, --outfile\t<PATH>\t\t\toutput path for extracting payload\n");
    printf("  -t, --type\t<TYPE>\t\t\tset type for creating IMG2 files from raw (TYPE must be exactly 4 bytes)\n");
    printf("\n");
}

std::vector<uint8_t> readFromFile(const char *filePath){
    int fd = -1;
    cleanup([&]{
        safeClose(fd);
    });
    struct stat st{};
    std::vector<uint8_t> ret;
    
    retassure((fd = open(filePath, O_RDONLY))>0, "Failed to open '%s'",filePath);
    retassure(!fstat(fd, &st), "Failed to stat file");
    ret.resize(st.st_size);
    retassure(read(fd, ret.data(), ret.size()) == ret.size(), "Failed to read file");
    return ret;
}

void saveToFile(const char *filePath, std::vector<uint8_t>data){
    int fd = -1;
    cleanup([&]{
        safeClose(fd);
    });
    retassure((fd = open(filePath, O_WRONLY | O_CREAT | O_TRUNC, 0644))>0, "failed to create file '%s'",filePath);
    retassure(write(fd, data.data(), data.size()) == data.size(), "failed to write to file");
}

std::vector<uint8_t> parseHexbytes(const char *bytes){
    std::vector<uint8_t> ret;
    for (;*bytes;bytes+=2) {
        unsigned int t;
        retassure(bytes[1],"odd hex string");
        retassure(sscanf(bytes,"%02x",&t) == 1,"Failed paring hexstring");
        ret.push_back(t);
    }
    return ret;
}

Img2Extension parseExtension(const char *str){
    Img2Extension ret;
    size_t len = strlen(str);
    retassure(len > 5, "Malformed extension argument");
    ret.tag = htonl(*(uint32_t*)str);
    const char *extdata = str+5;
    
    if (str[4] == '=') {
        ret.data = {extdata,extdata+strlen(extdata)+1};
    }else if (str[4] == '-'){
        ret.data = parseHexbytes(extdata);
    }else{
        reterror("invalid selector");
    }
    return ret;
}

MAINFUNCTION
int main_r(int argc, const char * argv[]) {
    info("%s",version());

    const char *lastArg = NULL;
    const char *outFile = NULL;
    const char *img2Type = NULL;
    
    std::vector<Img2Extension> extensions;
    
    int optindex = 0;
    int opt = 0;
    long flags = 0;

    while ((opt = getopt_long(argc, (char* const *)argv, "hc:eE:o:t:", longopts, &optindex)) >= 0) {
        switch (opt) {
            case 'h':
                cmd_help();
                return 0;
            case 'c':
                flags |= FLAG_CREATE;
                retassure(!(flags & FLAG_EXTRACT), "Invalid command line arguments. can't extract and create at the same time");
                retassure(!outFile, "Invalid command line arguments. outFile already set!");
                outFile = optarg;
                break;
            case 'e':
                retassure(!(flags & FLAG_CREATE), "Invalid command line arguments. can't extract and create at the same time");
                flags |= FLAG_EXTRACT;
                break;
            case 'E':
                extensions.push_back(parseExtension(optarg));
                break;
            case 'o':
                retassure(!outFile, "Invalid command line arguments. outFile already set!");
                outFile = optarg;
                break;
            case 't':
                retassure(!img2Type, "Invalid command line arguments. img2Type already set!");
                retassure(strlen(optarg) == 4, "Invalid command line arguments. strlen(type) != 4");
                img2Type = optarg;
                break;
            default:
                cmd_help();
                return -1;
        }
    }

    if (argc-optind == 1) {
        argc -= optind;
        argv += optind;
        lastArg = argv[0];
    }else{
        if (!(flags & FLAG_CREATE)) {
            cmd_help();
            return -2;
        }
    }
    
    std::vector<uint8_t> workingBuf;

    if (lastArg) {
        workingBuf = readFromFile(lastArg);
    }

    
    if (flags & FLAG_EXTRACT) {
        retassure(outFile, "Outfile required for operation");
        auto outdata = getPayloadFromIMG2(workingBuf.data(),workingBuf.size());
        saveToFile(outFile, outdata);
        info("Extracted IMG3 payload to %s",outFile);
    }else if (flags & FLAG_CREATE) {
        retassure(outFile, "Outfile required for operation");
        std::vector<uint8_t> img2;
        img2 = createIMG2FromPayloadWithType(workingBuf,htonl(*(uint32_t*)img2Type), extensions);
        saveToFile(outFile, img2);
        info("Created IMG2 file at %s",outFile);
    } else{
        //print
        printIMG2(workingBuf.data(), workingBuf.size());
    }

    
    return 0;
}
