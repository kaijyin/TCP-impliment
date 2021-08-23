#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <string>
#include <set>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  private:
    // Your code here -- add private members as necessary.
    struct node{
      size_t start_idx=0;
      size_t end_idx=0;
      std::string data="";
      node(size_t index,const std::string &DATA){
        this->start_idx=index;
        this->data=DATA;
        this->end_idx=index+DATA.size()-1;
      }
      bool operator<(const node &b)const{
        if(start_idx!=b.start_idx)return start_idx<b.start_idx;
        else return end_idx<b.end_idx;
      }
    };
    ByteStream _outputStream;  //!< The reassembled in-order byte stream
    size_t _capacity=0;    //!< The maximum number of bytes
    std::set<node>_seg_buffer={};
    size_t _input_end_idx=999999999;
    size_t _cur_idx=0;
    size_t _unassembled_bytes=0;
  private:
    void mergeTo(const node&a,node &b);
  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _outputStream; }
    ByteStream &stream_out() { return _outputStream; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
