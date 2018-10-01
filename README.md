# f8n
foundational code i tend to find myself rewriting over and over again in every new project i start.

the half-baked design goal for this repo is to maintain a small set of simple, usable (and in most cases dumb) abstractions that c++ doesn't provide out of the box. things include:
  - an easy to use http client
  - wrappers around low-level sqlite functionality
  - dynamically loadable plugins
  - preference handling
  - debugging
  - a lightweight set of unicode handling functionality to reduce boilerplate in utf16 windows apps and console-based programs.

on Windows it provides pre-configured versions of
  - sqlite
  - libressl
  - curl
  - zlib
  - pdcurses
  
it also includes the following libraries for all platforms:
  - sigslot
  - json.hpp

`f8n` is probably not useful to anyone except me.
