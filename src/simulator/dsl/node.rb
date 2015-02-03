require 'pty'
require 'thread'

module XBee
  class Node
    @@handlers = {}
    @@count = 0
    @@mutex = {}

    attr_reader :mac
    attr_reader :id
    attr_reader :tty

    def initialize(id, network)
      @id = id.to_s
      @@count += 1
      @mac = 0x0013a20000000000 + @@count
      @network = network
      @@mutex[self] ||= Mutex.new
      @tty = nil

      reset!
    end

    def active?
      @active
    end

    def reset!
      @max_retransmissions = 10
      @active = false
    end

    attr_reader :master

    def spawn!
      @@mutex[self].synchronize do
        @master, @slave = PTY.open
      end
      system 'stty raw', in: @slave

      @thread = Thread.new do
        info "PTY at '#{@slave.path}'"

        @tty = @slave.path

        packet = ''
        length = 0
        while last = @master.readbyte do
          if last == 0x7e
            packet = ''
            info 'Incoming frame'
          elsif packet.size == 1
            length = last << 8
          elsif packet.size == 2
            length |= last
          end

          packet += last.chr

          if packet.size == length + 4
            hex_packet = packet.split('').map{|x|"%02X" % x.ord}.join(' ')
            @active = true
            debug "Frame data: #{hex_packet}"

            begin
              frame = Frame.from_bytes packet
              debug "Frame #{frame}"

              begin
                instance_exec(frame, &handler_for_frame(frame))
              rescue
                error "Unknown error - '#{$!}'"
                debug $!.backtrace.join("\n")
              end
            rescue
              error 'Unknown frame type'
            end
          end
        end
      end

    end

    def handler_for_frame(frame)
      @@handlers.each_pair do |predicate, handler|
        next unless frame.is_a? predicate.class

        not_nil = 0
        accepted_values = 0

        predicate.each_pair do |field, value|
          next if value == nil
          not_nil += 1

          break unless frame[field] == value or (value.is_a? Array and value.include? frame[field])
          accepted_values += 1
        end

        if accepted_values == not_nil
          info "Handling with #{predicate}"

          return handler
        end
      end

      Proc.new do |f|
        error "No handler found for received frame type #{f.class}"
      end
    end

    def stop!
      @thread.join
    end

    def self.receive(type, **args, &block)
      struct = frame_for_type(type).new
      args.each_pair{|k,v| struct[k] = v}

      @@handlers[struct] = block
    end

    def reply(type, **args)
      frame = Node.frame_for_type(type).new
      args.each_pair{|k,v| frame[k] = v}

      begin
        packet = frame.to_bytes

        if (frame.is_a? Frame::Receive)
          ## FIXME: some super dirty hack is going on here
          packet.slice! 5
          packet[2] = (packet[2].ord-1).chr
        end
        info "Outgoing frame #{frame}"

        hex_packet = packet.split('').map{|x|"%02X" % x.ord}.join(' ')
        debug "Frame data: #{hex_packet}"
      rescue
        error "Incomplete frame #{frame.class}"
      end

      @@mutex[self].synchronize do
        @master.write packet
      end

      return frame
    end

    attr_reader :adjacent

    def inspect
      "NODE #{id.to_s}"
    end

    def connect_with(other)
      @adjacent ||= {}
      @adjacent[other.mac] = other
    end

    def hash
      @mac
    end

    def eql?(other)
      other.mac == @mac
    end

    def self.frame_for_type(type)
      if type.is_a? Symbol
        XBee::Frame.from_symbol type
      elsif type.is_a? Integer
        XBee::Frame.from_type type
      elsif type.is_a? Struct
        type
      else
        nil
      end
    end

    private_class_method :receive

    def debug(message)
      $logger.debug(@id) { message }
    end

    def info(message)
      $logger.info(@id) { message }
    end

    def warn(message)
      $logger.warn(@id) { message }
    end

    def error(message)
      $logger.error(@id) { message }
    end
  end
end
