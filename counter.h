#ifndef _COUNTER_H_
#define _COUNTER_H_

// system event counters, for

#include <vector>
#include <map>
#include <string>
#include <stdint.h>

#include "macros.h"
#include "core.h"
#include "util.h"
#include "spinlock.h"

namespace private_ {

  // these objects are *never* supposed to be destructed
  // (this is a purposeful memory leak)
  struct event_ctx : private util::noncopyable {

    static std::vector<event_ctx *> &event_counters();
    static spinlock &event_counters_lock();

    // tag to avoid making event_ctx virtual
    event_ctx(const std::string &name, bool avg_tag)
      : name(name), avg_tag(avg_tag)
    {}

    ~event_ctx()
    {
      ALWAYS_ASSERT(false);
    }

    const std::string name;
    const bool avg_tag;

    // per-thread counts
    volatile util::aligned_padded_u64 tl_counts[coreid::NMaxCores];
  };

  struct event_ctx_avg : public event_ctx {
    event_ctx_avg(const std::string &name) : event_ctx(name, true) {}
    volatile util::aligned_padded_u64 tl_invokes[coreid::NMaxCores];
  };
}

class event_counter : private util::noncopyable {
public:
  event_counter(const std::string &name);

  inline ALWAYS_INLINE void
  inc(uint64_t i = 1)
  {
#ifdef ENABLE_EVENT_COUNTERS
    const size_t id = coreid::core_id();
    ctx->tl_counts[id].elem += i;
#endif
  }

  inline ALWAYS_INLINE event_counter &
  operator++()
  {
    inc();
    return *this;
  }

  inline ALWAYS_INLINE event_counter &
  operator+=(uint64_t i)
  {
    inc(i);
    return *this;
  }

  // WARNING: an expensive operation!
  static std::map<std::string, double> get_all_counters();
  // WARNING: an expensive operation!
  static void reset_all_counters();

private:
#ifdef ENABLE_EVENT_COUNTERS
  private_::event_ctx *const ctx;
#endif
};

class event_avg_counter : private util::noncopyable {
public:
  event_avg_counter(const std::string &name);

  inline ALWAYS_INLINE void
  offer(uint64_t value)
  {
#ifdef ENABLE_EVENT_COUNTERS
    const size_t id = coreid::core_id();
    ctx->tl_counts[id].elem += value;
    ctx->tl_invokes[id].elem++;
#endif
  }

private:
#ifdef ENABLE_EVENT_COUNTERS
  private_::event_ctx_avg *const ctx;
#endif
};

#endif /* _COUNTER_H_ */
