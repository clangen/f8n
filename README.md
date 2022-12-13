# f8n
really boring, basic foundational code i find myself rewriting every time i start a new project.

the half-baked goal for this repo is to maintain a small set of simple, usable (and in most cases dumb) functions, interfaces and abstractions that c++ doesn't provide out of the box.

it includes things like:
  - a simple, re-usable event loop
  - an easy to use http client
  - basic string manipulation utilities
  - daemon creation with automatic signal handling
  - wrappers around low-level sqlite functionality
  - dynamically loadable plugins
  - preference handling
  - debugging
  - unicode stuff to reduce boilerplate in console-based programs and win32 apps.

on Windows it provides pre-configured versions of:
  - sqlite
  - openssl
  - curl
  - zlib
  - PDCursesMod
  
it also includes the following libraries for all platforms:
  - sigslot
  - json.hpp

`f8n` is probably not useful to anyone except me.
