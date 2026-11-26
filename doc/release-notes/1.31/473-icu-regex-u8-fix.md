## Bug Fixes

### Minor Bug Fixes

* Fixed ICU regex compilation error (`uregex_openUText failed`, error 0x8a150033) by ensuring version normalization regex literals are UTF-8 encoded using `u8R"(...)"` prefix. This resolves build failures on systems with non-UTF-8 active code pages (e.g., Windows with Russian/Ukrainian locale ACP 1251).