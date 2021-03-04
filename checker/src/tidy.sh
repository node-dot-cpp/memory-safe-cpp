#!/bin/sh

./run-clang-tidy.py -fix -format -p ../build/debug/ -header-filter=safememory-checker -clang-tidy-binary=clang-tidy-7 -clang-apply-replacements-binary=clang-apply-replacements-7 safememory-checker
