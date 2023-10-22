#pragma once

#define WRITE_ONLY // The Function should only write to this

namespace mem
{
	struct ring_buffer_t;
}

namespace sys
{
	/**
	 * \brief Task for initializing and controlling sensors
	 * \param args Double Ptr to structure with the first being the pointer to the ring buffers and the second being the number of ring buffers.
	 */
	void sensor_control_task(WRITE_ONLY void* args);
}