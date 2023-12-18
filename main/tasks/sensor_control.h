#pragma once

#include "../util/defines.h"

namespace sys
{
	/**
	 * \brief Task for initializing and controlling sensors
	 * \param view View of ring buffers.
	 */
	void sensor_control_task(WRITE_ONLY void* view);
}