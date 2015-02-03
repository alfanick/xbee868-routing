require 'yaml'
require_relative 'dsl/network'

module XBee
  class Network
    def self.create(environment_path, topology_path, speed=1)
      network = XBee::Network.new(0xfffe)

      begin
        topology = YAML.load_file(topology_path)

        topology.keys.each do |name|
          network.add_node name
        end

        topology.each_pair do |name, neighbours|
          neighbours.each do |neighbour|
            network.add_edge name, neighbour
          end
        end
      rescue
        puts 'Malformed topology file'
        exit 1
      end

      begin
        environment = YAML.load_file(environment_path)

        current_time = 0
        environment.each_pair do |name, definition|
          point = definition['point'] || (definition['delay'] + current_time)
          point /= speed.to_f
          current_time = point

          network.time_point! point, definition['edges'], definition['nodes']
        end

        network.build_distributions
      rescue
        puts 'Malformed environment file'
        exit 1
      end

      return network
    end

    def initialize(id)
      @id = id
      @nodes_by_mac = {}
      @nodes_by_name = {}

      @timeline = {}
      @threads = []
      @start_time = 0.0
      @current_time_point = nil
      @current_time_point_expire_time = 0.0
      @spies = []
      @time = 0.0
    end

    def set_time(time)
      @time = time
    end

    def add_node(name)
      node = Node.new(name.to_s, self)
      @nodes_by_mac[node.mac] = node
      @nodes_by_name[name.to_s.to_sym] = node
    end

    def add_edge(a, b)
      @nodes_by_name[a.to_s.to_sym].connect_with @nodes_by_name[b.to_s.to_sym]
      @nodes_by_name[b.to_s.to_sym].connect_with @nodes_by_name[a.to_s.to_sym]
    end

    def spawn!(wait = true)
      t = Time.now
      @start_time = t.to_i + t.usec / 1000000.0

      @nodes_by_mac.each_value do |node|
        @threads << node.spawn!
      end

      if wait
        sleep
      else
        @nodes_by_mac.each_value do |node|
          loop do
            break if node.tty
          end
        end
      end
    end

    def stop!
      @threads.each do |thread|
        thread.exit
        thread.join
      end

      @nodes_by_mac.each_value do |node|
        node.master.close
      end
    end

    attr_reader :id
    attr_reader :nodes_by_name
    attr_reader :nodes_by_mac
  end
end
