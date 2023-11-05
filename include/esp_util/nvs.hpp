#pragma once
#include <cstdint>
#include <cstring>
#include <esp_err.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <optional>
#include <string>
#include <vector>

namespace esp
{
	namespace detail
	{
		struct NVSHandle
		{
			nvs_handle_t handle;
			esp_err_t    err;
			NVSHandle(std::string const& storage_name)
			{
				err = nvs_open(storage_name.c_str(), NVS_READWRITE, &handle);
				if(err != ESP_OK)
				{
					ESP_LOGW("NVS", "nvs_open failed %s", esp_err_to_name(err));
				}
			}
			~NVSHandle()
			{
				if(err == ESP_OK)
				{
					nvs_close(handle);
				}
			}
		};
	}   // namespace detail

	template<typename T>
	std::optional<T> getNVSValue(const char* name)
	{
		const char* storage_name  = "storage";
		detail::NVSHandle handle_ = detail::NVSHandle(storage_name);
		auto        handle        = handle_.handle;
		auto        err           = handle_.err;
		if(err != ESP_OK)
		{
			ESP_LOGW("NVS", "nvs_open(%s) failed %s", storage_name, esp_err_to_name(err));
			return {};
		}

		T value{};

		if constexpr(std::is_same_v<T, std::uint64_t>)
		{
			err = nvs_get_u64(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::uint32_t>)
		{
			err = nvs_get_u32(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::uint16_t>)
		{
			err = nvs_get_u16(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::uint8_t>)
		{
			err = nvs_get_u8(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::int64_t>)
		{
			err = nvs_get_i64(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::int32_t>)
		{
			err = nvs_get_i32(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::int16_t>)
		{
			err = nvs_get_i16(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::int8_t>)
		{
			err = nvs_get_i8(handle, name, &value);
		} else if constexpr(std::is_same_v<T, std::string>)
		{
			std::size_t length = 0;
			err = nvs_get_str(handle, name, nullptr, &length);
			if(err != ESP_OK)
			{
				ESP_LOGW("NVS", "nvs_get(\"%s\") failed %s", name, esp_err_to_name(err));
				return {};
			}
			value.resize(length);
			err = nvs_get_str(handle, name, value.data(), &length);
			value.resize(value.size() - 1);
		} else if constexpr(std::is_same_v<T, std::vector<std::byte>>)
		{
			std::size_t length = 0;
			err = nvs_get_blob(handle, name, nullptr, &length);
			if(err != ESP_OK)
			{
				ESP_LOGW("NVS", "nvs_get(\"%s\") failed %s", name, esp_err_to_name(err));
				return {};
			}
			value.resize(length);
			err = nvs_get_blob(handle, name, value.data(), &length);
		} else
		{
			static_assert(std::is_trivial_v<T>);
			std::size_t length = sizeof(value);
			err = nvs_get_blob(handle, name, reinterpret_cast<char*>(&value), &length);
		}

		if(err != ESP_OK)
		{
			ESP_LOGW("NVS", "nvs_get(\"%s\") failed %s", name, esp_err_to_name(err));
			return {};
		}
		return value;
	};

	template<typename T>
	T getNVSValue(const char* name, T const& default_value)
	{
		auto ot = getNVSValue<T>(name);
		return ot ? *ot : default_value;
	}

	template<typename T>
	bool setNVSValue(const char* name, T const& value)
	{
		std::string storage_name  = "storage";
		detail::NVSHandle handle_ = storage_name;
		auto        handle        = handle_.handle;
		auto        err           = handle_.err;
		if(err != ESP_OK)
		{
			ESP_LOGW("NVS", "nvs_open(%s) failed %s", storage_name, esp_err_to_name(err));
			return false;
		}

		if constexpr(std::is_same_v<T, std::uint64_t>)
		{
			err = nvs_set_u64(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::uint32_t>)
		{
			err = nvs_set_u32(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::uint16_t>)
		{
			err = nvs_set_u16(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::uint8_t>)
		{
			err = nvs_set_u8(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::int64_t>)
		{
			err = nvs_set_i64(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::int32_t>)
		{
			err = nvs_set_i32(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::int16_t>)
		{
			err = nvs_set_i16(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::int8_t>)
		{
			err = nvs_set_i8(handle, name, value);
		} else if constexpr(std::is_same_v<T, std::string>)
		{
			err = nvs_set_str(handle, name, value.c_str());
		} else if constexpr(std::is_same_v<T, std::vector<std::byte>>)
		{
			err = nvs_set_blob(handle, name, value.data(), value.size());
		} else
		{
			static_assert(std::is_trivial_v<T>);
			err = nvs_set_blob(handle, name, reinterpret_cast<char const*>(&value), sizeof(value));
		}

		if(err != ESP_OK)
		{
			ESP_LOGW("NVS", "nvs_set(\"%s\") failed: %s", name, esp_err_to_name(err));
			return false;
		}
		nvs_commit(handle);
		return true;
	}

}   // namespace esp
