#include "PCF8574.hpp"
#include <array>

namespace device
{
	PCF8574::PCF8574()
		: Output({.value = 0}),  _currentInput(0), _oldOutput(0)
	{
	}

	void PCF8574::Init()
	{
		_refreshTimePoint = std::chrono::system_clock::now() + REFRESH_TIME;
	}

	void PCF8574::TransferData()
	{
		// Input
		util::byte rxData[1];
		this->read(0x00, std::size(rxData), rxData);
		_currentInput = rxData[0];

		// Output
		if(Output.value != _oldOutput)
		{
			this->write(std::array{Output.value});
			_oldOutput = Output.value;
		}

		// Block until refresh was reached
		_refreshTimePoint = std::chrono::system_clock::now() + REFRESH_TIME;
	}

	void PCF8574::PollTransferData()
	{
		if(std::chrono::system_clock::now() > _refreshTimePoint)
		{
			TransferData();
		}
	}

	util::byte PCF8574::Input() const
	{
		return _currentInput;
	}
}
