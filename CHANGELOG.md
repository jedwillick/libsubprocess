# Changelog

## 1.0.0 (2023-03-12)


### Features

* add a few tests ([7ae6574](https://github.com/jedwillick/libsubprocess/commit/7ae6574e94348984e211933d39de5b5014e04e62))
* add code coverage ([95ef8fd](https://github.com/jedwillick/libsubprocess/commit/95ef8fd60b1971487f7dcebe917206972ba1769b))
* add detach, and inheritFds options when setting up a process. ([01f3bee](https://github.com/jedwillick/libsubprocess/commit/01f3bee45877dd2e9fd95223d4fbde9b064a2723))
* add install/uninstall targets ([a0c9b3c](https://github.com/jedwillick/libsubprocess/commit/a0c9b3c7ad8de85c0a07bd3f9861b159ed379441))
* add redirection and pipes ([bc7fdf0](https://github.com/jedwillick/libsubprocess/commit/bc7fdf091a223357a465c5f39c4c334da65f69f5))
* added bear target to generate compile_commands ([5440a81](https://github.com/jedwillick/libsubprocess/commit/5440a81374a9a0b54eb78a6807812e4a42cd4b1f))
* Added separate memcheck test instead of rerunning all tests ([6798bd3](https://github.com/jedwillick/libsubprocess/commit/6798bd3f6189b7ee746436cedd3db72a49bf8a91))
* **ci:** run clang-format and commit changes ([8634782](https://github.com/jedwillick/libsubprocess/commit/8634782df1fe9f139979e80e3716c438958e2ce8))
* improve dupe_array() and add free_array() ([254923d](https://github.com/jedwillick/libsubprocess/commit/254923db9fca811412317a0c12957a762c52876b))
* improve error handling, rearrange directory structure ([05dd5c9](https://github.com/jedwillick/libsubprocess/commit/05dd5c9d2c4aec44e2a373d6f6070699107616ab))
* initial functionality without pipes/redirecton ([91acd2c](https://github.com/jedwillick/libsubprocess/commit/91acd2ce3a8e0d98de4c2564c74301ef2fac02e9))
* refactor io.h into redirect .h. Add more tests ([0108998](https://github.com/jedwillick/libsubprocess/commit/0108998556209b40aa468b0b4c8de73f07529e8b))
* **static:** add targets to build a static library ([d924e11](https://github.com/jedwillick/libsubprocess/commit/d924e110b763d43f136c07fbeac252ffe55eba47))
* **test:** add tests for the inheritFds option ([17acc66](https://github.com/jedwillick/libsubprocess/commit/17acc66c3e8fda59bed18f97e3e07010830d98e3))
* WIP subprocess library w/ initial functionality ([cf55e98](https://github.com/jedwillick/libsubprocess/commit/cf55e983cde3f2e3931fa1c26f030470a379beb9))


### Bug Fixes

* add missing header file ([880db7d](https://github.com/jedwillick/libsubprocess/commit/880db7deacb194045d1024f4d09361d794418615))
* check for insufficient memory when spawning process ([d2ab20b](https://github.com/jedwillick/libsubprocess/commit/d2ab20b4d02c9234b003262aad43c8cd093eee85))
* **ci:** s/ubuntu-latest/ubuntu-22.04 ([5eaf95b](https://github.com/jedwillick/libsubprocess/commit/5eaf95be68c144dc622b833c05c8ac4ca89760ef))
* **ci:** use pipx to install clang-format 15 ([95ead76](https://github.com/jedwillick/libsubprocess/commit/95ead76a8ae6b011b42221422ffcbf2a37344eda))
* Compile with _GNU_SOURCE. Properly link criterion in tests. ([64912e8](https://github.com/jedwillick/libsubprocess/commit/64912e8e3aaa62dbb6ddad7d984773d5258a63de))
* include subprocess/pipe.h in subprocess/process.h ([c66237e](https://github.com/jedwillick/libsubprocess/commit/c66237ebf8d52da789106d70a7a0a0126563a6f6))
* memcheck properly reports leaks ([412915d](https://github.com/jedwillick/libsubprocess/commit/412915d77cc2b3f06ec888418eace4f903aaeb17))
* **memcheck:** ensure library is built before running memcheck ([7667530](https://github.com/jedwillick/libsubprocess/commit/7667530eb1be741451bba0582f3970fb95fb8f92))
* only define _GNU_SOURCE when needed ([14aad9b](https://github.com/jedwillick/libsubprocess/commit/14aad9bbb056df52f0bdc1851d855b2644d0258e))
* set errno in sp_fd_to_redir_opts() when fd is unknown ([a5475f2](https://github.com/jedwillick/libsubprocess/commit/a5475f21dd5c954b2c0e154c3c17b4d8c2bb11a6))
* **test:** wait for process before trying to poll ([7f961df](https://github.com/jedwillick/libsubprocess/commit/7f961dfcd212ff4c23a3a8864f51fba1d2122d23))
