#!/usr/bin/env ruby

NTIMES = 10
TESTS = ["memcpy-simple", "memcpy-blocks", "pipe-copy", "shm-1buf", "shm-2buf", "uds-copy"]

SIZES = 0..10
SCALE = 50_000_000

TESTS.each { |test|
  puts "#{test}"
  SIZES.each { |size|
    size = size * SCALE
    total = 0.0
    NTIMES.times {
      time_str = `./#{test} #{size} 2> /dev/null`
      if time_str =~ /TIME: ([0-9\.]+)/
        total += $1.to_f
      else
        puts "ERROR: #{test} #{size}"
      end
    }
    average = total/NTIMES
    puts "#{size/1000}, #{average}\n"
  }
}
