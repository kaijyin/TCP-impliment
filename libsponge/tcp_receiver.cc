#include "tcp_receiver.hh"
#include<iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.


const uint64_t max_int=4294967296;
using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {

        // cout<<"datasize:"<<data.size()<<" sqno"<<seg.header().seqno<<"fin:"<<seg.header().fin<<" syn:"<<seg.header().syn<<endl;

    if(seg.header().syn){
        recieve_isn=true;
        isn=wrap(1ull,seg.header().seqno);
    }
    cout<<recieve_isn<<endl;
    if(!recieve_isn){
        return ;
    }
  const uint64_t check_point=static_cast<uint64_t>(_reassembler.stream_out().bytes_written());
  const uint64_t win_left=check_point;
  //syn和fin算不算做capcity里面
  const uint64_t win_right=static_cast<uint64_t>(_capacity-_reassembler.stream_out().buffer_size())+win_left-1;
  const uint64_t seg_start=seg.header().syn?0:unwrap(seg.header().seqno,isn,check_point);
    if(seg_start>win_right){
        return ;
    }
    string data=string(seg.payload().str().data(),seg.payload().size());
    // string data=string(seg.payload().str().data());
    if(seg.header().fin){
        fin_off_set=seg_start+data.size();
    }
    cout<<"datasize:"<<data.size()<<" startindex:"<<seg_start<<endl;
    _reassembler.push_substring(data,seg_start,seg.header().fin);

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
