#include <functional>
#include <sys/types.h>
#include <utility>

using SIGNAL_FUNC = std::function<void()>;
using CHANNEL_FUNC = std::function<void()>;
using Task = std::function<void()>;