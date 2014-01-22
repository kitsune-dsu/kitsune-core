#!/usr/bin/env ruby

NTIMES = 4

TESTS = ["memcpy-blocks"]

SIZES = 0..4
SCALE = 50_000_000

BLOCK_ARG = 1..10
BLOCK_SCALE = 40_000

TESTS.each { |test|
  puts "#{test}"

  BLOCK_ARG.each {|block|
    block_val = block * BLOCK_SCALE
    puts "BLOCK: #{block_val}"

    SIZES.each { |size|
      size = size * SCALE
      total = 0.0
      NTIMES.times {
        time_str = `./#{test} #{size} #{block_val} 2> /dev/null`
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
}
