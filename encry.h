#pragma once

static inline char* XOR(char* buf, int len)
{
  for(int i=0; i < len; ++i)
  {
    buf[i]^=1;
  }
}

static inline void Decrypt(char* buf, int len)
{
  XOR(buf, len);
}

static inline void Encry(char* buf, int len)
{
  XOR(buf, len);
}
