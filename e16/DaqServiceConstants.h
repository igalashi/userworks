#ifndef DaqService_Plugins_PluginOptions_h
#define DaqService_Plugins_PluginOptions_h

#include <string_view>

namespace daq::service {

static constexpr std::string_view Separator{"separator"};
static constexpr std::string_view ServiceName{"service-name"};
static constexpr std::string_view ServiceRegistryUri{"registry-uri"};

static constexpr std::string_view RunInfoPrefix{"run_info"};
static constexpr std::string_view RunNumber{"run_number"};
static constexpr std::string_view StartTime{"start_time"};
static constexpr std::string_view StartTimeNS{"start_time_ns"};
static constexpr std::string_view StopTime{"stop_time"};
static constexpr std::string_view StopTimeNS{"stop_time_ns"};

static constexpr std::string_view Uuid{"uuid"};
static constexpr std::string_view MaxTtl{"max-ttl"};
static constexpr std::string_view TtlUpdateInterval{"ttl-update-interval"};
static constexpr std::string_view HostIpAddress{"host-ip"};
static constexpr std::string_view Hostname{"hostname"};
static constexpr std::string_view CommandChannel{"command-channel"};
} // namespace daq::service

#endif