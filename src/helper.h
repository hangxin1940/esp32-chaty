#ifndef HELPER_H
#define HELPER_H

inline String randomString(int len)
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const size_t maxIndex = (sizeof(charset) - 1);
    String str = "";
    for (int i = 0; i < len; ++i)
    {
        str += charset[rand() % maxIndex];
    }
    return str;
}

#endif //HELPER_H
