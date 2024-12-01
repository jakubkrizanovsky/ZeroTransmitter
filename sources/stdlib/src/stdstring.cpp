#include <stdstring.h>

namespace
{
    const char CharConvArr[] = "0123456789ABCDEF";
}

char* itoa(unsigned int input, char* output, unsigned int base)
{
	int i = 0;

	while (input > 0)
	{
		output[i] = CharConvArr[input % base];
		input /= base;
		i++;
	}

    if (i == 0)
    {
        output[i] = CharConvArr[0];
        i++;
    }

	output[i] = '\0';
	i--;

	for (int j = 0; j <= i/2; j++)
	{
		char c = output[i - j];
		output[i - j] = output[j];
		output[j] = c;
	}

	return output;
}

int atoi(const char* input)
{
	int output = 0;

	while (*input != '\0')
	{
		output *= 10;
		if (*input > '9' || *input < '0')
			break;

		output += *input - '0';

		input++;
	}

	return output;
}

char* ftoa(float input, char* output, unsigned int decimal_places)
{
	char* ptr = output;

    // Handle negative numbers
    if (input < 0) {
        *ptr++ = '-';
        input = -input;
    }

    // Extract the integer part
    int int_part = (int)input;
    float fraction = input - int_part;

    // Convert the integer part to a string
    char int_str[12]; // Temporary buffer for the integer part
    char* int_ptr = int_str;
    if (int_part == 0) {
        *int_ptr++ = '0';
    } else {
        while (int_part > 0) {
            *int_ptr++ = '0' + (int_part % 10);
            int_part /= 10;
        }
    }

    // Reverse the integer part string and write to buffer
    while (int_ptr != int_str) {
        *ptr++ = *(--int_ptr);
    }

    // Add the decimal point
    if (decimal_places > 0) {
        *ptr++ = '.';
    }

    // Convert the fractional part to the specified number of decimal places
    for (int i = 0; i < decimal_places; i++) {
        fraction *= 10;
        int digit = (int)fraction;
        *ptr++ = '0' + digit;
        fraction -= digit;
    }

    // Null-terminate the string
    *ptr = '\0';

    // Remove trailing zeros if any decimal places were specified
    if (decimal_places > 0) {
        char* end = ptr - 1;
        while (end > output && *end == '0') {
            *end-- = '\0';
        }
        if (end > output && *end == '.') {
            *end = '\0'; // Remove the decimal point if no fractional part remains
        }
    }

    return output;
}

char* strncpy(char* dest, const char *src, int num)
{
	int i;

	for (i = 0; i < num && src[i] != '\0'; i++)
		dest[i] = src[i];
	for (; i < num; i++)
		dest[i] = '\0';

   return dest;
}

int strncmp(const char *s1, const char *s2, int num)
{
	unsigned char u1, u2;
  	while (num-- > 0)
    {
      	u1 = (unsigned char) *s1++;
     	u2 = (unsigned char) *s2++;
      	if (u1 != u2)
        	return u1 - u2;
      	if (u1 == '\0')
        	return 0;
    }

  	return 0;
}

int strlen(const char* s)
{
	int i = 0;

	while (s[i] != '\0')
		i++;

	return i;
}

void bzero(void* memory, int length)
{
	char* mem = reinterpret_cast<char*>(memory);

	for (int i = 0; i < length; i++)
		mem[i] = 0;
}

void memcpy(const void* src, void* dst, int num)
{
	const char* memsrc = reinterpret_cast<const char*>(src);
	char* memdst = reinterpret_cast<char*>(dst);

	for (int i = 0; i < num; i++)
		memdst[i] = memsrc[i];
}
