#ifndef REBALANCE_UTIL_HPP
#define REBALANCE_UTIL_HPP

#include <functional>
#include <iostream>
#include <sstream>
#include <tuple>

namespace stdx {
  template<int i, int n, class tup>
  struct hash_tuple {
    std::size_t operator()(size_t h, const tup &x) const {
      h ^= std::hash<typename std::tuple_element<i,tup>::type>()(std::get<i>(x));
      h *= 0x9e3779b97f4a7c15u;
      return hash_tuple<i+1,n,tup>()(h, x);
    }
  };
  
  template<int n, class tup>
  struct hash_tuple<n,n,tup> {
    std::size_t operator()(size_t h, const tup &x) const {
      return h;
    }
  };
}

namespace std {
  template<class ...Ts>
  std::size_t hash_of(const Ts ...xs) {
    return stdx::hash_tuple<0, sizeof...(Ts), std::tuple<Ts...>>()(0, std::tuple<Ts...>(xs...));
  }

  /*template<class T>
  struct hash {
    std::size_t operator()(const T &x) const {
      return hash_of(x);
    }
  };*/

  template<class ...Ts>
  struct hash<std::tuple<Ts...>> {
    std::size_t operator()(std::tuple<Ts...> x) const {
      return stdx::hash_tuple<0, sizeof...(Ts), std::tuple<Ts...>>()(0, x);
    }
  };

  // grabbed from jdbachan's typeclass.hxx
  template<class A, class B>
  struct hash<pair<A,B>> {
    inline size_t operator()(const pair<A,B> &x) const {
      size_t h = hash<A>()(x.first);
      h ^= h >> 13;
      h *= 41;
      h += hash<B>()(x.second);
      return h;
    }
  };
}
#endif