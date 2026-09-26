// This file is part of the cpp-platformdirs Project.
// License: GPL-3.0-or-later. Contact: cpp-platformdirs-project@trinity2k.net
//

// A shared Dirs resolved from many threads at once; the rules are pure and the
// sources only read, so the TSan preset is where this test earns its keep.

#include <platformdirs/platformdirs.h>
#include <platformdirs/testing.h>

#include <gtest/gtest.h>

#include <atomic>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace platformdirs;

namespace {

// Joins on scope exit whatever was started, also when starting a later thread
// throws; std::jthread would do this but libc++ 19 still makes it opt-in.
class JoinedThreads {
public:
    JoinedThreads() = default;
    JoinedThreads(const JoinedThreads&) = delete;
    JoinedThreads& operator=(const JoinedThreads&) = delete;
    ~JoinedThreads()
    {
        for (auto& t : threads_) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    template <typename Fn>
    void start(Fn&& fn)
    {
        threads_.emplace_back(std::forward<Fn>(fn));
    }

private:
    std::vector<std::thread> threads_;
};

std::vector<std::string> snapshot(const Dirs& d)
{
    std::vector<std::string> out;
    const auto push = [&out](const Result<Path>& r) { out.push_back(r ? r->string() : "<error>"); };
    push(d.user_data_dir());
    push(d.user_config_dir());
    push(d.user_cache_dir());
    push(d.user_state_dir());
    push(d.user_log_dir());
    push(d.user_runtime_dir());
    push(d.user_documents_dir());
    push(d.user_fonts_dir());
    push(d.site_data_dir());
    push(d.site_config_dir());
    push(d.site_cache_dir());
    if (const auto fonts = d.iter_fonts_dirs()) {
        for (const Path& p : *fonts) {
            out.push_back(p.string());
        }
    }
    return out;
}

} // namespace

TEST(Threads, shared_dirs_answers_the_same_from_every_thread)
{
    FixedEnvironment env;
    FixedSystem sys;
    sys.set_home("/home/test");
    sys.set_user_id(1000);
    env.set("XDG_RUNTIME_DIR", "/rt");
    Options o;
    o.app_name = "myapp";
    o.version = "1.0";
    o.platform = Platform::Linux;
    const auto d = Dirs::create(o, env, sys);
    ASSERT_TRUE(d.has_value());

    const std::vector<std::string> expected = snapshot(*d);
    std::atomic<int> mismatches = 0;
    {
        JoinedThreads workers;
        for (int t = 0; t < 32; ++t) {
            workers.start([&] {
                for (int i = 0; i < 1000; ++i) {
                    if (snapshot(*d) != expected) {
                        mismatches.fetch_add(1);
                    }
                }
            });
        }
    }
    EXPECT_EQ(mismatches.load(), 0);
}

TEST(Threads, native_sources_can_be_read_concurrently)
{
    Options o;
    o.app_name = "myapp";
    const auto d = Dirs::create(o);
    ASSERT_TRUE(d.has_value());
    const std::vector<std::string> expected = snapshot(*d);
    std::atomic<int> mismatches = 0;
    {
        JoinedThreads workers;
        for (int t = 0; t < 16; ++t) {
            workers.start([&] {
                for (int i = 0; i < 200; ++i) {
                    if (snapshot(*d) != expected) {
                        mismatches.fetch_add(1);
                    }
                }
            });
        }
    }
    EXPECT_EQ(mismatches.load(), 0);
}
