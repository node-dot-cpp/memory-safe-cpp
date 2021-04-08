#-------------------------------------------------------------------------------------------
# Copyright (c) 2021, OLogN Technologies AG
#-------------------------------------------------------------------------------------------

#-------------------------------------------------------------------------------------------
# Executable definition
#-------------------------------------------------------------------------------------------

add_executable(test_dezombiefy_iterators test_dezombiefy_iterators.cpp)
target_link_libraries(test_dezombiefy_iterators safememory_dezombiefy_iterators)

#-------------------------------------------------------------------------------------------
# Run Unit tests and verify the results.
#-------------------------------------------------------------------------------------------
add_test(test_dezombiefy_iteratorsRun test_dezombiefy_iterators)
