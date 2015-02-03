#!/usr/bin/env ruby

require 'logger'
require 'optparse'

$logger = Logger.new(STDERR)
$logger_start = Time.now
$logger.formatter = proc do |severity, datetime, progname, msg|
  "#{severity[0]} [#{'%8.03f' % (datetime - $logger_start)}] #{progname}: #{msg}\n"
end

require_relative 'simulator/frame'
require_relative 'simulator/node'
require_relative 'simulator/network'


module XBee
  def self.run!
    options = {
      environment: nil,
      topology: nil,
      speed: 1
    }

    parser = OptionParser.new do |opts|
      opts.banner = "Usage: simulator options"

      opts.on '-ePATH', '--environment PATH', String, 'Environment definition file' do |e|
        options[:environment] = e
      end

      opts.on '-tPATH', '--topology PATH', String, 'Topology definition file' do |t|
        options[:topology] = t
      end

      opts.on '-sFREQUENCY', '--speed FREQUENCY', Float, 'Frequency' do |s|
        options[:speed] = s.to_i
      end

      opts.on '-lLEVEL', Float, 'Logger level' do |s|
        $logger.level = s.to_i
      end

      opts.on_tail '-h', '--help', 'Show this message' do
        puts opts
        exit
      end
    end

    parser.parse!

    if not options[:environment] or not options[:topology]
      puts 'Both environment and topology are required!'
      puts parser
      exit 1
    end

    Signal.trap 'INT' do
      puts 'Closing...'
      exit 0
    end

    network = XBee::Network.create(*options.values)
    network.spawn!
  end
end

XBee::run! if __FILE__ == $0

