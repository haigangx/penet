#include "codec.h"

#include "net.h"

int LineCodec::tryDecode(Slice data, Slice &msg)
{
    if (data.size() == 1 && data[0] == 0x04)
    {
        msg = data;
        return 1;
    }
    for (size_t i = 0; i < data.size(); i++)
    {
        //只取一行数据
        if (data[i] == '\n')
        {
            if (i > 0 && data[i-1] == '\r')
            {
                msg = Slice(data.data(), i-1);
                return i + 1;
            }
            else
            {
                msg = Slice(data.data(), i);
                return i + 1;
            }
            
        }
    }
}

void LineCodec::encode(Slice msg, Buffer &buf)
{
    buf.append(msg).append("\r\n");
}

int LengthCodec::tryDecode(Slice data, Slice &msg)
{
    if (data.size() < 8)
        return 0;

    int len = Net::ntoh(*(int32_t *)(data.data() + 4));
    //长度太长了或者不满足格式要求
    if (len > 1024 * 1024 || memcmp(data.data(), "mBdT", 4) != 0)
        return -1;
    if ((int)data.size() >= len + 8)
    {
        msg = Slice(data.data() + 8, len);
        return len + 8;
    }
    return 0;
}

void LengthCodec::encode(Slice msg, Buffer &buf)
{
    buf.append("mBdT").appendValue(Net::hton((int32_t)msg.size())).append(msg);
}
