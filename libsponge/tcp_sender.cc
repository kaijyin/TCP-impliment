#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>
#include <iostream>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) ,RTO(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { return _next_seqno-ack_seqno; }

void TCPSender::fill_window() {
    //全部数据都已近发送,无数据发送
    if(_next_seqno>=fin_seqno){
        return ;
    }
    if(_next_seqno==0){
      TCPSegment seg;
      seg.header().syn=true;
      seg.header().seqno=_isn;
      _segments_out.push(seg);
      _next_seqno+=1ull;
      seg_buffer.push_back({seg,_next_seqno});
      return ;
    }

    uint32_t max_size=1452;
    uint32_t fill_size=static_cast<uint32_t>(wd_right+1ull>_next_seqno?wd_right+1ull-_next_seqno:0);
    string total_data=_stream.read(fill_size);
    cout<<"wd_right:"<<wd_right<<" next_seq:"<<_next_seqno<<" fill_size:"<<fill_size<<endl;
    bool fin_flg=_stream.input_ended()&&_stream.buffer_empty()&&total_data.size()<fill_size;
    cout<<"datasize:"<<total_data.size()<<" fin"<<fin_flg<<endl;
    uint32_t i=0;
    while(i+max_size<total_data.size()){
        TCPSegment seg;
        seg.header().seqno=wrap(_next_seqno,_isn);
        seg.payload()=Buffer(total_data.substr(i,max_size));
        _segments_out.push(seg);
        _next_seqno+=static_cast<uint64_t>(max_size);
        seg_buffer.push_back({seg,_next_seqno});
        i+=max_size;
    }
    //fin也算一个字节
    //处理最后部分
    if(i<total_data.size()||fin_flg){
       TCPSegment last_seg;
       if(fin_flg){last_seg.header().fin=true;}
       last_seg.header().seqno=wrap(_next_seqno,_isn);
       if(i<total_data.size())last_seg.payload()=Buffer(total_data.substr(i));
       _segments_out.push(last_seg);
       if(_next_seqno<fin_seqno){
           _next_seqno+=static_cast<uint64_t>(last_seg.length_in_sequence_space());
       }
       seg_buffer.push_back({last_seg,_next_seqno});
       if(fin_flg)fin_seqno=_next_seqno;
    }
}
//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    uint64_t ack=unwrap(ackno,_isn,_next_seqno);
    cout<<"ack:"<<ack<<" nextseq:"<<_next_seqno<<endl;
    if(ack>_next_seqno){
        return ;
    }
    bool new_ack_flg=false;
    while(!seg_buffer.empty()){
        if(seg_buffer.front().ack_index>ack){
            break;
        }else{
            new_ack_flg=true;
            ack_seqno=seg_buffer.front().ack_index;
            //   cout<<"ack_seqno:"<<ack_seqno<<endl;
            seg_buffer.pop_front();
        }
    }
    if(new_ack_flg){
       RTO=_initial_retransmission_timeout;
       retrans_num=0;
       timer.flash();
    }
    uint64_t now_wd_right=ack+static_cast<uint64_t>(window_size)-1ull;
    cout<<"now_wd_right:"<<now_wd_right<<" wd_right:"<<wd_right<<endl;
    if(now_wd_right>wd_right){
        wd_right=now_wd_right;
    }
    if(window_size==0){
        wd_right++;
    }
    fill_window();
    if(window_size==0){
        wd_right--;
    }
 }

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    //=?
    timer.tick(ms_since_last_tick);
    if(!timer.time_out(RTO)){
       return ;
    }
    if(!seg_buffer.empty())_segments_out.push(seg_buffer.front().seg);
    uint64_t wind_size=wd_right+1ull-ack_seqno;
    if(wind_size!=0ull){
        retrans_num++;
        RTO=RTO*2u;
    }
    timer.flash();
 }

unsigned int TCPSender::consecutive_retransmissions() const { 
    return retrans_num;
 }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno=wrap(_next_seqno,_isn);
    _segments_out.push(seg);
}
