#pragma once

#include "fish/fish.hpp"
//
#include "bootloader_commands.h"
#include "esp_ota_ops.h"
#include "esp_util/nvs.hpp"
#include "fish/socket.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <esp_system.h>
#include <optional>
#include <string>
#include <thread>
#include <vector>
namespace esp {
namespace detail {
    using namespace fish::bootloader;
    struct Updater {
        static constexpr std::size_t BlockSize = 1024;

        enum class State {
            waitForStart,
            waitForBlock,
            waitForFinalize,
        };

        State state{State::waitForStart};

        std::size_t appSize{};
        std::size_t nextBlock{};
        std::size_t numBlocks{};
        std::size_t lastBlockSize{};

        esp_partition_t const* next_ota;
        esp_ota_handle_t       ota_handle;

        ResponseSet handle(FlashStart::Request const& request) {
            if(state != State::waitForStart) {
                state = State::waitForStart;
                return FlashStart::Response{.failed = true};
            }

            if(appSize > next_ota->size) {
                return FlashStart::Response{.failed = true};
            }

            appSize                    = request.appSize;
            nextBlock                  = 0;
            bool const isFullLastBlock = appSize % BlockSize == 0;
            numBlocks                  = (appSize / BlockSize) + (isFullLastBlock ? 0 : 1);
            lastBlockSize              = isFullLastBlock ? BlockSize : (appSize % BlockSize);
            state                      = State::waitForBlock;

            return FlashStart::Response{.failed = false, .blockSize = BlockSize};
        }

        ResponseSet handle(WriteBlock::Request const& request) {
            if(state != State::waitForBlock || request.blockNumber != nextBlock) {
                state = State::waitForStart;
                return WriteBlock::Response{.failed = true};
            }
            bool const isLastBlock = nextBlock == numBlocks - 1;
            if(request.data.size() != (isLastBlock ? lastBlockSize : BlockSize)) {
                state = State::waitForStart;
                return WriteBlock::Response{.failed = true};
            }

            if(esp_ota_write(ota_handle, request.data.data(), request.data.size()) != ESP_OK) {
                state = State::waitForStart;
                return WriteBlock::Response{.failed = true};
            }

            ++nextBlock;
            if(isLastBlock) {
                state = State::waitForFinalize;
            }
            return WriteBlock::Response{.failed = false};
        }

        ResponseSet handle(Finalize::Request const& request) {
            if(state != State::waitForFinalize) {
                state = State::waitForStart;
                return Finalize::Response{.failed = true};
            }

            if(esp_ota_end(ota_handle) != ESP_OK) {
                state = State::waitForStart;
                return Finalize::Response{.failed = true};
            }
            if(esp_ota_set_boot_partition(next_ota) != ESP_OK) {
                state = State::waitForStart;
                return Finalize::Response{.failed = true};
            }
            return Finalize::Response{.failed = false};
        }

        void update(std::string const& server, std::uint16_t port) {
            next_ota = esp_ota_get_next_update_partition(nullptr);
            if(next_ota == nullptr) {
                throw std::runtime_error("get_next_update_partition failed");
            }
            auto size = next_ota->size;
            size      = size - (size % SPI_FLASH_SEC_SIZE);

            if(esp_ota_begin(next_ota, size, &ota_handle) != ESP_OK) {
                throw std::runtime_error("ota begin failed");
            }

            fish::TCPCSocket socket(server, port);

            auto                   lastCommand = std::chrono::steady_clock::now();
            std::vector<std::byte> buffer;
            std::vector<std::byte> send_buffer;
            while(socket.valid()) {
                auto request = fish::recv_typed<RequestSet>(socket, buffer);
                if(request) {
                    auto const response
                      = std::visit([this](auto const& req) { return handle(req); }, *request);
                    fish::send_typed(socket, send_buffer, response);
                    bool const failed
                      = std::visit([this](auto const& res) { return res.failed; }, response);

                    if(failed) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        throw std::runtime_error("update failed");
                    }
                    bool const isFinalize = std::holds_alternative<Finalize::Response>(response);
                    if(isFinalize) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        return;
                    }
                    lastCommand = std::chrono::steady_clock::now();
                } else {
                    if(std::chrono::steady_clock::now() > lastCommand + std::chrono::seconds{10}) {
                        throw std::runtime_error("update timeout");
                    }
                }
            }
        }
    };
};   // namespace detail

struct System {
    void restart() { esp_restart(); }

    bool can_restart() const { return true; }

    bool can_update() const { return true; }

    void update(std::string const& server, std::uint16_t port) {
        try {
            detail::Updater updater{};
            updater.update(server, port);
        } catch(std::exception const& e) {
            ESP_LOGW("SYSTEM", "catched \"%s\" while updateing", e.what());
        }
        ESP_LOGW("SYSTEM", "restart");
        esp_restart();
    }

    std::optional<std::vector<std::byte>> read(std::string const& name) {
        return getNVSValue<std::vector<std::byte>>(name);
    }

    bool write(std::string const& name, std::vector<std::byte> const& buffer) {
        return setNVSValue<std::vector<std::byte>>(name, buffer);
    }
};

}   // namespace esp
