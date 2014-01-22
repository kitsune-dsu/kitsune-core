#!/usr/bin/env ruby

NTIMES = 20

TESTS = ["uds-copy-pull"]

SIZES = 1..10
SCALE = 50_000_000

BLOCKS = [1000, 10000]
BLOCK_SCALE = 1

TESTS.each { |test|
  puts "#{test}"

	BLOCKS.each {|block|
		puts "BLOCKS: #{block * BLOCK_SCALE}"
		
		SIZES.each { |size|

			size = size * SCALE
			block_val = size / (block * BLOCK_SCALE)		
		
			#puts "BLOCKS: #{block_val}"
			
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
			puts "#{size/1000}, #{average}, \n"
    }
  }
}
