module XBee
  class Node
    receive :transmit, mac: 0xffff do |frame|
      info 'Broadcasting frame'

      @network.record self, frame
      broadcast frame.id, frame.data
    end

    receive :transmit do |frame|
      info 'Sending frame'

      deliver frame.mac, frame.id, frame.data
    end

    def broadcast(id, data)
      @adjacent.each_value do |node|
        next unless node.active?

        debug "Sending data to #{node.id}"

        node.receive self, id, data, false
      end
    end

    def deliver(mac, id, data)
      if @adjacent.has_key? mac and @adjacent[mac].active?
        @adjacent[mac].receive self, id, data
      else
        ack @network.nodes_by_mac[mac], id, @max_retransmissions, 0x25 if id > 0
      end
    end

    def receive(source, id, data, ack = true)
      Thread.new do
        retries = [@network.current_value([source, self], :retries).to_i, 0].max
        node_failure = @network.current_value(self, :power) < 1
        edge_failure = @network.current_value([source, self], :errors) > 0
        retries = @max_retransmissions if node_failure or edge_failure

        # delay is accumulating for each retry
        delay = (@network.current_value([source, self], :delay) / 1000.0) * (retries + 1)
        sleep delay
        debug "Edge delay [#{source.inspect}, #{self.inspect}] is #{delay}s (#{retries} retries)"

        # no ack if not required
        ack = false if id == 0

        # check for node failure
        if ack and node_failure
          warn 'Power failure'
          return source.ack(self, id, retries, 1)
        end

        # check for edge failure
        if ack and edge_failure
          warn "Edge failure [#{source.inspect}, #{self.inspect}]"
          return source.ack(self, id, retries, 1)
        end

        info "Received data from #{source.id}"

        # deliver packet
        frame = reply :receive,
                      id: id,
                      mac: source.mac,
                      network: @network.id,
                      options: 0,
                      data: data

        # let observers know
        @network.record source, frame, self

        # packet ack
        source.ack self, id, retries if ack
      end
    end

    def ack(destination, id, retries = 0, status = 0)
      # ack is send (not generated!) only if message was delivered
      if status == 0
        delay = @network.current_value([destination, self], :delay) / 2000.0
        debug "Edge delay [#{destination.inspect}, #{self.inspect}] is #{delay}s"
        sleep delay
      end

      info "ACK from #{destination.id} - #{retries} retries, #{status} status"

      reply :status,
        id: id,
        network: @network.id,
        retries: retries,
        status: status,
        discovery: 0
    end
  end
end
