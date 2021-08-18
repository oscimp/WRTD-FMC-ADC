#ifndef SI570_RFREQ_H
#define SI570_RFREQ_H

#include <stdint.h>

double rfreq_to_float(uint64_t rfreq)
{
	// Searching for the first bit that is 1
	int8_t i = 37;
	while ((rfreq & (((uint64_t) 1) << i)) == 0 && i >= 0)
		i--;

	// Mantissa
	rfreq &= ~(((uint64_t) 1) << i);
	rfreq <<= 51 - (i - 1);

	// Exponant
	rfreq |= ((uint64_t) (1023 + 10 - (38 - i))) << 52;

	return *((double *) &rfreq);
}

uint64_t float_to_rfreq(double input)
{
	uint64_t rfreq = *((uint64_t *) &input);
	uint16_t exponant = ((rfreq >> 52) & 0b11111111111);
	// Index of the first bit that is a one in rfreq
	int8_t i = exponant - 1023 - 10 + 38;

	// Extracting the mantissa
	rfreq &= 0x000FFFFFFFFFFFFF;
	// Shifting to the right based on i
	rfreq >>= 51 - (i - 1);

	// Adding the implicit 1
	rfreq |= ((uint64_t) 1) << i;

	return rfreq;
}

#endif /* SI570_RFREQ_H */
