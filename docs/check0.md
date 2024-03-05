# Check0-ByteStream

实现一个在内存中的可靠字节流，可以在一端读，另一端写，也就是一个队列。同时要实现抽象出的读/写的子类。

底层的ByteStream需要维护一个队列，首先要考虑这个队列应该以什么为元素
1. char
2. std::string
Writer.push()的参数为std::string，如果把string分成char的话首先是一个O(n)的操作，而直接转发这个string就是O(1)，所以底层的队列应该是一个std::queue<std::string>

对于一些标志位，可以使用flags + 移位，但是当时不会这种

重点：Reader.peek()，这个函数返回一个std::string_view，这个当时不会所以导致没用，进而导致push()和pop()都是假装string实则char，这次用一下。

首先要在ByteStream里定义一个std::string_view bytestream_view {}，string_view持有一个指针char*和一个长度length，可以认为它是一个只读的字符串，用在这里的好处是在pop()和peek()时方便操作，不用自己维护ByteStream的起始下标，具体如下：
```cpp
void Reader::pop( uint64_t len ) {
    ...
    else {
        bytestream_view.remove_prefix(len);
        len = 0;
    }
    ...
}

string_view Reader::peek() const
{
  return bytestream_view;
}
```