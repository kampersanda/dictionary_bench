#ifndef POPLAR_TRIE_EXCEPTION_HPP
#define POPLAR_TRIE_EXCEPTION_HPP

#include <exception>

namespace poplar {

class exception : public std::exception {
 public:
  explicit exception(const char* msg) : msg_{msg} {}
  ~exception() throw() override = default;

  const char* what() const throw() override {
    return msg_;
  }

 private:
  const char* msg_;
};

#define POPLAR_TO_STR_(n) #n
#define POPLAR_TO_STR(n) POPLAR_TO_STR_(n)
#define POPLAR_THROW(msg) throw poplar::exception(__FILE__ ":" POPLAR_TO_STR(__LINE__) ":" msg)
#define POPLAR_THROW_IF(cond, msg) (void)((!(cond)) || (POPLAR_THROW(msg), 0))

}  // namespace poplar

#endif  // POPLAR_TRIE_EXCEPTION_HPP
