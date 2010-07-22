#include <stdio.h>
#include <inttypes.h>

#define POLYNOMIAL 0xA001

int main()
{
	int      n, k;
	uint16_t c;
	puts("/* AUTOMATICALLY GENERATED, DO NOT EDIT */");
	puts("#include \"crc16.h\"\n");
	printf("static const uint16_t crc_table[256] = {");
	for (n = 0; n < 256; ++n) {
		c = (uint16_t)n;
		for (k = 0; k < 8; ++k) {
			if (c & 1) {
				c = POLYNOMIAL ^ (c >> 1);
			} else {
				c = c >> 1;
			}
		}
		printf("%hu%s", c, n == 255 ? "" : ", ");
	}
	puts("};\n");
	puts("uint16_t crc16(const void* data, size_t length)\n{");
	puts("\tuint16_t c = (uint16_t)-1;");
	puts("\tconst uint8_t* buf = (const uint8_t*)data;");
	puts("\tsize_t n;");
	puts("\tfor (n = 0; n < length; ++n) {");
	puts("\t\tc = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);");
	puts("\t}");
	puts("\treturn ~c;");
	puts("}");
	return 0;
}

