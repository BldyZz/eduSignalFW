#pragma once

/**
 * \brief In-code indication defines
 */
#define nobreak [[fallthrough]] // Indicates that a jump into the next case statement occurs. Generally for optimization purposes.

/**
 * \brief Function indication defines
 */
#define OUT						// Indicates an output parameter.
#define IN						// Indicates an input  parameter.
#define NODISCARD [[nodiscard]]	// The return should not be ignored.
#define DISCARD   (void)		// Indicates that the return value of a function gets ignoreed.
#define CHECKVAL				// Return value is not necessarily valid. A check of the returned value is advised.
#define WRITE_ONLY				// Indicates that the parameter should only be written to.
#define READ_ONLY				// Indicates that the parameter should only be read from.

/**
 * \brief Utility defines
 */
#define NAME(x)			 #x		   // Returns the name of the x parameter.
#define BYTES_TO_BITS(x) ((x) * 8) // Returns the given number of bytes in bits
#define PRINTI(tag, ...) std::printf(tag " " __VA_ARGS__)