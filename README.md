# dictionary_bench

This software is that we used for the experiments of Sect 6.5 and Appendix A in our paper "[Dynamic Path-Decomposed Tries](https://arxiv.org/abs/1906.06015)".

There are the source codes for other experiments at [poplar-trie/bench](https://github.com/kampersanda/poplar-trie/tree/master/bench).

## Build instructions

You can download and compile this software as the following commands.

```
$ git clone https://github.com/kampersanda/dictionary_bench.git
$ cd dictionary_bench
$ mkdir build
$ cd build
$ cmake ..
$ make
```

The library uses C++17, so please install g++ 7.0 (or greater) or clang 4.0 (or greater).
In addition, CMake 2.8 (or greater) has to be installed to compile the library.

On the default setting, the library attempts to use `SSE4.2` for popcount primitives.
If you do not want to use it, please set `DISABLE_SSE4_2` at build time, e.g., `cmake .. -DDISABLE_SSE4_2=1`.

Please install libraries [Judy](http://judy.sourceforge.net) and [sparsehash](https://github.com/sparsehash/sparsehash). The other data structures are contained in this package.

## Running example 

Through the compiling command, executable file `bench` will be created.

```
$ ./bench 
usage: ./bench [options] ... 
options:
  -w, --wrapper_id    type id of dictionary wrappers (int [=2])
  -k, --key_fn        input file name of keywords (string [=])
  -q, --query_fn      input file name of queries (string [=-])
  -r, --runs          # of runs (int [=10])
  -l, --list_all      list all dictionary wrappers (bool [=0])
  -?, --help          print this message
wrapper_ids:
  -  1: std_map
  -  2: std_unordered_map
  -  3: google_dense_hash_map
  -  4: sparsepp
  -  5: tsl_array_hash
  -  6: tsl_hat_trie
  -  7: tsl_hopscotch_map
  -  8: tsl_robin_map
  -  9: judySL
  - 10: art
  - 11: cedar
  - 12: cedarpp
  - 13: poplar_plain_bonsai (PDT-PB)
  - 14: poplar_semi_compact_bonsai_8 (PDT-SB)
  - 15: poplar_semi_compact_bonsai_16 (PDT-SB)
  - 16: poplar_semi_compact_bonsai_32 (PDT-SB)
  - 17: poplar_semi_compact_bonsai_64 (PDT-SB)
  - 18: poplar_compact_bonsai_8 (PDT-CB)
  - 19: poplar_compact_bonsai_16 (PDT-CB)
  - 20: poplar_compact_bonsai_32 (PDT-CB)
  - 21: poplar_compact_bonsai_64 (PDT-CB)
  - 22: poplar_plain_fkhash (PDT-PFK)
  - 23: poplar_semi_compact_fkhash_8 (PDT-SFK)
  - 24: poplar_semi_compact_fkhash_16 (PDT-SFK)
  - 25: poplar_semi_compact_fkhash_32 (PDT-SFK)
  - 26: poplar_semi_compact_fkhash_64 (PDT-SFK)
  - 27: poplar_compact_fkhash_8 (PDT-CFK)
  - 28: poplar_compact_fkhash_16 (PDT-CFK)
  - 29: poplar_compact_fkhash_32 (PDT-CFK)
  - 30: poplar_compact_fkhash_64 (PDT-CFK)
```

For example, you can test `dynpdt_plain_bonsai` as follows.

```
$ ./bench -w 13 -k jawiki.10000 -q jawiki.10000 
mode:measure
name:poplar_plain_bonsai (PDT-PB)
key_fn:jawiki.10000
query_fn:jawiki.10000
insert_runs:10
num_keys:10000
insert_us_per_key:0.163162
best_insert_us_per_key:0.156172
search_runs:10
num_queries:10000
search_us_per_query:0.0955724
best_search_us_per_query:0.0934902
ok:10000
ng:0
process_size:1097728
-- extra stats --
name:map
lambda:32
size:9976
hash_trie_:
    name:plain_bonsai_trie
    factor:15.2267
    max_factor:90
    size:9979
    capa_bits:16
    symb_bits:13
label_store_:
    name:plain_bonsai_nlm
    size:9976
    num_ptrs:65536
```