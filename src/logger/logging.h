#pragma once

#include <string>
#include <fstream>
#include <cstdio>
#include <atomic>
#include <thread>

#include <boost/log/trivial.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include "macros.h"
#include "lf_queue.h"
#include "thread_utils.h"
#include "time_utils.h"
#include "OptMemPool.h"

namespace Common {
  constexpr size_t LOG_QUEUE_SIZE = 8 * 1024 * 1024;
  constexpr size_t LOG_ELEMENT_POOL_SIZE = 16 * 1024 * 1024;

  enum class LogType : int8_t {
    CHAR = 0,
    INTEGER = 1,
    LONG_INTEGER = 2,
    LONG_LONG_INTEGER = 3,
    UNSIGNED_INTEGER = 4,
    UNSIGNED_LONG_INTEGER = 5,
    UNSIGNED_LONG_LONG_INTEGER = 6,
    FLOAT = 7,
    DOUBLE = 8
  };

  struct LogElement {
    LogType type_ = LogType::CHAR;
    union {
      char c;
      int i;
      long l;
      long long ll;
      unsigned u;
      unsigned long ul;
      unsigned long long ull;
      float f;
      double d;
    } u_;
  };

  class Logger final {
  public:
    explicit Logger(const std::string &file_name)
        : file_name_(file_name), queue_(LOG_QUEUE_SIZE), 
          log_element_pool_(LOG_ELEMENT_POOL_SIZE) {
      file_.open(file_name);
      ASSERT(file_.is_open(), "Could not open log file:" + file_name);
      logger_thread_ = createAndStartThread(-1, "Common/Logger " + file_name_, [this]() { flushQueue(); });
      ASSERT(logger_thread_ != nullptr, "Failed to start Logger thread.");
    }

    ~Logger() {
      std::string time_str;
      BOOST_LOG_TRIVIAL(info) << Common::getCurrentTimeStr(&time_str) << " Flushing and closing Logger for " << file_name_;

      while (queue_.size()) {
        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);
      }
      running_ = false;
      logger_thread_->join();

      file_.close();
      BOOST_LOG_TRIVIAL(info) << Common::getCurrentTimeStr(&time_str) << " Logger for " << file_name_ << " exiting.";
    }

    void flushQueue() noexcept {
        while (running_) {
            for (auto next = queue_.getNextToRead(); queue_.size() && next; next = queue_.getNextToRead()) {
                switch ((*next)->type_) {
                    case LogType::CHAR:
                        file_ << (*next)->u_.c;
                        break;
                    case LogType::INTEGER:
                        file_ << (*next)->u_.i;
                        break;
                    case LogType::LONG_INTEGER:
                        file_ << (*next)->u_.l;
                        break;
                    case LogType::LONG_LONG_INTEGER:
                        file_ << (*next)->u_.ll;
                        break;
                    case LogType::UNSIGNED_INTEGER:
                        file_ << (*next)->u_.u;
                        break;
                    case LogType::UNSIGNED_LONG_INTEGER:
                        file_ << (*next)->u_.ul;
                        break;
                    case LogType::UNSIGNED_LONG_LONG_INTEGER:
                        file_ << (*next)->u_.ull;
                        break;
                    case LogType::FLOAT:
                        file_ << (*next)->u_.f;
                        break;
                    case LogType::DOUBLE:
                        file_ << (*next)->u_.d;
                        break;
                }
                log_element_pool_.deallocate(*next);
                queue_.updateReadIndex();
            }
            file_.flush();

            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(10ms);
        }
    }

    auto pushValue(const LogElement &log_element) noexcept {
      auto* new_element = log_element_pool_.allocate();
      *new_element = log_element;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const char value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::CHAR;
      new_element->u_.c = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const int value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::INTEGER;
      new_element->u_.i = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const long value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::LONG_INTEGER;
      new_element->u_.l = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const long long value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::LONG_LONG_INTEGER;
      new_element->u_.ll = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const unsigned value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::UNSIGNED_INTEGER;
      new_element->u_.u = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const unsigned long value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::UNSIGNED_LONG_INTEGER;
      new_element->u_.ul = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const unsigned long long value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::UNSIGNED_LONG_LONG_INTEGER;
      new_element->u_.ull = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const float value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::FLOAT;
      new_element->u_.f = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const double value) noexcept {
      auto* new_element = log_element_pool_.allocate();
      new_element->type_ = LogType::DOUBLE;
      new_element->u_.d = value;
      *(queue_.getNextToWriteTo()) = new_element;
      queue_.updateWriteIndex();
    }

    auto pushValue(const char *value) noexcept {
      while (*value) {
        pushValue(*value);
        ++value;
      }
    }

    auto pushValue(const std::string &value) noexcept {
      pushValue(value.c_str());
    }

    template<typename T, typename... A>
    auto log(const char *s, const T &value, A... args) noexcept {
      while (*s) {
        if (*s == '%') {
          if (UNLIKELY(*(s + 1) == '%')) {
            ++s;
          } else {
            pushValue(value);
            log(s + 1, args...);
            return;
          }
        }
        pushValue(*s++);
      }
      FATAL("extra arguments provided to log()");
    }

    auto log(const char *s) noexcept {
      while (*s) {
        if (*s == '%') {
          if (UNLIKELY(*(s + 1) == '%')) {
            ++s;
          } else {
            FATAL("missing arguments to log()");
          }
        }
        pushValue(*s++);
      }
    }

    auto logLatency(const std::string& message_type, long long network_latency_us, 
                    long long processing_time_us, long long total_latency_us) noexcept {
        log(LATENCY_FORMAT, message_type, network_latency_us, processing_time_us, total_latency_us);
    }

    Logger() = delete;
    Logger(const Logger &) = delete;
    Logger(const Logger &&) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger &operator=(const Logger &&) = delete;

  private:
    const std::string file_name_;
    std::ofstream file_;

    LFQueue<LogElement*> queue_;
    OptCommon::OptMemPool<LogElement> log_element_pool_;
    std::atomic<bool> running_ = {true};
    std::thread *logger_thread_ = nullptr;
    static constexpr const char* LATENCY_FORMAT = 
    "Message Type: % Network Latency: % ns, Processing Time: % ns, Total Latency: % ns\n";
  };
}