#include "sam/utils/vec_io.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <itpp/itbase.h>

#define THROW_ERROR(condition, message) \
    do { \
        if (condition) { \
            throw std::runtime_error(message); \
        } \
    } while (false)


itpp::cvec read_sc16_as_cvec(const std::string& filename, size_t fix_point, size_t num_samples) {
    THROW_ERROR(fix_point > 15, "fix_point must be in [0, 15] for int16.");

    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    THROW_ERROR(!file.is_open(), "Could not open binary file: " + filename);

    const std::streamsize file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    THROW_ERROR(file_size % (2 * sizeof(int16_t)) != 0, "File size not a multiple of sc16 sample size (4 bytes).");

    size_t total_num_samples = static_cast<size_t>(file_size / (2 * sizeof(int16_t)));
    THROW_ERROR(num_samples > total_num_samples, "Requested number of samples exceeds total samples in file.");
    if (num_samples == 0) num_samples = total_num_samples;
    
    const double scale = 1.0 / static_cast<double>(1 << fix_point);

    std::vector<std::complex<int16_t>> raw(num_samples);
    
    std::streamsize bytes_to_read = num_samples * 2 * sizeof(int16_t);
    char * raw_ptr = reinterpret_cast<char*>(raw.data());
    THROW_ERROR(!file.read(raw_ptr, bytes_to_read), "Error reading complex binary data from file: " + filename);
    
    itpp::cvec out_vec(num_samples);
    for (size_t i = 0; i < num_samples; i++) out_vec(i) = std::complex<double>(raw[i].real() * scale, raw[i].imag() * scale);

    return out_vec;
}

itpp::ivec read_int8_as_ivec(const std::string& filename, size_t num_samples) {
    // 1. Open the file in binary mode, starting at the end to get the size
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return itpp::ivec(); // Return an empty vector on failure
    }
    
    // 2. Determine the number of bytes (which equals the number of int8 elements)
    size_t total_num_samples = file.tellg();
    file.seekg(0, std::ios::beg); // Rewind back to the beginning

    THROW_ERROR(num_samples > total_num_samples, "Requested number of samples exceeds total samples in file.");
    if (num_samples == 0) num_samples = total_num_samples;

    // 3. Read the raw 8-bit data into a standard C++ vector
    std::vector<int8_t> buffer(num_samples);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), num_samples)) {
        std::cerr << "Error: Failed to read data from " << filename << std::endl;
        return itpp::ivec();
    }
    
    // 4. Create the IT++ ivec and safely cast/copy the data over
    itpp::ivec result(num_samples);
    for (size_t i = 0; i < num_samples; ++i) {
        // The static_cast ensures the sign bit is preserved when expanding to 32-bit
        result(i) = static_cast<int>(buffer[i]);
    }
    
    return result;
}

void save_cvec_as_complex128(const itpp::cvec& vec, const std::string& filename) {
    std::ofstream outfile(filename, std::ios::binary | std::ios::app);    
    THROW_ERROR(!outfile.is_open(), "Could not open file " + filename + " for writing/appending.");

    int n = vec.length();
    for (int i = 0; i < n; ++i) {
        double r_double = std::real(vec(i));
        double i_double = std::imag(vec(i));
        outfile.write(reinterpret_cast<const char*>(&r_double), sizeof(double));
        outfile.write(reinterpret_cast<const char*>(&i_double), sizeof(double));
    }

    outfile.close();
}