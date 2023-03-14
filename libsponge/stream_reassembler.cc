#include "stream_reassembler.hh"
#include <iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _subMap(), _expect(0), _remain(capacity), _eof_index(-1), _output(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    DUMMY_CODE(data, index, eof);
    _remain = _capacity - getSubsLength() - _output.buffer_size();
    if (eof) {
        _eof_index = index + data.size();
    }

    if (data == "") {
        if (_expect == _eof_index) {
            _output.end_input();
        }
        return;
    }
    if (index <= _expect) {
        // overlap with assembled strings;
        if (index + data.size() - 1 >= _expect) {
            string data_cut = data.substr(_expect - index);
            // scan the map and combine soem subs to save space
            for (auto i = _subMap.begin(); i != _subMap.end();) {
                auto sub = *i;
                if (sub.first >= _expect) {
                    if (sub.first +  sub.second.size() <= _expect + data_cut.size()) {
                        _remain += sub.second.size();
                        i = _subMap.erase(i);
                    } else if (sub.first <= _expect + data_cut.size() && sub.first + sub.second.size() >= _expect + data_cut.size()) {
                        data_cut += sub.second.substr(_expect + data_cut.size() - sub.first);
                        _remain += sub.second.size();
                        i = _subMap.erase(i);
                    } else {
                        ++i;
                    }
                } else {
                    cerr << "should not reach here" << endl;
                }
            }
            if (data_cut.size() > _remain) {
                data_cut = data_cut.substr(0, _remain);
            }
            _output.write(data_cut);
            _expect += data_cut.size();
            _remain -= data_cut.size();
        } else {
            // ignore this substrings
        }
    } else {
        string data_add = data;
        for (auto i = _subMap.begin(); i != _subMap.end();) {
            auto sub = *i;
            if (sub.first <= index && (sub.first + sub.second.size() - 1 >= index + data.size() - 1)) {
                // the sub can include the whole data;
                if (sub.first + sub.second.size() >= index + data.size()) {
                    data_add = "";
                    break;
                } else {
                    string sub_cut = sub.second.substr(0, index - sub.first - 1);
                    data_add = data + sub_cut;
                    _remain += sub.second.size();
                    i = _subMap.erase(i);
                }
            } else if (sub.first >= index && (sub.first <= index + data.size() - 1)) {
                // the sub is totally included by the data;
                if (sub.first + sub.second.size() <= index + data.size()) {
                    _remain += sub.second.size();
                    i = _subMap.erase(i);
                } else {
                    string sub_cut = sub.second.substr(index + data.size() - sub.first);
                    data_add = data + sub_cut;
                    _remain += sub.second.size();
                    i = _subMap.erase(i);
                }
            } else {
                ++i;
            }
        }
        if (data_add.size() > 0) {
            _subMap[index] = data_add;
            _remain -= data_add.size();
        }
    }

    // iterate through the map and write continuous subs to output
    for (auto i = _subMap.begin(); i != _subMap.end();) {
        string s = (*i).second;
        if (i->first == _expect) {
            _output.write(s);
            _expect += s.size();
            i = _subMap.erase(i);
        } else {
            ++i;
        }
    }

    if (_expect == _eof_index) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const {
    return getSubsLength();
}

bool StreamReassembler::empty() const {
    return getSubsLength() == 0;
}

size_t StreamReassembler::getSubsLength() const {
    size_t sum = 0;
    for (auto sub: _subMap) {
        sum += sub.second.size();
    }
    return sum;
}
