#ifndef OKS_CSTRING_CMP_H
#define OKS_CSTRING_CMP_H

#include <stdint.h>

namespace oks {

  inline bool cmp_str1(const char * s1, const char s2[2]) {
    return (*(const int16_t *)s1 == *(const int16_t *)s2);
  }

  inline bool cmp_str2(const char * s1, const char s2[3]) {
    return cmp_str1(s1,s2) && (s1[2] == 0);
  }

  inline bool cmp_str2n(const char * s1, const char s2[2]) {
    return cmp_str1(s1,s2);
  }

  inline bool cmp_str3(const char * s1, const char s2[4]) {
    return (*(const int32_t *)s1 == *(const int32_t *)s2);
  }

  inline bool cmp_str3n(const char * s1, const char s2[3]) {
    return (s1[0] == s2[0]) && cmp_str1(s1+1,s2+1);
  }

  inline bool cmp_str4(const char * s1, const char s2[5]) {
    return (s1[0] == s2[0]) && cmp_str3(s1+1,s2+1);
  }

  inline bool cmp_str4n(const char * s1, const char s2[4]) {
    return cmp_str3(s1,s2);
  }

  inline bool cmp_str5(const char * s1, const char s2[6]) {
    return cmp_str1(s1,s2) && cmp_str3(s1+2,s2+2);
  }

  inline bool cmp_str5n(const char * s1, const char s2[5]) {
    return cmp_str3(s1,s2) && (s1[4] == s2[4]);
  }

  inline bool cmp_str6(const char * s1, const char s2[7]) {
    return cmp_str3(s1,s2) && cmp_str2(s1+4,s2+4);
  }

  inline bool cmp_str6n(const char * s1, const char s2[6]) {
    return cmp_str3(s1,s2) && cmp_str1(s1+4,s2+4);
  }

  inline bool cmp_str7(const char * s1, const char s2[8]) {
    return (*(const int64_t *)s1 == *(const int64_t *)s2);
  }

  inline bool cmp_str7n(const char * s1, const char s2[8]) {
    return (cmp_str3(s1,s2) && cmp_str3n(s1+4,s2+4));
  }

  inline bool cmp_str8(const char * s1, const char s2[9]) {
    return cmp_str7(s1,s2) && (s1[8] == 0);
  }

  inline bool cmp_str8n(const char * s1, const char s2[9]) {
    return (*(const int64_t *)s1 == *(const int64_t *)s2);
  }

  inline bool cmp_str9(const char * s1, const char s2[10]) {
    return cmp_str1(s1,s2) && cmp_str7(s1+2,s2+2);
  }

  inline bool cmp_str10(const char * s1, const char s2[11]) {
    return cmp_str7(s1,s2) && cmp_str2(s1+8,s2+8);
  }

  inline bool cmp_str11(const char * s1, const char s2[12]) {
    return cmp_str7(s1,s2) && cmp_str3(s1+8,s2+8);
  }

  inline bool cmp_str12(const char * s1, const char s2[13]) {
    return cmp_str7(s1,s2) && cmp_str4(s1+8,s2+8);
  }

  inline bool cmp_str13(const char * s1, const char s2[14]) {
    return cmp_str7(s1,s2) && cmp_str5(s1+8,s2+8);
  }

  inline bool cmp_str14(const char * s1, const char s2[14]) {
    return cmp_str7(s1,s2) && cmp_str6(s1+8,s2+8);
  }

  inline bool cmp_str15(const char * s1, const char s2[16]) {
    return cmp_str7(s1,s2) && cmp_str7(s1+8,s2+8);
  }

  inline bool cmp_str16(const char * s1, const char s2[17]) {
    return cmp_str7(s1,s2) && cmp_str7(s1+8,s2+8) && (s1[16] == 0);
  }

}

#endif
