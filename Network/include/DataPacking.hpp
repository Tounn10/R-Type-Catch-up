/*
** EPITECH PROJECT, 2025
** R-Type [WSL: Ubuntu]
** File description:
** DataPacking
*/

#ifndef DATAPACKING_HPP
#define DATAPACKING_HPP

#include <string>
#include <sstream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/copy.hpp>

class DataPacking {
public:
    static std::string compressData(const std::string& data) {
        std::stringstream compressed;
        std::stringstream original(data);

        boost::iostreams::filtering_streambuf<boost::iostreams::output> out;
        out.push(boost::iostreams::gzip_compressor());
        out.push(compressed);

        boost::iostreams::copy(original, out);
        return compressed.str();
    }

    static std::string decompressData(const std::string& compressed) {
        std::stringstream decompressed;
        std::stringstream compressedStream(compressed);

        boost::iostreams::filtering_streambuf<boost::iostreams::input> in;
        in.push(boost::iostreams::gzip_decompressor());
        in.push(compressedStream);

        boost::iostreams::copy(in, decompressed);
        return decompressed.str();
    }
};

#endif //DATAPACKING_HPP
