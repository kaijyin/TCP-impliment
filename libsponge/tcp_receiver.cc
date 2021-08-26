#include "tcp_receiver.hh"
#include<iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


const uint64_t max_int=4294967296;
using namespace std;

//seg中的syn,fin,和data可能同时存在,和实现reassembler一样,用一个终点标识符fin_off_set标记,如果字节流读入字节数到达该标识符,发送fin确认
void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn){
        recieve_isn=true;
        //把syn_seqno+1作为isn,
        isn=wrap(1ull,seg.header().seqno);
    }
    if(!recieve_isn){
        return ;
    }
  //把第一个没有重组的字节作为check_point
  const uint64_t check_point=static_cast<uint64_t>(_reassembler.stream_out().bytes_written());
  //找到window的左右边界
  const uint64_t win_left=check_point;
  //syn和fin算不算做capcity里面,不算,额外处理
  const uint64_t win_right=static_cast<uint64_t>(_capacity-_reassembler.stream_out().buffer_size())+win_left-1;
  //找到seg的真实起始下标,如果是用于连接的seg,起始下标为0
  const uint64_t seg_start=seg.header().syn?0:unwrap(seg.header().seqno,isn,check_point);
  //超出窗口就不要了
    if(seg_start>win_right||seg_start+seg.length_in_sequence_space()<=win_left){
        return ;
    }
    string data=string(seg.payload().str());
    if(seg.header().fin){
        fin_off_set=seg_start+data.size();
    }
    _reassembler.push_substring(data,seg_start,seg.header().fin);
    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(recieve_isn){
       if(fin_off_set==_reassembler.stream_out().bytes_written()){
         return wrap(1ull+static_cast<uint64_t>(_reassembler.stream_out().bytes_written()),isn);
       }else{
           return wrap(static_cast<uint64_t>(_reassembler.stream_out().bytes_written()),isn);
       }
    }
    return {};
 }

size_t TCPReceiver::window_size() const {   
   return _capacity-_reassembler.stream_out().buffer_size();
 }
