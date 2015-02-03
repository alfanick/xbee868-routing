module XBee
  class Node
    receive :command, command: 'FR' do |frame|
      info 'XBee reset'
      reply :modem_status,
        status: 0

      reset!
      @master.flush
    end

    receive :command, command: 'NI' do |frame|
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0,
        data: @id.to_s
    end

    receive :command, command: 'ID' do |frame|
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0,
        data: @network.id
    end

    receive :command, command: 'SL' do |frame|
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0,
        data: (@mac % (0xffffffff+1))
    end

    receive :command, command: 'SH' do |frame|
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0,
        data: 0x0013a200
    end

    receive :command, command: 'PL' do |frame|
      info "Setting power level to #{frame.data.ord}"
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0
    end

    receive :command, command: ['MT', 'RR'] do |frame|
      info "Setting retransmissions to #{frame.data.ord}"
      @max_retransmissions = frame.data.ord
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0
    end

    receive :command, command: 'DB' do |frame|
      info 'Requesting signal strength of last packet'
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 0,
        data: 0x60
    end

    receive :command do |frame|
      warn "Unimplemented command '#{frame.command}'"
      reply :command_response,
        id: frame.id,
        command: frame.command,
        status: 2
    end
  end
end
