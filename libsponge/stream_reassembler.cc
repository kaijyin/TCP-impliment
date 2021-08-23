#include "stream_reassembler.hh"
#include<iostream>

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`


using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _outputStream(capacity), _capacity(capacity) {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::mergeTo(const node &a,node &b){
    if(a.start_idx<b.start_idx){
      b.data.insert(b.data.begin(),a.data.begin(),a.data.begin()+b.start_idx-a.start_idx);
      b.start_idx=a.start_idx;
    }
    if(a.end_idx>b.end_idx){
      b.data.insert(b.data.end(),a.data.end()-(a.end_idx-b.end_idx),a.data.end());
      b.end_idx=a.end_idx;
    }
}
//先重组,再提交到缓冲区
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    
    size_t max_idx=_cur_idx-_outputStream.buffer_size()+_capacity-1;
    if(eof){_input_end_idx=index+data.size();}
    node nd(index,data);

    //边界情况
    //如果已进入缓冲区的seg包含了该seg,或者该seg的起始idx都大于窗口最大值,或者seg为空直接丢弃
    if(_cur_idx>nd.end_idx||nd.start_idx>max_idx||data.empty()){
        //可能是eof延迟标记   
        if(_cur_idx>=_input_end_idx)_outputStream.end_input();
         return ;
    }
    //去除左右越界数据
    if(nd.start_idx<_cur_idx){
        nd.data=data.substr(_cur_idx-nd.start_idx);
        nd.start_idx=_cur_idx;
    }
    if(nd.end_idx>max_idx){
        nd.data=nd.data.substr(0,nd.data.size()-(nd.end_idx-max_idx));
        nd.end_idx=max_idx;
    }
    //处理完边界情况,当前seg的data都在nextIdx和maxIdx之间,把可以重合的seg重组
    if(!_seg_buffer.empty()){
       set<node>::iterator it=_seg_buffer.upper_bound(nd);
       if(it!=_seg_buffer.begin()){
           it--;
       }
       while(it!=_seg_buffer.end()){
        //如果有交集,则删除迭代器,重合部分添加在nd中,别问我为什么判断要写这么丑,直接放里面不行嘛,嗯,你可以试试
        int a=it->end_idx-(nd.start_idx-1),b=(nd.end_idx+1)-it->start_idx;
           if(a>=0&&b>=0){
              mergeTo(*it,nd);
              _unassembled_bytes-=it->data.size();
              it=_seg_buffer.erase(it);
           }
           //没有交集,但是seg已经在右侧,再往右找也找不到能产生交集的seg,退出
           else if(it->start_idx>nd.end_idx){
               break;
           }else{
               it++;
           }
       }
    }
    //提交到缓冲区,找seg最前的部分,判断是否能读入字节流
    if(nd.start_idx==_cur_idx){
        _outputStream.write(nd.data);
        _cur_idx=nd.end_idx+1;
        if(_cur_idx>=_input_end_idx)_outputStream.end_input();
    }else{
    _seg_buffer.insert(nd);
    _unassembled_bytes+=nd.data.size();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes==0; }
