module XBee
  module Frame
    module Meta
      module Static
        attr_accessor :template
        attr_accessor :frame_type

        def from_bytes(bytes)
          _, data_size, frame_type = bytes.unpack 'CS>C'

          matrix = self.template.reduce '' do |head, field|
            head + (field[1][0] == '*' ? field[1][1..-1] + (data_size-4).to_s : field[1])
          end

          self.new *(bytes[4..-2].unpack matrix)
        end
      end

      def to_s
        fields = to_h.select{|k,v| v != nil and not (v.is_a? String and v.empty?) }.map do |k, v|
          value = v.inspect

          if v.is_a? Integer
            value = v.to_s(16).upcase
            value = '0x' + value.rjust(v > 255 ? (2 ** (Math.log(value.size)/Math.log(2)).ceil) : 2, '0')
          elsif v.is_a? String and not v.each_char.reduce(true){|r,c| r and (('0'..'9').include? c or ('a'..'z').include? c.downcase)}
            value = v.each_char.map{|c|"%02X"%c.ord}.join(' ')
          elsif v.is_a? Array
            value = v.map(&:inspect).join(' or ')
          end

          "#{k}: #{value}"
        end.join(', ')

        "#{self.class.name.split('::').last}(#{fields})"
      end

      def to_bytes
        self.class.template.each_pair do |field, type|
          next unless type[0] == '*'
          next unless self[field].is_a? Integer

          t = self[field]

          if t == 0
            self[field] = "\0"
          else
            s = ''

            while t > 0 do
              s += (t % 256).chr
              t /= 256
            end

            ## FIXME dirty hack against size of MAC
            if self.is_a? CommandResponse and self.command == 'SH'
              s = s.ljust(4, "\0")
            end

            self[field] = s.reverse
          end
        end

        matrix = self.class.template.reduce '' do |head, field|
          head + (field[1][0] == '*' ? field[1][1..-1] + (self[field[0]] || '').size.to_s : field[1])
        end

        s = [self.class.frame_type, *self].to_a.pack 'C' + matrix
        checksum = 0xff - s.split('').map(&:ord).inject(&:+) % 0xff

        [0x7e, s.size].pack('CS>') + s + [checksum].pack('C')
      end

      def self.included(o)
        o.extend Static
      end
    end

    def frame(id, name, **args)
      classname = name.to_s.split('_').collect(&:capitalize).join

      klass = Struct.new(*args.keys)
      klass.include Meta

      klass.template = args
      klass.frame_type = id

      @@class_for_symbol[name] = klass
      @@class_for_type[id] = klass

      XBee::Frame.const_set(classname, klass)
    end

    def self.from_bytes(bytes)
      @@class_for_type[bytes[3].ord].from_bytes bytes
    end

    @@class_for_symbol = {}
    def self.from_symbol(s)
      @@class_for_symbol[s]
    end

    @@class_for_type = {}
    def self.from_type(s)
      @@class_for_type[s]
    end

    module_function :frame
    private_class_method :frame
  end
end

