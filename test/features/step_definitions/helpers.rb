require_relative '../../../src/simulator'
require 'redis'
require 'json'
require 'fileutils'

class String
  def underscore
    self.gsub(/::/, '/').
    gsub(/([A-Z]+)([A-Z][a-z])/,'\1_\2').
    gsub(/([a-z\d])([A-Z])/,'\1_\2').
    tr('-', '_').
    tr(' ', '_').
    gsub(/[^a-zA-Z0-9_]/, '').
    downcase
  end
end

class Logs
  def initialize(scenario)
    @files = {}
    @directory = File.join(@@directory, scenario.feature.name.underscore, scenario.name.underscore)

    FileUtils.makedirs @directory
  end

  def save
    @files.each_pair do |key, file|
      file.close unless file.closed?
    end
  end

  def simulator
    file_for :simulator
  end

  def router(name)
    file_for "#{name}.router"
  end

  def file_for(name)
    @files[name] = File.open(File.join(@directory, "#{name}.log"), 'w+') unless @files.has_key? name

    return @files[name]
  end

  def self.next
    @@directory = File.join('logs/test', ([Dir['logs/test/*'].size-1, 0].max + 1).to_s)
    FileUtils.makedirs @@directory

    FileUtils.rm 'logs/test/last', force: true
    FileUtils.symlink File.absolute_path(@@directory), 'logs/test/last'
  end
end

def wait_or_fail(message, timeout = $receive_timeout, delay = 0.05, &condition)
  t = 0.0

  while not condition.call
    t += delay

    if t >= timeout
      fail message
    else
      sleep delay
    end
  end
end

def kill_routers!
  $routers.each_pair do |name, router|
    begin
      Process.kill 9, router[:pid] if router[:pid]
    rescue
      puts "#{name} died prematurely"
    end
  end
end

def network_topology
  topology = {}

  $network.nodes_by_name.each_pair do |name, node|
    topology[$routers[name.to_sym][:address]] = []
    node.adjacent.each_pair do |mac, neighbour|
      topology[$routers[name.to_sym][:address]] << $routers[neighbour.id.to_sym][:address]
    end
  end

  return topology
end

def spawn_router(name)
  address = $routers.size+1
  $routers[name] = {
    address: address,
    pid: nil,
    stdout: nil,
    alive: false,
    topology: {},
    messages: [],
    transmits: [],
    acks: [],
    undelivered: []
  }

  router_executable = File.join(File.dirname(__FILE__), '..', '..', '..', 'bin', 'router')

  # ensure router binary exists
  `make -j9` if not File.exist? router_executable

  env = {
    'REDIS_DATABASE' => address.to_s,
    'GLOG_logtostderr' => '1',
    'IN_SIMULATOR' => '1',
    'DYLD_LIBRARY_PATH' => './bin',
    'LD_LIBRARY_PATH' => './bin'
  }

  pipe_out, pipe_in = IO.pipe
  pid = spawn(env, router_executable,
              $network.nodes_by_name[name].tty,
              address.to_s,
              [:out, :err] => pipe_in)


  Process.detach(pid)

  pipe_in.close

  $routers[name][:pid] = pid
  $routers[name][:stdout] = pipe_out

  Thread.new do
    while line = pipe_out.gets do
      case line.strip
        when /^ROUTER RUN$/
          $routers[name][:alive] = true
        when /^EDGE (\d+) (\d+)$/
          $routers[name][:topology][$1.to_i] ||= []
          $routers[name][:topology][$1.to_i] << $2.to_i
          $routers[name][:topology][$1.to_i].sort!
          $routers[name][:topology][$2.to_i] ||= []
          $routers[name][:topology][$2.to_i] << $1.to_i
          $routers[name][:topology][$2.to_i].sort!
        # when /^UPDATE/
          # puts "#{name}: " + line
        # else
          # puts line
      end
      $logs.router(name).puts line
    end
  end


  Thread.new do
    $routers[name][:redis] = Redis.new(path: '/tmp/redis.sock')
    $routers[name][:redis].psubscribe("#{address}/*") do |on|
      on.pmessage do |pattern, event, message|
        type, port, destination, source = event.split(':')
        type.sub! /(\d+)\//, ''

        case type
          when 'network'
            if destination == 'self' and source != 'self'
              source = $routers.select{|n,r|r[:address] == source.to_i}.keys.first
              $routers[name][:messages] << { source: source, port: port.to_i, message: message }
            end
          when 'undelivered'
            $routers[name][:undelivered] << message
        end
      end
    end
  end

end

def send_message(source, destination, port, message)
  source_address = $routers[source.to_sym][:address]
  destination_address = $routers[destination.to_sym][:address]

  $messages[message] ||= []
  $messages[message] << source.to_sym
  $redis.publish("#{source_address}/network:#{port}:#{destination_address}:self", message) > 1
end

def compare_routes(template, route)
  route.each_with_index do |node, index|
    return false unless template[index].include? node.to_s
  end

  return true
end

def data_regex(source, destination, data)
  source_hex = "%02X" % $routers[source.to_s.to_sym][:address]
  destination_hex = "%02X" % $routers[destination.to_s.to_sym][:address] rescue ''

  hexdata = data.upcase.gsub(' ','').gsub('SS', source_hex).gsub('TT', destination_hex)

  Regexp.new "^#{hexdata}"
end

def hex_to_string(hex)
  hex.each_char.map{|c|"%02X"%c.ord}.join
end

def wait_for_data(source, destination, data, timeout = 5)
  wait_or_fail 'Data not sent within time' do
    $routers[source.to_s.to_sym][:transmits].find do |d,f|
      (destination == nil or d == $network.nodes_by_name[destination.to_s.to_sym]) and
        data_regex(source, destination, data) =~ hex_to_string(f.data)
    end
  end
end

def make_route(route)
  r=route.gsub(/([a-z0-9A-Z_]+)/){ "\"#{$1}\"" }
  JSON.parse("[#{r}]")
end

def wait_for_message(destination, source, port, message, timeout = 5)
  wait_or_fail 'Message not received within time' do
    $routers[destination.to_s.to_sym][:messages].find do |m|
      m[:source] == source.to_s.to_sym and m[:port] == port and m[:message] == message
    end
  end
end

def wait_for_undelivered_message(source, message, timeout = 5)
  wait_or_fail 'Message not received back within time' do
    $routers[source.to_s.to_sym][:undelivered].include? message
  end
end
