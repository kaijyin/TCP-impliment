#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

typedef size_t st;
using namespace std;
ByteStream::ByteStream(const size_t capacity) {
    this->capcity=capacity;
 }

size_t ByteStream::write(const string &data) {
    st addsize=min(data.size(),capcity-size);
    for(st i=0;i<addsize;i++){
      buffer.push_back(data[i]);
    }
    size+=addsize;
    totoalWrite+=addsize;
    return addsize;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    st outsize=min(size,len);
    return buffer.substr(0,outsize);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    st outsize=min(size,len);
    buffer.erase(0,outsize);
    size-=outsize;
    totoalRead+=outsize;
 }

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    st outsize=min(size,len);
    string res=buffer.substr(0,outsize);
    buffer.erase(0,outsize);
    size-=outsize;
    totoalRead+=outsize;
    return res;
}
//标志读入结束
void ByteStream::end_input() {
    inputEnd=true;
}
//是否停止读入
bool ByteStream::input_ended() const { 
    return inputEnd;
 }
//缓冲区容量
size_t ByteStream::buffer_size() const { 
    return size;
 }
//当前缓冲区是否为空
bool ByteStream::buffer_empty() const {
    return size==0;
}
//停止输入且缓冲区中没有数据,读取结束
bool ByteStream::eof() const { 
    return inputEnd&&size==0;
}
//已写字节数
size_t ByteStream::bytes_written() const { 
      return totoalWrite;
 }
//已读字节数
size_t ByteStream::bytes_read() const { 
    return totoalRead;
 }
//剩余容量
size_t ByteStream::remaining_capacity() const { 
    return capcity-size;
}
