#ifndef __INCLUDED_TAGGED_POINTER
#define __INCLUDED_TAGGED_POINTER

#ifdef __cplusplus
extern "C"
{
#endif

typedef union _tagged_pointer {
	unsigned long longVal;
	void* ptrVal;
} tp_t;

static inline tp_t tp_init(void* ptr, int flags)
{
	tp_t val;
	val.ptrVal = ptr;
	val.longVal |= (unsigned long)(flags & 0x3);
	return val;
}

static inline void* tp_ptr(tp_t tp)
{
	tp.longVal &= ~(0x3UL);
	return tp.ptrVal;
}

static inline int tp_flags(tp_t tp)
{
	return tp.longVal & 0x3;
}

static inline tp_t tp_setflags(tp_t tp, int flags)
{
	return tp_init(tp_ptr(tp), flags);
}

static inline tp_t tp_setptr(tp_t tp, void* ptr)
{
	return tp_init(ptr, tp_flags(tp));
}

#ifdef __cplusplus
}
#endif

#endif
