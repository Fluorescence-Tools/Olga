#ifndef PCG_H
#define PCG_H
#include <cstdint>
#include <random>

class PCG32Generator
{
	uint64_t state;
	uint64_t inc;
public:
	PCG32Generator()
	{
		std::random_device r;
		state=r();
		inc=r();
		inc=inc%2?inc:inc-1;
	}
	uint32_t random()
	{
	    uint64_t oldstate = state;
	    // Advance internal state
	    rng->state = oldstate * 6364136223846793005ULL + (inc|1);
	    // Calculate output function (XSH RR), uses old state for max ILP
	    uint32_t xorshifted = ((oldstate >> 18u) ^ oldstate) >> 27u;
	    uint32_t rot = oldstate >> 59u;
	    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
	}
	uint32_t boundedrand(uint32_t bound)
	{
		uint32_t threshold = -bound % bound;
		    for (;;) {
			uint32_t r = random();
			if (r >= threshold)
			    return r % bound;
		    }
	}
};

class PCG64Generator {
	PCG32Generator gen1, gen2;
public:
	uint64_t random()
	{
		return ((uint64_t)(gen1.random()) << 32)
			   | gen2.random();
	}
	uint64_t boundedrand(uint64_t bound)
	{
		uint64_t threshold = -bound % bound;
		    for (;;) {
			uint64_t r = random();
			if (r >= threshold)
			    return r % bound;
		    }
	}
};


#endif // PCG_H
