# f8n
foundational code i tend to find myself rewriting over and over again in every new project i start.

the half-baked design goal for this repo is to maintain a small set of simple, usable (and in most cases dumb) abstractions that c++ doesn't provide out of the box. things include:
  - a lightweight set of unicode handling functionality to reduce boilerplate in utf16 windows apps and console-based programs.
  - an http client
  - wrappers around low-level sqlite functionality
  - dynamically loadable plugins
  - preference handling
  - debugging

this is probably not useful to anyone except me.
