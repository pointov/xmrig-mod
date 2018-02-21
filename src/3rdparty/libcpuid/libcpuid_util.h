#ifndef __LIBCPUID_UTIL_H__
#define __LIBCPUID_UTIL_H__

#define COUNT_OF(array) (sizeof(array) / sizeof(array[0]))

struct feature_map_t {
	unsigned bit;
	cpu_feature_t feature;
};
 
void match_features(const struct feature_map_t* matchtable, int count,
                    uint32_t reg, struct cpu_id_t* data);

struct match_entry_t {
	int family, model, stepping, ext_family, ext_model;
	int ncores, l2cache, l3cache, brand_code;
	uint64_t model_bits;
	int model_code;
	char name[32];
};

// returns the match score:
int match_cpu_codename(const struct match_entry_t* matchtable, int count,
                       struct cpu_id_t* data, int brand_code, uint64_t bits,
                       int model_code);
/*
 * Seek for a pattern in `haystack'.
 * Pattern may be an fixed string, or contain the special metacharacters
 * '.' - match any single character
 * '#' - match any digit
 * '[<chars>] - match any of the given chars (regex-like ranges are not
 *              supported)
 * Return val: 0 if the pattern is not found. Nonzero if it is found (actually,
 *             x + 1 where x is the index where the match is found).
 */
int match_pattern(const char* haystack, const char* pattern);

/*
 * Gets an initialized cpu_id_t. It is cached, so that internal libcpuid
 * machinery doesn't need to issue cpu_identify more than once.
 */
struct cpu_id_t* get_cached_cpuid(void);


/* returns true if all bits of mask are present in `bits'. */
int match_all(uint64_t bits, uint64_t mask);

/*
 * Sets the current errno
 */
int set_error(cpu_error_t err);

#endif /* __LIBCPUID_UTIL_H__ */
