#include "fileUtils.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

std::string readFileToString(const std::string &filepathStr) {
    fs::path filepath(filepathStr);  // Convert std::string to fs::path.
    std::ifstream fileStream(filepath, std::ios::in | std::ios::binary);
    if (!fileStream) {
        throw std::runtime_error("Could not open file: " + filepath.string());
    }
    std::ostringstream ss;
    ss << fileStream.rdbuf();
    return ss.str();
}
