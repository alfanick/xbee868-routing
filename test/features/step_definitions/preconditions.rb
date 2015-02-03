Given /^simulation of (.+?) network in (.+?) environment$/ do |network_name, environment_name|
  network_file = File.join(File.dirname(__FILE__), '..', '..', 'fixtures', "#{network_name}.network.yml")
  environment_file = File.join(File.dirname(__FILE__), '..', '..', 'fixtures', "#{environment_name}.environment.yml")

  Thread.new do
    $network = XBee::Network.create(environment_file, network_file)

    $network.spoof do |source, destination, frame|
      $routers[source.id.to_sym][:transmits] << [destination, frame]

      case frame.data[0]
        when "\x01"
          message = frame.data[6..-1][(frame.data[5].ord)..-1]

          $messages[message] ||= []
          $messages[message] << destination.id.to_sym
        when "\x03"
          $routers[destination.id.to_sym][:acks] << source.id.to_sym
      end
    end

    $network.spawn! false
  end

  wait_or_fail 'Could not simulate network' do
    $network and $network.nodes_by_name.values.reduce(true){|s,n| s and (n.tty != nil)}
  end
end

Given /^time is (\d+)$/ do |time|
  $network.set_time time.to_i
end

Given /timeout is (.+)s$/ do |time|
  $receive_timeout = time.to_f
end

Given /^node (.+?) is down$/ do |node|
  $network.current_time_point[$network.nodes_by_name[node.to_sym]] = XBee::Network.make_distributions(power: 0)
  $network.build_distributions
end

Given /^router (.+?) is alive$/ do |router|
  spawn_router router.to_sym

  wait_or_fail 'Could not start router' do
    $routers[router.to_sym][:alive]
  end
end

Given /^every router is alive$/ do
  $network.nodes_by_name.each_key do |name|
    spawn_router name
  end

  wait_or_fail 'Could not start every router', 10 do
    $routers.values.reduce(true){|s,r| s and r[:alive]}
  end
end

Given /^topology is discovered$/ do
  current_topology = network_topology

  wait_or_fail 'Topology not discovered' do
    $routers.values.reduce(true){|s,r| s and (r[:topology] == current_topology)}
  end
end

When /^(.+?) sends to (.+?) message "(.*?)"$/ do |source, destination, message|
  send_message(source, destination, 15, message)
end
