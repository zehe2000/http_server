#include "compression.hpp"
#include <zlib.h>
#include <cstring>
#include <stdexcept>
#include <string>

std::string gzipCompress(const std::string &str) {
    // z_stream is the structure zlib uses for compression.
    z_stream zs;
    std::memset(&zs, 0, sizeof(zs));

    // Initialize zlib for compression.
    // The windowBits parameter of 15 + 16 tells zlib to create a gzip header.
    if (deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        throw std::runtime_error("deflateInit2 failed while compressing.");
    }

    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(str.data()));
    zs.avail_in = str.size();

    int ret;
    char outbuffer[32768];
    std::string outstring;

    // Compress the data in chunks.
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);

        ret = deflate(&zs, Z_FINISH);

        if (outstring.size() < zs.total_out) {
            // Append the newly compressed data to outstring.
            outstring.append(outbuffer, zs.total_out - outstring.size());
        }
    } while (ret == Z_OK);

    deflateEnd(&zs);

    if (ret != Z_STREAM_END) {  // Check if the compression ended successfully.
        throw std::runtime_error("Exception during zlib compression: " + std::to_string(ret));
    }
    
    return outstring;
}
