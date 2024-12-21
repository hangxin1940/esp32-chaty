#ifndef HELPER_H
#define HELPER_H

inline String randomString(int len)
{
    const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    const size_t maxIndex = (sizeof(charset) - 1);
    String str = "";
    for (int i = 0; i < len; ++i)
    {
        str += charset[esp_random() % maxIndex];
    }
    return str;
}


inline char* x_ps_malloc(uint16_t len)
{
    char* ps_str = NULL;
    if (psramFound()) { ps_str = (char*)ps_malloc(len); }
    else { ps_str = (char*)malloc(len); }
    return ps_str;
}

inline char* urlencode(const char* str, bool spacesOnly)
{
    // Reserve memory for the result (3x the length of the input string, worst-case)
    char* encoded = x_ps_malloc(strlen(str) * 3 + 1);
    char* p_encoded = encoded;

    if (encoded == NULL)
    {
        return NULL; // Memory allocation failed
    }

    while (*str)
    {
        // Adopt alphanumeric characters and secure characters directly
        if (isalnum((unsigned char)*str))
        {
            *p_encoded++ = *str;
        }
        else if (spacesOnly && *str != 0x20)
        {
            *p_encoded++ = *str;
        }
        else
        {
            p_encoded += sprintf(p_encoded, "%%%02X", (unsigned char)*str);
        }
        str++;
    }
    *p_encoded = '\0'; // Null-terminieren
    return encoded;
}

#endif //HELPER_H
