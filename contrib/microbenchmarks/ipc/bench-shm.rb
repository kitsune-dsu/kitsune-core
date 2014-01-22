#!/usr/bin/env ruby

NTIMES = 4

TESTS = ["shm-1buf", "shm-2buf"]

SIZES = 0..4
SCALE = 50_000_000

SHM_ARG = 1..10
SHM_SCALE = 40_000

TESTS.each { |test|
  puts "#{test}"

  SHM_ARG.each {|shm|
    shm_val = shm * SHM_SCALE
    puts "SHM: #{shm_val}"

    SIZES.each { |size|
      size = size * SCALE
      total = 0.0
      NTIMES.times {
        time_str = `./#{test} #{size} #{shm_val} 2> /dev/null`
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
