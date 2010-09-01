#include "escape.h"
#include <alloca.h>
#include <string.h>

#define REQUIRE_SPACE(bytes) { if (dest_index + (bytes) >= maxlength) { \
memcpy(data, src, length);\
return false;\
} }

bool escape_frame(uint8_t* data, unsigned length, unsigned maxlength)
{
	unsigned source_index, dest_index;
	uint8_t* src = alloca(length);
	uint8_t* dst = data;
	memcpy(src, data, length);
	for (source_index = 0, dest_index = 0; source_index < length; ++source_index) {
		if (src[source_index] == 0x7E) {
			REQUIRE_SPACE(2);
			dst[dest_index++] = 0x7D;
			dst[dest_index++] = 0x5E;
		} else if (src[source_index] == 0x7D) {
			REQUIRE_SPACE(2);
			dst[dest_index++] = 0x7D;
			dst[dest_index++] = 0x5D;
		} else {
			REQUIRE_SPACE(1);
			dst[dest_index++] = src[source_index];
		}
	}
	return true;
}

bool unescape_frame(uint8_t* data, unsigned length, unsigned* outlength)
{
	uint8_t* source = data;
	uint8_t* dest = data;
	uint8_t* end = data + length;
	uint8_t current;
	while (source != end) {
		current = *source++;
		if (current == 0x7D) {
			if (source == end) {
				return false;
			} else {
				current = *source++;
				switch (current) {
					case 0x5E:
						*dest++ = 0x7E;
						break;
					case 0x5D:
						*dest++ = 0x7D;
						break;
					default:
						return false;
				}
			}
		} else {
			*dest++ = current;
		}
	}
	*outlength = dest - data;
	return true;
}
