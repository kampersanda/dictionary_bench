#ifdef __APPLE__
#include <mach/mach.h>
#endif

#include <unistd.h>
#include <array>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>

#include <map>
#include <string>
#include <unordered_map>

// Google Sparse Hash
#include <sparsepp/spp.h>
#include <sparsehash/dense_hash_map>
#include <sparsehash/sparse_hash_map>

// Judy
#include <Judy.h>

// ART
#include <libart/art.h>

// Cedar
#include <cedar/cedar.h>
#include <cedar/cedarpp.h>

// Tessil
#include <tsl/array-hash/array_map.h>
#include <tsl/hopscotch_map.h>
#include <tsl/htrie_map.h>
#include <tsl/robin_map.h>

// Poplar-trie
#include <poplar-trie/poplar.hpp>

#include "cmdline.h"

size_t get_process_size() {
#ifdef __APPLE__
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    task_info(current_task(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&t_info), &t_info_count);
    return t_info.resident_size;
#else
    FILE* fp = std::fopen("/proc/self/statm", "r");
    size_t dummy(0), vm(0);
    std::fscanf(fp, "%ld %ld ", &dummy, &vm);  // get resident (see procfs)
    std::fclose(fp);
    return vm * ::getpagesize();
#endif
}

class timer {
  public:
    using hrc = std::chrono::high_resolution_clock;

    timer() = default;

    template <class Period = std::ratio<1>>
    double get() const {
        return std::chrono::duration<double, Period>(hrc::now() - tp_).count();
    }

  private:
    hrc::time_point tp_{hrc::now()};
};

template <size_t N>
inline double get_average(const std::array<double, N>& ary) {
    double sum = 0.0;
    for (auto v : ary) {
        sum += v;
    }
    return sum / N;
}
inline double get_average(const std::vector<double>& ary) {
    double sum = 0.0;
    for (auto v : ary) {
        sum += v;
    }
    return sum / ary.size();
}

template <size_t N>
inline double get_min(const std::array<double, N>& ary) {
    double min = std::numeric_limits<double>::max();
    for (auto v : ary) {
        if (v < min) {
            min = v;
        }
    }
    return min;
}
inline double get_min(const std::vector<double>& ary) {
    double min = std::numeric_limits<double>::max();
    for (auto v : ary) {
        if (v < min) {
            min = v;
        }
    }
    return min;
}

template <typename T>
inline std::string realname() {
    int status;
    return abi::__cxa_demangle(typeid(T).name(), nullptr, nullptr, &status);
}

/**
 *  STL and Google
 */
enum class standard_map_types { STD_MAP, STD_HASH, GOOGLE_DENCE_HASH, GOOGLE_SPARSE_HASH, SPP };

template <standard_map_types>
struct standard_map_trait;

template <>
struct standard_map_trait<standard_map_types::STD_MAP> {
    using type = std::map<std::string, int>;
    static std::string name() {
        return "std_map";
    }
};
template <>
struct standard_map_trait<standard_map_types::STD_HASH> {
    using type = std::unordered_map<std::string, int>;
    static std::string name() {
        return "std_unordered_map";
    }
};
template <>
struct standard_map_trait<standard_map_types::GOOGLE_DENCE_HASH> {
    using type = google::dense_hash_map<std::string, int>;
    static std::string name() {
        return "google_dense_hash_map";
    }
};
template <>
struct standard_map_trait<standard_map_types::GOOGLE_SPARSE_HASH> {
    using type = google::sparse_hash_map<std::string, int>;
    static std::string name() {
        return "google_sparse_hash_map";
    }
};
template <>
struct standard_map_trait<standard_map_types::SPP> {
    using type = spp::sparse_hash_map<std::string, int>;
    static std::string name() {
        return "sparsepp";
    }
};

template <standard_map_types T>
class standard_map_wrapper {
  public:
    explicit standard_map_wrapper(const std::vector<std::string>&) {
        if constexpr (T == standard_map_types::GOOGLE_DENCE_HASH) {
            dict_.set_empty_key("");
        }
    }

    static std::string name() {
        return standard_map_trait<T>::name();
    }
    bool insert(const std::string& key) {
        return dict_.insert(std::make_pair(key, 1)).second;
    }
    bool search(const std::string& key) {
        return dict_.find(key) != dict_.end();
    }
    void show_stat(std::ostream& os) const {}

  private:
    typename standard_map_trait<T>::type dict_;
};

/**
 *  Tessil impl.
 */
enum class tsl_map_types { ARRAY_HASH, HAT_TRIE, HOPSCOTCH, ROBIN };

template <tsl_map_types>
struct tsl_map_trait;

template <>
struct tsl_map_trait<tsl_map_types::ARRAY_HASH> {
    using type = tsl::array_map<char, int>;
    static std::string name() {
        return "tsl_array_hash";
    }
};
template <>
struct tsl_map_trait<tsl_map_types::HAT_TRIE> {
    using type = tsl::htrie_map<char, int>;
    static std::string name() {
        return "tsl_hat_trie";
    }
};
template <>
struct tsl_map_trait<tsl_map_types::HOPSCOTCH> {
    using type = tsl::hopscotch_map<std::string, int>;
    static std::string name() {
        return "tsl_hopscotch_map";
    }
};
template <>
struct tsl_map_trait<tsl_map_types::ROBIN> {
    using type = tsl::robin_map<std::string, int>;
    static std::string name() {
        return "tsl_robin_map";
    }
};

template <tsl_map_types T>
class tsl_map_wrapper {
  public:
    explicit tsl_map_wrapper(const std::vector<std::string>&) {}

    static std::string name() {
        return tsl_map_trait<T>::name();
    }
    bool insert(const std::string& key) {
        if constexpr ((T == tsl_map_types::ARRAY_HASH) or (T == tsl_map_types::HAT_TRIE)) {
            return dict_.insert(key, 1).second;
        } else {
            return dict_.insert({key, 1}).second;
        }
    }
    bool search(const std::string& key) {
        return dict_.find(key) != dict_.end();
    }
    void show_stat(std::ostream& os) const {}

  private:
    typename tsl_map_trait<T>::type dict_;
};

/**
 *  Judy
 */
class judy_wrapper {
  public:
    explicit judy_wrapper(const std::vector<std::string>&) {}

    ~judy_wrapper() {
        Word_t bytes = 0;
        JSLFA(bytes, dic_);
    }
    static std::string name() {
        return "judySL";
    }
    bool insert(const std::string& key) {
        Pvoid_t ptr = nullptr;
        JSLI(ptr, dic_, reinterpret_cast<const uint8_t*>(key.c_str()));
        auto p_word = reinterpret_cast<PWord_t>(ptr);
        *p_word = 1;
        return true;
    }
    bool search(const std::string& key) {
        Pvoid_t ptr = nullptr;
        JSLG(ptr, dic_, reinterpret_cast<const uint8_t*>(key.c_str()));
        auto p_word = reinterpret_cast<PWord_t>(ptr);
        return *p_word == 1;
    }
    void show_stat(std::ostream& os) const {}

  private:
    Pvoid_t dic_ = nullptr;
};

/**
 *  ART
 */
class art_wrapper {
  public:
    explicit art_wrapper(const std::vector<std::string>&) {
        art_tree_init(&dict_);
    }
    ~art_wrapper() {
        destroy_art_tree(&dict_);
    }
    static std::string name() {
        return "art";
    }
    bool insert(const std::string& key) {
        uintptr_t value = 1;
        return art_insert(&dict_, reinterpret_cast<const unsigned char*>(key.c_str()),
                          static_cast<int>(key.length() + 1),  // with terminator
                          reinterpret_cast<void*>(value)) == nullptr;
    }
    bool search(const std::string& key) {
        return reinterpret_cast<uintptr_t>(art_search(&dict_, reinterpret_cast<const unsigned char*>(key.c_str()),
                                                      static_cast<int>(key.length()) + 1  // with terminator
                                                      )) == 1;
    }
    void show_stat(std::ostream& os) const {}

  private:
    art_tree dict_;
};

/**
 *  Cedar
 */
enum class cedar_types { TRIE, MP_TRIE };

template <cedar_types>
struct cedar_wrapper_trait;
template <>
struct cedar_wrapper_trait<cedar_types::TRIE> {
    using type = cedar::da<int, -1, -2, false>;
    static std::string name() {
        return "cedar";
    }
};
template <>
struct cedar_wrapper_trait<cedar_types::MP_TRIE> {
    using type = cedarpp::da<int, -1, -2, false>;
    static std::string name() {
        return "cedarpp";
    }
};

template <cedar_types T>
class cedar_wrapper {
  public:
    explicit cedar_wrapper(const std::vector<std::string>&) {}

    static std::string name() {
        return cedar_wrapper_trait<T>::name();
    }
    bool insert(const std::string& key) {
        dict_.update(key.c_str(), key.length()) = 1;
        return true;
    }
    bool search(const std::string& key) {
        return dict_.template exactMatchSearch<int>(key.c_str(), key.length()) == 1;
    }
    void show_stat(std::ostream& os) const {
        os << "capacity:" << dict_.capacity() << "\n";
        os << "size:" << dict_.size() << "\n";
        if constexpr (T == cedar_types::MP_TRIE) {
            os << "length:" << dict_.length() << "\n";
        }
        os << "total_size:" << dict_.total_size() << "\n";
        os << "unit_size:" << dict_.unit_size() << "\n";
        os << "nonzero_size:" << dict_.nonzero_size() << "\n";
        if constexpr (T == cedar_types::MP_TRIE) {
            os << "nonzero_length:" << dict_.nonzero_length() << "\n";
        }
        os << "num_keys:" << dict_.num_keys() << "\n";
    }

  private:
    typename cedar_wrapper_trait<T>::type dict_;
};

/**
 *  Poplar
 */
enum class poplar_types {
    PLAIN_BONSAI,
    SEMI_COMPACT_BONSAI,
    COMPACT_BONSAI,
    PLAIN_FKHASH,
    SEMI_COMPACT_FKHASH,
    COMPACT_FKHASH,
};

template <poplar_types, uint64_t ChunkSize>
struct poplar_wrapper_trait;

template <uint64_t ChunkSize>
struct poplar_wrapper_trait<poplar_types::PLAIN_BONSAI, ChunkSize> {
    using type = poplar::plain_bonsai_map<int>;
    static std::string name() {
        return "poplar_plain_bonsai (PDT-PB)";
    }
};
template <uint64_t ChunkSize>
struct poplar_wrapper_trait<poplar_types::SEMI_COMPACT_BONSAI, ChunkSize> {
    using type = poplar::semi_compact_bonsai_map<int, ChunkSize>;
    static std::string name() {
        return "poplar_semi_compact_bonsai_" + std::to_string(ChunkSize) + " (PDT-SB)";
    }
};
template <uint64_t ChunkSize>
struct poplar_wrapper_trait<poplar_types::COMPACT_BONSAI, ChunkSize> {
    using type = poplar::compact_bonsai_map<int, ChunkSize>;
    static std::string name() {
        return "poplar_compact_bonsai_" + std::to_string(ChunkSize) + " (PDT-CB)";
    }
};
template <uint64_t ChunkSize>
struct poplar_wrapper_trait<poplar_types::PLAIN_FKHASH, ChunkSize> {
    using type = poplar::plain_fkhash_map<int>;
    static std::string name() {
        return "poplar_plain_fkhash (PDT-PFK)";
    }
};
template <uint64_t ChunkSize>
struct poplar_wrapper_trait<poplar_types::SEMI_COMPACT_FKHASH, ChunkSize> {
    using type = poplar::semi_compact_fkhash_map<int, ChunkSize>;
    static std::string name() {
        return "poplar_semi_compact_fkhash_" + std::to_string(ChunkSize) + " (PDT-SFK)";
    }
};
template <uint64_t ChunkSize>
struct poplar_wrapper_trait<poplar_types::COMPACT_FKHASH, ChunkSize> {
    using type = poplar::compact_fkhash_map<int, ChunkSize>;
    static std::string name() {
        return "poplar_compact_fkhash_" + std::to_string(ChunkSize) + " (PDT-CFK)";
    }
};

template <poplar_types T, uint64_t ChunkSize = 0>
class poplar_wrapper {
  public:
    explicit poplar_wrapper(const std::vector<std::string>& args) {
        if (args.size() >= 2) {
            uint32_t capa_bits = std::stoul(args[0]);
            uint64_t lambda = std::stoul(args[1]);
            dict_ = typename poplar_wrapper_trait<T, ChunkSize>::type(capa_bits, lambda);
        }
    }
    static std::string name() {
        return poplar_wrapper_trait<T, ChunkSize>::name();
    }
    bool insert(const std::string& key) {
        *dict_.update(poplar::make_char_range(key)) = 1;
        return true;
    }
    bool search(const std::string& key) {
        return *dict_.find(poplar::make_char_range(key));
    }
    void show_stat(std::ostream& os) const {
        dict_.show_stats(os);
    }

  private:
    typename poplar_wrapper_trait<T, ChunkSize>::type dict_;
};

template <class Wrapper>
int bench(int runs, const std::string& key_fn, const std::string& query_fn, const std::vector<std::string>& args) {
    auto wrapper = std::make_unique<Wrapper>(args);

    size_t num_keys = 0, num_queries = 0;
    size_t ok = 0, ng = 0;
    size_t process_size = get_process_size();
    double constr_sec = 0.0;
    double insert_us_per_key = 0.0, search_us_per_query = 0.0;
    double best_insert_us_per_key = 0.0, best_search_us_per_query = 0.0;

    {
        std::ifstream ifs(key_fn);
        if (!ifs) {
            std::cerr << "open error: key_fn = " << key_fn << std::endl;
            return 1;
        }

        std::string key;
        key.reserve(1024);

        size_t num_keys = 0;

        timer t;
        while (std::getline(ifs, key)) {
            wrapper->insert(key);
            ++num_keys;
        }
        constr_sec = t.get<>();
        process_size = get_process_size() - process_size;
    }

    std::shared_ptr<std::vector<std::string>> keys;
    std::shared_ptr<std::vector<std::string>> queries;

    {
        std::ifstream ifs(key_fn);
        if (!ifs) {
            std::cerr << "open error: key_fn = " << key_fn << std::endl;
            return 1;
        }
        keys = std::make_shared<std::vector<std::string>>();
        for (std::string line; std::getline(ifs, line);) {
            keys->push_back(line);
        }
    }

    if (query_fn != "-") {
        std::ifstream ifs(query_fn);
        if (!ifs) {
            std::cerr << "open error: query_fn = " << query_fn << std::endl;
            return 1;
        }
        queries = std::make_shared<std::vector<std::string>>();
        for (std::string line; std::getline(ifs, line);) {
            queries->push_back(line);
        }
    } else {
        queries = keys;
    }

    {
        std::vector<double> insert_times(runs);
        std::vector<double> search_times(runs);

        for (int i = 0; i < runs; ++i) {
            wrapper = std::make_unique<Wrapper>(args);

            // insertion
            {
                timer t;
                for (const std::string& key : *keys) {
                    wrapper->insert(key);
                }
                insert_times[i] = t.get<std::micro>() / keys->size();
            }

            // retrieval
            size_t _ok = 0, _ng = 0;
            {
                timer t;
                for (const std::string& query : *queries) {
                    if (wrapper->search(query)) {
                        ++_ok;
                    } else {
                        ++_ng;
                    }
                }
                search_times[i] = t.get<std::micro>() / queries->size();
            }

            if (i != 0) {
                if ((ok != _ok) or (ng != _ng)) {
                    std::cerr << "critical error for search results" << std::endl;
                    return 1;
                }
            }

            ok = _ok;
            ng = _ng;
        }

        num_keys = keys->size();
        num_queries = queries->size();
        insert_us_per_key = get_average(insert_times);
        best_insert_us_per_key = get_min(insert_times);
        search_us_per_query = get_average(search_times);
        best_search_us_per_query = get_min(search_times);
    }

    std::cout << "mode:measure\n"
              << "name:" << Wrapper::name() << '\n'
              << "key_fn:" << key_fn << '\n'
              << "query_fn:" << query_fn << '\n'
              << "insert_runs:" << runs << '\n'
              << "num_keys:" << num_keys << '\n'
              << "insert_us_per_key:" << insert_us_per_key << '\n'
              << "best_insert_us_per_key:" << best_insert_us_per_key << '\n'
              << "search_runs:" << runs << '\n'
              << "num_queries:" << num_queries << '\n'
              << "search_us_per_query:" << search_us_per_query << '\n'
              << "best_search_us_per_query:" << best_search_us_per_query << '\n'
              << "ok:" << ok << '\n'
              << "ng:" << ng << '\n'
              << "process_size:" << process_size << '\n';
    std::cout << "-- extra stats --\n";
    wrapper->show_stat(std::cout);

    return 0;
}

// clang-format off
using wrapper_types = std::tuple<standard_map_wrapper<standard_map_types::STD_MAP>,
                                 standard_map_wrapper<standard_map_types::STD_HASH>,
                                 standard_map_wrapper<standard_map_types::GOOGLE_DENCE_HASH>,
                                 standard_map_wrapper<standard_map_types::SPP>, 
                                 tsl_map_wrapper<tsl_map_types::ARRAY_HASH>,
                                 tsl_map_wrapper<tsl_map_types::HAT_TRIE>,
                                 tsl_map_wrapper<tsl_map_types::HOPSCOTCH>,
                                 tsl_map_wrapper<tsl_map_types::ROBIN>,
                                 judy_wrapper,
                                 art_wrapper,
                                 cedar_wrapper<cedar_types::TRIE>, 
                                 cedar_wrapper<cedar_types::MP_TRIE>,
                                 poplar_wrapper<poplar_types::PLAIN_BONSAI>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_BONSAI, 8>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_BONSAI, 16>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_BONSAI, 32>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_BONSAI, 64>,
                                 poplar_wrapper<poplar_types::COMPACT_BONSAI, 8>,
                                 poplar_wrapper<poplar_types::COMPACT_BONSAI, 16>,
                                 poplar_wrapper<poplar_types::COMPACT_BONSAI, 32>,
                                 poplar_wrapper<poplar_types::COMPACT_BONSAI, 64>,
                                 poplar_wrapper<poplar_types::PLAIN_FKHASH>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_FKHASH, 8>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_FKHASH, 16>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_FKHASH, 32>,
                                 poplar_wrapper<poplar_types::SEMI_COMPACT_FKHASH, 64>,
                                 poplar_wrapper<poplar_types::COMPACT_FKHASH, 8>,
                                 poplar_wrapper<poplar_types::COMPACT_FKHASH, 16>,
                                 poplar_wrapper<poplar_types::COMPACT_FKHASH, 32>,
                                 poplar_wrapper<poplar_types::COMPACT_FKHASH, 64>
                                 >;
// clang-format on

constexpr size_t NUM_WRAPPERS = std::tuple_size<wrapper_types>::value;

template <int N = 0>
int run(const cmdline::parser& p) {
    if constexpr (N >= NUM_WRAPPERS) {
        std::cerr << "error: wrapper_id is out of range.\n";
        return 1;
    } else {
        if (p.get<int>("wrapper_id") - 1 == N) {
            using wrapper_type = std::tuple_element_t<N, wrapper_types>;
            return bench<wrapper_type>(p.get<int>("runs"), p.get<std::string>("key_fn"), p.get<std::string>("query_fn"),
                                       p.rest());
        }
        return run<N + 1>(p);
    }
}

template <typename Types, size_t N = std::tuple_size_v<Types>>
inline void list_all(const char* pfx, std::ostream& os) {
    static_assert(N != 0);
    if constexpr (N > 1) {
        list_all<Types, N - 1>(pfx, os);
    }
    using type = std::tuple_element_t<N - 1, Types>;
    os << pfx << std::setw(2) << N << ": " << type::name() << '\n';
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);

    cmdline::parser p;
    p.add<int>("wrapper_id", 'w', "type id of dictionary wrappers", false, 2);
    p.add<std::string>("key_fn", 'k', "input file name of keywords", false, "");
    p.add<std::string>("query_fn", 'q', "input file name of queries", false, "-");
    p.add<int>("runs", 'r', "# of runs", false, 10);
    p.add<bool>("list_all", 'l', "list all dictionary wrappers", false, false);
    p.parse_check(argc, argv);

    if (p.get<bool>("list_all")) {
        std::cerr << "wrapper_ids:\n";
        list_all<wrapper_types>("  - ", std::cerr);
        return 1;
    }

    if ((p.get<std::string>("key_fn").empty()) or (run<>(p) != 0)) {
        std::cerr << p.usage();
        std::cerr << "wrapper_ids:\n";
        list_all<wrapper_types>("  - ", std::cerr);
        return 1;
    }

    return 0;
}