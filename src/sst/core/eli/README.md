![SST](http://sst-simulator.org/img/sst-logo-small.png)

# Element Library Concepts

Here we introduce the classes necessary to understanding how SST element libraries work.
The most difficult (and possibly most important) thing to understand are the C++ Concepts defining the interfaces between functions. To be used in a particular context, classes must define a minimal set of type aliases (typedefs) and functions.

# Element Builders

The element builders are used for making derived types of a base class based on string inputs.
````
Base* b = create<Base>(name, args...);
````
This eventually needs to call
````
Derived* d = new Derived(args...);
return d;
````
## Constructor Concept

A constructor class represents a particular instance of a constructor, usually via a variadic template `Args...` that represent the list of arguments in the constructor. The constructor concept is meant to encapsulate any number of valid constructors rather than a single set of `Args...`.  A class meeting the constructor concept needs two things.
* A template function `add` that adds the derived type to the Base's registry for all matching constructors.
````
template <class T> void add(){
  ...
  Base::addBuilder(...);
}
````
* A template type alias `is_constructible` that says whether a give type `T` provides any of the valid constructors for a given `Base`, e.g.
````
template <class T>
using is_constructible = std::is_constructible<T,Args>;
````

### Single Constructor
The simplest class meeting the constructor concept is `SingleCtor` which represents a single valid constructor of `Args...`.

### Constructor Lists
A more complicated constructor is the `CtorList` that represents a list of possible valid constructors.
This class should register builders matching at least one of the constructors in the list.

### Extended Constructor
The extended constructor is the most complicated version. This represents a second base class, i.e.
a given class `Derived` might be created via either:
````
Base1* b1 = create<Base1>(name, args...);
````
or
````
Base2* b2 = create<Base2>(name, args...);
````
where `Base2` itself inherits from `Base1`.
The derived constructor for `Base2` must register a class `T` to itself and to the original `Base1`.
The meaning of the type alias `is_constructible` applies only to the most derived base class.

#### Copyright (c) 2009-2022, National Technology and Engineering Solutions of Sandia, LLC (NTESS)
