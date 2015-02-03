require 'rubygems'
require 'croupier'

module XBee
  class Network
    def self.make_distributions(options)
      result = options.map do |property, distribution|
        if distribution.is_a? Integer
          distribution = {
            distribution: :degenerate,
            constant: distribution
          }
        end

        distribution = Hash[distribution.map{|k,v|[k.to_sym,v.is_a?(String) ? v.to_sym : v]}]

        if distribution[:distribution] == :constant
          distribution[:distribution] = :degenerate
          distribution[:constant] = distribution[:value] if distribution[:value]
        end

        [property.to_sym, distribution]
      end

      Hash[*result.flatten]
    end

    def spoof(&block)
      @spies << block
    end

    def record(source, frame, destination = nil)
      @spies.each do |spy|
        spy.call(source, destination, frame)
      end
    end

    def build_distributions
      @timeline.each_pair do |point, event|
        event.each_pair do |key, parameters|
          parameters.each_pair do |name, d|
            next unless d[:distribution].is_a? Symbol
            @timeline[point][key][name] = {
              distribution: Croupier::Distributions.send(d[:distribution], **d),
              scale: (d[:scale] || 1).to_f,
              bias: (d[:bias] || 0).to_f
            }
          end
        end
      end
    end

    def current_time_point
      t = Time.now
      dt = (t.to_i + t.usec / 1000000.0) - @start_time
      dt = @time if @time != nil and @time > 0

      @current_time_point = @timeline[@timeline.keys.sort.reverse.find{|x| x <= dt}] if dt > @current_time_point_expire_time or @current_time_point == nil

      @current_time_point
    end

    def current_value(key, name)
      params = current_time_point[key][name.to_sym]

      params[:scale] * params[:distribution].first + params[:bias]
    end

    def time_point!(point, edges, nodes)
      @timeline[point] ||= Marshal.load(Marshal.dump(@timeline.values.last)) || {}

      edges.each_pair do |edge, properties|
        if edge == 'all'
          @nodes_by_name.values.each do |node|
            node.adjacent.values.each do |adjacent|
              @timeline[point][[node, adjacent]] ||= {}
              @timeline[point][[adjacent, node]] ||= {}
              @timeline[point][[node, adjacent]].merge! Network.make_distributions(properties)
              @timeline[point][[adjacent, node]].merge! Network.make_distributions(properties)
            end
          end
        else
          edge.map!{|n|@nodes_by_name[n.to_s.to_sym]}

          @timeline[point][edge] ||= {}
          # @timeline[point][edge.reverse] ||= {}
          @timeline[point][edge].merge! Network.make_distributions(properties)
          # @timeline[point][edge.reverse].merge! Network.make_distributions(properties)
        end
      end

      nodes.each_pair do |node, properties|
        if node == 'all'
          @nodes_by_name.values.each do |node|
            @timeline[point][node] ||= {}
            @timeline[point][node].merge! Network.make_distributions(properties)
          end
        else
          node = @nodes_by_name[node.to_s.to_sym]

          @timeline[point][node] ||= {}
          @timeline[point][node].merge! Network.make_distributions(properties)
        end
      end
    end
  end
end
