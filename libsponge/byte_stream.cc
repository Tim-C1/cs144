#include "byte_stream.hh"
#include <iostream>

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity) :
    _capacity (capacity) {}

size_t ByteStream::write(const string &data) {
    DUMMY_CODE(data);
    size_t n = (_capacity - bs.size() >= data.size()) ? data.size() : remaining_capacity();
    for (size_t i = 0; i < n; ++i) {
        bs.push_back(data[i]);
    }
    nw += n;
    return n;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    DUMMY_CODE(len);
    size_t n = bs.size() >= len ? len : remaining_capacity();
    return string().assign(bs.begin(), bs.begin() + n);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    DUMMY_CODE(len);
    size_t n = bs.size() >= len ? len : remaining_capacity();
    nr += n;
    while (n--) {
        bs.pop_front();
    }
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    DUMMY_CODE(len);
    string str = peek_output(len);
    pop_output(len);
    return str;
}

void ByteStream::end_input() {
    _end = true;
}

bool ByteStream::input_ended() const {
    return _end;
}

size_t ByteStream::buffer_size() const {
    return bs.size();
}

bool ByteStream::buffer_empty() const {
    return bs.empty();
}

bool ByteStream::eof() const {
    return input_ended() && buffer_empty();
}

size_t ByteStream::bytes_written() const {
    return nw;
}

size_t ByteStream::bytes_read() const {
    return nr;
}

size_t ByteStream::remaining_capacity() const {
    return _capacity - bs.size();
}
