require 'redis'
require_relative '../../../src/simulator'
require_relative 'helpers'

Before do |scenario|
  $logs = Logs.new(scenario)

  formatter = $logger.formatter
  $logger = Logger.new($logs.simulator)
  $logger_start = Time.now
  $logger.formatter = formatter

  $logger.level = 0

  $network = nil
  $receive_timeout = 5
  $routers = {}
  $messages = {}

  $redis = Redis.new(path: '/tmp/redis.sock')
end

After do
  kill_routers!
  $routers = {}
  $network.stop! if $network
  $network = nil

  $logs.save

  Thread.list.each do |thread|
    Thread.kill thread unless thread == Thread.main
  end
end

$redis = nil
$network = nil
$routers = {}
$logger.level = 0
$receive_timeout = 5
$messages = {}
$logs = nil

Logs.next

if not File.exist? '/tmp/redis.sock'
  STDERR.puts 'Redis instance is required (with unix socket on /tmp/redis.sock)'
  exit 1
end

at_exit do
  kill_routers!
end
