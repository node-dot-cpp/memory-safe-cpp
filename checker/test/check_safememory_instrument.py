#!/usr/bin/env python
#
#===- check_safememory_instrument.py - Nodecpp Test Helper -----*- python -*--===#
#
#                     The LLVM Compiler Infrastructure
#
# This file is distributed under the University of Illinois Open Source
# License. See LICENSE.TXT for details.
#
#===------------------------------------------------------------------------===#

r"""
Nodecpp Test Helper
=====================

This script runs safememory-instrument and verify changes and messages.

Usage:
  check_safememory_instrument.py <source-file> <temp-file> \
    -- [optional safememory-instrument arguments]

Example:
  // RUN: %check_safememory_instrument %s %t -- -- -isystem %S/Inputs
"""

import argparse
import os
import re
import subprocess
import sys


def write_file(file_name, text):
  with open(file_name, 'w') as f:
    f.write(text)
    f.truncate()

def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('input_file_name')
  parser.add_argument('temp_file_name')

  args, extra_args = parser.parse_known_args()

  input_file_name = args.input_file_name
  temp_file_name = args.temp_file_name

  _, extension = os.path.splitext(input_file_name)
  if extension not in ['.c', '.hpp', '.m', '.mm']:
    extension = '.cpp'

  cleaned_file_name = temp_file_name + ".clean" + extension
  temp_file_name = temp_file_name + extension

  check_fixes_prefix = 'CHECK-FIXES'
  check_messages_prefix = 'CHECK-MESSAGES'

  with open(input_file_name, 'r') as input_file:
    input_text = input_file.read()

  has_check_fixes = check_fixes_prefix in input_text
  has_check_messages = check_messages_prefix in input_text

  if not has_check_fixes and not has_check_messages:
    sys.exit('Neither %s nor %s found in the input' % (check_fixes_prefix, check_messages_prefix) )

  # Remove the contents of the CHECK lines to avoid CHECKs matching on
  # themselves.  We need to keep the comments to preserve line numbers while
  # avoiding empty lines which could potentially trigger formatting-related
  # checks.
  cleaned_test = re.sub('// *CHECK-[A-Z0-9\-]*:[^\r\n]*', '//', input_text)

#  write_file(temp_file_name, cleaned_test)

#  cleaned_file_name = temp_file_name + ".orig"
  write_file(cleaned_file_name, cleaned_test)

  args = ['safememory-instrument', '-o', temp_file_name, cleaned_file_name] + \
        extra_args

  print('Running ' + repr(args) + '...')
  try:
    clang_tidy_output = \
        subprocess.check_output(args, stderr=subprocess.STDOUT).decode()
  except subprocess.CalledProcessError as e:
    print('safememory-instrument failed:\n' + e.output.decode())
    raise

  print('----------------------------- output -----------------------------\n' +
        clang_tidy_output +
        '\n------------------------------------------------------------------')

  # try:
  #   diff_output = subprocess.check_output(
  #       ['diff', '-u', original_file_name, temp_file_name],
  #       stderr=subprocess.STDOUT)
  # except subprocess.CalledProcessError as e:
  #   diff_output = e.output

  # print('------------------------------ Fixes -----------------------------\n' +
  #       diff_output.decode() +
  #       '\n------------------------------------------------------------------')

  if has_check_fixes:
    try:
      subprocess.check_output(
          ['FileCheck', '-input-file=' + temp_file_name, input_file_name,
           '-check-prefix=' + check_fixes_prefix],
          stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
      print('FileCheck failed:\n' + e.output.decode())
      raise

  if has_check_messages:
    messages_file = temp_file_name + '.msg'
    write_file(messages_file, clang_tidy_output)
    try:
      subprocess.check_output(
          ['FileCheck', '-input-file=' + messages_file, input_file_name,
           '-check-prefix=' + check_messages_prefix,
           '-implicit-check-not={{warning|error}}:'],
          stderr=subprocess.STDOUT)
    except subprocess.CalledProcessError as e:
      print('FileCheck failed:\n' + e.output.decode())
      raise

if __name__ == '__main__':
  main()
