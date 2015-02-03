require_relative 'dsl/frame'

module XBee
  module Frame
    frame 0x8a, :modem_status,
      status: 'C'

    frame 0x08, :command,
      id: 'C',
      command: 'a2',
      data: '*a'

    frame 0x09, :command_queue,
      id: 'C',
      command: 'a2',
      data: '*a'

    frame 0x88, :command_response,
      id: 'C',
      command: 'a2',
      status: 'C',
      data: '*a'

    frame 0x17, :remote_command,
      id: 'C',
      mac: 'Q>',
      network: 'S>',
      options: 'C',
      command: 'a2',
      data: '*a'

    frame 0x97, :remote_command_response,
      id: 'C',
      mac: 'Q>',
      network: 'S>',
      command: 'a2',
      status: 'C',
      data: '*a'

    frame 0x10, :transmit,
      id: 'C',
      mac: 'Q>',
      network: 'S>',
      radius: 'C',
      options: 'C',
      data: '*a'

    frame 0x11, :explicit_transmit,
      id: 'C',
      mac: 'Q>',
      network: 'S>',
      source_endpoint: 'C',
      destination_endpoint: 'C',
      cluster: 'S>',
      profile: 'S>',
      radius: 'C',
      options: 'C',
      data: '*a'

    frame 0x8b, :status,
      id: 'C',
      network: 'S>',
      retries: 'C',
      status: 'C',
      discovery: 'C'

    frame 0x90, :receive,
      id: 'C',
      mac: 'Q>',
      network: 'S>',
      options: 'C',
      data: '*a'

    frame 0x91, :explicit_receive,
      mac: 'Q>',
      network: 'S>',
      source_endpoint: 'C',
      destination_endpoint: 'C',
      cluster: 'S>',
      profile: 'S>',
      options: 'C',
      data: '*a'
  end
end

