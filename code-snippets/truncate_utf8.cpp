string truncateUTF8(const string& src, size_t tolen)
{
    string dst;
    dst.reserve(tolen);
    size_t i = 0;
    while (dst.size() < tolen && i < src.size())
    {   
        size_t step = 0;
        if ((src[i] & 0x80) == 0x00)              // 1字节
            step = 1;
        else if ((src[i] & 0xE0) == 0xC0)          // 2字节
            step = 2;
        else if ((src[i] & 0xF0) == 0xE0)         // 3字节
            step = 3;
        else if ((src[i] & 0xF8) == 0xF0)         // 4字节
            step = 4;

        if (0 == step || i + step > src.size()) 
        {   
            LOG(error, "invaild utf-8 string: " << src);
            break;
        }   
        if (dst.size() + step > tolen) break;
        while (step-- > 0) dst.push_back(src[i++]);
    }                                                                                                                                                                                   
    return dst;
}
