When /^(.+?) receives acknowledge from (.+?)$/ do |destination, source|
  wait_or_fail 'Acknowledge not received within time' do
    $routers[destination.to_sym][:acks].include? source.to_sym
  end
end

When /^(.+?) receives from (.+?) message "(.+?)"$/ do |destination, source, message|
  wait_for_message(destination, source, 15, message, $receive_timeout)
end

When /^(.+?) receives back message "(.+?)"$/ do |source, message|
  wait_for_undelivered_message(source, message, $receive_timeout)
end

When /^(.+?) does not (.+?) (.*?)$/ do |a, noun, b|
  success = false
  begin
    step "#{a} #{noun}s #{b}"
    success = true
  rescue
  end

  fail 'Somehow step succeeded' if success
end

When /^"(.+?)" route is (.+?)$/ do |message, route|
  fail "Different route - #{$messages[message].map(&:to_s).join(', ')}" unless compare_routes(make_route(route), $messages[message])
end

When /^route contains (.+?)$/ do |route|
  pending
end

When /^(.+?) broadcasts data (.+?)$/ do |node, data|
  wait_for_data(node, nil, data, $receive_timeout)
end

When /^(.+?) transmits to (.+?) data (.+?)$/ do |source, destination, data|
  wait_for_data(source, destination, data, $receive_timeout)
end

When /^(.+?) is broadcasted from (.+?)$/ do |type, node|
  data = case type
    when 'node'
      'FESS'
    when 'graph'
      'FB.*?SS'
    else
      fail 'Unknown packet type!'
  end

  step "#{node} broadcasts data #{data}"
end
